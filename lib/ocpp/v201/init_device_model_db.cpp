// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/init_device_model_db.hpp>

#include <cstdint>
#include <fstream>
#include <map>
#include <string>

#include <everest/logging.hpp>

const static std::string STANDARDIZED_COMPONENT_CONFIG_DIR = "standardized";
const static std::string CUSTOM_COMPONENT_CONFIG_DIR = "custom";

// TODO mz change EverestEnvironmentOCPPConfiguration in everest_environment_setup.py in everest-utils
// TODO mz search for component_schemas and config_file etc to remove or change paths

namespace ocpp::v201 {

// Forward declarations.
static void check_integrity(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_configs);
static bool is_same_component_key(const ComponentKey& component_key1, const ComponentKey& component_key2);
static bool is_same_attribute_type(const VariableAttribute attribute1, const VariableAttribute& attribute2);
static bool is_attribute_different(const VariableAttribute& attribute1, const VariableAttribute& attribute2);
static bool variable_has_same_attributes(const std::vector<DbVariableAttribute>& attributes1,
                                         const std::vector<DbVariableAttribute>& attributes2);
static bool is_characteristics_different(const VariableCharacteristics& c1, const VariableCharacteristics& c2);
static bool is_same_variable(const DeviceModelVariable& v1, const DeviceModelVariable& v2);
static bool is_variable_different(const DeviceModelVariable& v1, const DeviceModelVariable& v2);
static std::string get_string_value_from_json(const json& value);
static std::string get_component_name_for_logging(const ComponentKey& component);
static std::string get_variable_name_for_logging(const DeviceModelVariable& variable);

InitDeviceModelDb::InitDeviceModelDb(const std::filesystem::path& database_path,
                                     const std::filesystem::path& migration_files_path) :
    common::DatabaseHandlerCommon(std::make_unique<common::DatabaseConnection>(database_path), migration_files_path,
                                  MIGRATION_DEVICE_MODEL_FILE_VERSION_V201),
    database_path(database_path),
    database_exists(std::filesystem::exists(database_path)) {
}

InitDeviceModelDb::~InitDeviceModelDb() {
    close_connection();
}

void InitDeviceModelDb::initialize_database(const std::filesystem::path& config_path, bool delete_db_if_exists = true) {
    execute_init_sql(delete_db_if_exists);

    // Get existing components from the database.
    std::map<ComponentKey, std::vector<DeviceModelVariable>> existing_components;
    DeviceModelMap device_model;
    if (this->database_exists) {
        existing_components = get_all_components_from_db();
    }

    // Get component schemas from the filesystem.
    std::map<ComponentKey, std::vector<DeviceModelVariable>> component_configs = get_all_component_configs(config_path);

    // Check if the config is consistent (fe has a value when required).
    check_integrity(component_configs);

    // Remove components from db if they do not exist in the component schemas
    if (this->database_exists) {
        remove_not_existing_components_from_db(component_configs, existing_components);
    }

    // Starting a transaction makes this a lot faster (inserting all components takes a few seconds without it and a
    // few milliseconds if it is done inside a transaction).
    std::unique_ptr<common::DatabaseTransactionInterface> transaction = database->begin_transaction();
    insert_components(component_configs, existing_components);
    transaction->commit();
}

void InitDeviceModelDb::execute_init_sql(const bool delete_db_if_exists) {
    if (delete_db_if_exists) {
        if (std::filesystem::exists(database_path)) {
            if (!std::filesystem::remove(database_path)) {
                EVLOG_AND_THROW(InitDeviceModelDbError("Could not remove database " + database_path.u8string()));
            }

            database_exists = false;
        }
    }

    if (database_exists) {
        // Check if this is an old database version.
        try {
            this->database->open_connection();
            if (this->database->get_user_version() == 0) {
                EVLOG_AND_THROW(
                    InitDeviceModelDbError("Database does not support migrations yet, please update the database."));
            }
        } catch (const std::runtime_error& /* e*/) {
            EVLOG_AND_THROW(
                InitDeviceModelDbError("Database does not support migrations yet, please update the database."));
        }
    }

    // Connect to the database. This will automatically do the migrations (including the initial sql file).
    open_connection();
}

std::vector<std::filesystem::path>
InitDeviceModelDb::get_component_config_from_directory(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> component_schema_files;
    for (const auto& p : std::filesystem::directory_iterator(directory)) {
        if (p.path().extension() == ".json") {
            component_schema_files.push_back(p.path());
        }
    }

    return component_schema_files;
}

std::map<ComponentKey, std::vector<DeviceModelVariable>>
InitDeviceModelDb::get_all_component_configs(const std::filesystem::path& directory) {
    const std::vector<std::filesystem::path> standardized_component_schema_files =
        get_component_config_from_directory(directory / STANDARDIZED_COMPONENT_CONFIG_DIR);
    const std::vector<std::filesystem::path> custom_component_schema_files =
        get_component_config_from_directory(directory / CUSTOM_COMPONENT_CONFIG_DIR);

    std::map<ComponentKey, std::vector<DeviceModelVariable>> standardized_components_map =
        read_component_config(standardized_component_schema_files);
    std::map<ComponentKey, std::vector<DeviceModelVariable>> components =
        read_component_config(custom_component_schema_files);

    // Merge the two maps so they can be used for the insert_component function with a single iterator. This will use
    // the custom components map as base and add not existing standardized components to the components map. So if the
    // component exists in both, the custom component will be used.
    components.merge(standardized_components_map);

    return components;
}

void InitDeviceModelDb::insert_components(
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& components,
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& existing_components) {
    for (auto& component : components) {
        // Check if component already exists in the database.
        std::optional<std::pair<ComponentKey, std::vector<DeviceModelVariable>>> component_db;
        if (this->database_exists &&
            (component_db = component_exists_in_db(existing_components, component.first)).has_value()) {
            // Component exists in the database, update component if necessary.
            update_component_variables(component_db.value(), component.second);
        } else {
            // Database is new or component does not exist. Insert component.
            insert_component(component.first, component.second);
        }
    }
}

void InitDeviceModelDb::insert_component(const ComponentKey& component_key,
                                         const std::vector<DeviceModelVariable>& component_variables) {
    EVLOG_debug << "Inserting component " << component_key.name;

    static const std::string statement = "INSERT OR REPLACE INTO COMPONENT (NAME, INSTANCE, EVSE_ID, CONNECTOR_ID) "
                                         "VALUES (@name, @instance, @evse_id, @connector_id)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_component_statement;
    try {
        insert_component_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    insert_component_statement->bind_text("@name", component_key.name, ocpp::common::SQLiteString::Transient);
    if (component_key.connector_id.has_value()) {
        insert_component_statement->bind_int("@connector_id", component_key.connector_id.value());
    } else {
        insert_component_statement->bind_null("@connector_id");
    }

    if (component_key.evse_id.has_value()) {
        insert_component_statement->bind_int("@evse_id", component_key.evse_id.value());
    } else {
        insert_component_statement->bind_null("@evse_id");
    }

    if (component_key.instance.has_value() && !component_key.instance.value().empty()) {
        insert_component_statement->bind_text("@instance", component_key.instance.value(),
                                              ocpp::common::SQLiteString::Transient);
    } else {
        insert_component_statement->bind_null("@instance");
    }

    if (insert_component_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not insert component " + component_key.name + ": " +
                                     std::string(this->database->get_error_message()));
    }

    const int64_t component_id = this->database->get_last_inserted_rowid();

    // Loop over the properties of this component.
    for (const DeviceModelVariable& variable : component_variables) {
        EVLOG_debug << "-- Inserting variable " << variable.name;

        // Add variable
        insert_variable(variable, static_cast<uint64_t>(component_id));
    }
}

std::map<ComponentKey, std::vector<DeviceModelVariable>>
InitDeviceModelDb::read_component_config(const std::vector<std::filesystem::path>& components_schema_path) {
    std::map<ComponentKey, std::vector<DeviceModelVariable>> components;
    for (const std::filesystem::path& path : components_schema_path) {
        std::ifstream schema_file(path);
        try {
            json data = json::parse(schema_file);
            ComponentKey p = data;
            if (data.contains("properties")) {
                std::vector<DeviceModelVariable> variables =
                    get_all_component_properties(data.at("properties"), p.required);
                components.insert({p, variables});
            } else {
                EVLOG_warning << "Component " << data.at("name") << " does not contain any properties";
                continue;
            }
        } catch (const json::parse_error& e) {
            EVLOG_error << "Error while parsing schema file: " << path;
            throw;
        }
    }

    return components;
}

std::vector<DeviceModelVariable>
InitDeviceModelDb::get_all_component_properties(const json& component_properties,
                                                std::vector<std::string> required_properties) {
    std::vector<DeviceModelVariable> variables;

    for (const auto& variable : component_properties.items()) {
        DeviceModelVariable v = variable.value();
        const std::string variable_key_name = variable.key();

        // Check if this is a required variable and if it is, add that to the variable struct.
        if (std::find(required_properties.begin(), required_properties.end(), variable_key_name) !=
            required_properties.end()) {
            v.required = true;
        }
        variables.push_back(v);
    }

    return variables;
}

void InitDeviceModelDb::insert_variable_characteristics(const VariableCharacteristics& characteristics,
                                                        const int64_t& variable_id) {
    static const std::string statement = "INSERT OR REPLACE INTO VARIABLE_CHARACTERISTICS (DATATYPE_ID, VARIABLE_ID, "
                                         "MAX_LIMIT, MIN_LIMIT, SUPPORTS_MONITORING, "
                                         "UNIT, VALUES_LIST) VALUES (@datatype_id, @variable_id, @max_limit, "
                                         "@min_limit, @supports_monitoring, @unit, @values_list)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_characteristics_statement;
    try {
        insert_characteristics_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    insert_characteristics_statement->bind_int("@datatype_id", static_cast<int>(characteristics.dataType));

    insert_characteristics_statement->bind_int("@variable_id", static_cast<int>(variable_id));

    const uint8_t supports_monitoring = (characteristics.supportsMonitoring ? 1 : 0);
    insert_characteristics_statement->bind_int("@supports_monitoring", supports_monitoring);

    if (characteristics.unit.has_value()) {
        insert_characteristics_statement->bind_text("@unit", characteristics.unit.value(),
                                                    ocpp::common::SQLiteString::Transient);
    } else {
        insert_characteristics_statement->bind_null("@unit");
    }

    if (characteristics.valuesList.has_value()) {
        insert_characteristics_statement->bind_text("@values_list", characteristics.valuesList.value(),
                                                    ocpp::common::SQLiteString::Transient);
    } else {
        insert_characteristics_statement->bind_null("@values_list");
    }

    if (characteristics.maxLimit.has_value()) {
        insert_characteristics_statement->bind_double("@max_limit",
                                                      static_cast<double>(characteristics.maxLimit.value()));
    } else {
        insert_characteristics_statement->bind_null("@max_limit");
    }

    if (characteristics.minLimit.has_value()) {
        insert_characteristics_statement->bind_double("@min_limit",
                                                      static_cast<double>(characteristics.minLimit.value()));
    } else {
        insert_characteristics_statement->bind_null("@min_limit");
    }

    if (insert_characteristics_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError(this->database->get_error_message());
    }
}

void InitDeviceModelDb::update_variable_characteristics(const VariableCharacteristics& characteristics,
                                                        const int64_t& characteristics_id, const int64_t& variable_id) {
    static const std::string update_characteristics_statement =
        "UPDATE VARIABLE_CHARACTERISTICS SET DATATYPE_ID=@datatype_id, VARIABLE_ID=@variable_id, MAX_LIMIT=@max_limit, "
        "MIN_LIMIT=@min_limit, SUPPORTS_MONITORING=@supports_monitoring, UNIT=@unit, VALUES_LIST=@values_list WHERE "
        "ID=@characteristics_id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement;
    try {
        update_statement = this->database->new_statement(update_characteristics_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + update_characteristics_statement);
    }

    update_statement->bind_int("@datatype_id", static_cast<int>(characteristics.dataType));

    update_statement->bind_int("@characteristics_id", static_cast<int>(characteristics_id));
    update_statement->bind_int("@variable_id", static_cast<int>(variable_id));

    const uint8_t supports_monitoring = (characteristics.supportsMonitoring ? 1 : 0);
    update_statement->bind_int("@supports_monitoring", supports_monitoring);

    if (characteristics.unit.has_value()) {
        update_statement->bind_text("@unit", characteristics.unit.value(), ocpp::common::SQLiteString::Transient);
    } else {
        update_statement->bind_null("@unit");
    }

    if (characteristics.valuesList.has_value()) {
        update_statement->bind_text("@values_list", characteristics.valuesList.value(),
                                    ocpp::common::SQLiteString::Transient);
    } else {
        update_statement->bind_null("@values_list");
    }

    if (characteristics.maxLimit.has_value()) {
        update_statement->bind_double("@max_limit", static_cast<double>(characteristics.maxLimit.value()));
    } else {
        update_statement->bind_null("@max_limit");
    }

    if (characteristics.minLimit.has_value()) {
        update_statement->bind_double("@min_limit", static_cast<double>(characteristics.minLimit.value()));
    } else {
        update_statement->bind_null("@min_limit");
    }

    if (update_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not update variable characteristics: " +
                                     std::string(this->database->get_error_message()));
    }
}

void InitDeviceModelDb::insert_variable(const DeviceModelVariable& variable, const uint64_t& component_id) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE (NAME, INSTANCE, COMPONENT_ID, REQUIRED) VALUES "
        "(@name, @instance, @component_id, @required)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_variable_statement;
    try {
        insert_variable_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    insert_variable_statement->bind_text("@name", variable.name, ocpp::common::SQLiteString::Transient);
    insert_variable_statement->bind_int("@component_id", static_cast<int>(component_id));

    if (variable.instance.has_value() && !variable.instance.value().empty()) {
        insert_variable_statement->bind_text("@instance", variable.instance.value(),
                                             ocpp::common::SQLiteString::Transient);
    } else {
        insert_variable_statement->bind_null("@instance");
    }

    const uint8_t required_int = (variable.required ? 1 : 0);
    insert_variable_statement->bind_int("@required", required_int);

    if (insert_variable_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Variable " + variable.name +
                                     " could not be inserted: " + std::string(this->database->get_error_message()));
    }

    const int64_t variable_id = this->database->get_last_inserted_rowid();

    insert_variable_characteristics(variable.characteristics, variable_id);
    insert_attributes(variable.attributes, static_cast<uint64_t>(variable_id), variable.default_actual_value);
}

void InitDeviceModelDb::update_variable(const DeviceModelVariable& variable, const DeviceModelVariable& db_variable,
                                        const uint64_t component_id) {
    if (!db_variable.db_id.has_value()) {
        EVLOG_error << "Can not update variable " << variable.name << ": database id unknown";
        return;
    }

    static const std::string update_variable_statement =
        "UPDATE VARIABLE SET NAME=@name, INSTANCE=@instance, COMPONENT_ID=@component_id, REQUIRED=@required WHERE "
        "ID=@variable_id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement;
    try {
        update_statement = this->database->new_statement(update_variable_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + update_variable_statement);
    }

    update_statement->bind_int("@variable_id", static_cast<int>(db_variable.db_id.value()));
    update_statement->bind_text("@name", variable.name, ocpp::common::SQLiteString::Transient);
    update_statement->bind_int("@component_id", static_cast<int>(component_id));

    if (variable.instance.has_value() && !variable.instance.value().empty()) {
        update_statement->bind_text("@instance", variable.instance.value(), ocpp::common::SQLiteString::Transient);
    } else {
        update_statement->bind_null("@instance");
    }

    const uint8_t required_int = (variable.required ? 1 : 0);
    update_statement->bind_int("@required", required_int);

    if (update_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not update variable " + variable.name + ": " +
                                     std::string(this->database->get_error_message()));
    }

    if (db_variable.variable_characteristics_db_id.has_value() &&
        is_characteristics_different(variable.characteristics, db_variable.characteristics)) {
        update_variable_characteristics(variable.characteristics,
                                        static_cast<int64_t>(db_variable.variable_characteristics_db_id.value()),
                                        static_cast<int64_t>(db_variable.db_id.value()));
    }

    if (!variable_has_same_attributes(variable.attributes, db_variable.attributes)) {
        update_attributes(variable.attributes, db_variable.attributes, db_variable.db_id.value(),
                          variable.default_actual_value);
    }
}

void InitDeviceModelDb::delete_variable(const DeviceModelVariable& variable) {
    if (!variable.db_id.has_value()) {
        EVLOG_error << "Can not remove variable " << variable.name << " from db: id unknown";
        return;
    }

    static const std::string delete_variable_statement = "DELETE FROM VARIABLE WHERE ID=@variable_id";

    std::unique_ptr<common::SQLiteStatementInterface> delete_statement;
    try {
        delete_statement = this->database->new_statement(delete_variable_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + delete_variable_statement);
    }

    delete_statement->bind_int("@variable_id", static_cast<int>(variable.db_id.value()));

    if (delete_statement->step() != SQLITE_DONE) {
        EVLOG_error << "Can not remove variable " << variable.name
                    << " from db: " << this->database->get_error_message();
        throw InitDeviceModelDbError("Could not delete variable " + variable.name + ": " +
                                     std::string(this->database->get_error_message()));
    }
}

void InitDeviceModelDb::insert_attribute(const VariableAttribute& attribute, const uint64_t& variable_id,
                                         const std::optional<std::string>& default_actual_value) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_ATTRIBUTE (VARIABLE_ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID) "
        "VALUES(@variable_id, @mutability_id, @persistent, @constant, @type_id)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_attributes_statement;
    try {
        insert_attributes_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    insert_attributes_statement->bind_int("@variable_id", static_cast<int>(variable_id));
    insert_attributes_statement->bind_int("@persistent", 1);
    insert_attributes_statement->bind_int("@constant", 0);

    if (attribute.mutability.has_value()) {
        insert_attributes_statement->bind_int("@mutability_id", static_cast<int>(attribute.mutability.value()));
    } else {
        insert_attributes_statement->bind_null("@mutability_id");
    }

    if (attribute.type.has_value()) {
        insert_attributes_statement->bind_int("@type_id", static_cast<int>(attribute.type.value()));
    }

    if (insert_attributes_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not insert attribute: " + std::string(this->database->get_error_message()));
    }

    const int64_t attribute_id = this->database->get_last_inserted_rowid();

    if (attribute.value.has_value() ||
        (attribute.type.has_value() && (attribute.type.value() == AttributeEnum::Actual) &&
         default_actual_value.has_value())) {
        insert_variable_attribute_value(
            attribute_id, (attribute.value.has_value() ? attribute.value.value().get() : default_actual_value.value()),
            true);
    }
}

void InitDeviceModelDb::insert_attributes(const std::vector<DbVariableAttribute>& attributes,
                                          const uint64_t& variable_id,
                                          const std::optional<std::string>& default_actual_value) {
    for (const DbVariableAttribute& attribute : attributes) {
        insert_attribute(attribute.variable_attribute, variable_id, default_actual_value);
    }
}

void InitDeviceModelDb::update_attributes(const std::vector<DbVariableAttribute>& new_attributes,
                                          const std::vector<DbVariableAttribute>& db_attributes,
                                          const uint64_t& variable_id,
                                          const std::optional<std::string>& default_actual_value) {
    // First check if there are attributes in the database that are not in the config. They should be removed.
    for (const DbVariableAttribute& db_attribute : db_attributes) {
        const auto& it = std::find_if(
            new_attributes.begin(), new_attributes.end(), [&db_attribute](const DbVariableAttribute& new_attribute) {
                return is_same_attribute_type(db_attribute.variable_attribute, new_attribute.variable_attribute);
            });
        if (it == new_attributes.end()) {
            // Attribute not found in config, remove from db.
            delete_attribute(db_attribute);
        }
    }

    // Check if the variable attributes in the config match the ones from the database. If not, add or update.
    for (const DbVariableAttribute& new_attribute : new_attributes) {
        const auto& it = std::find_if(
            db_attributes.begin(), db_attributes.end(), [&new_attribute](const DbVariableAttribute& db_attribute) {
                return is_same_attribute_type(new_attribute.variable_attribute, db_attribute.variable_attribute);
            });

        if (it == db_attributes.end()) {
            // Variable attribute does not exist in the db, add to db.
            insert_attribute(new_attribute.variable_attribute, variable_id, default_actual_value);
        } else {
            if (is_attribute_different(new_attribute.variable_attribute, it->variable_attribute)) {
                update_attribute(new_attribute.variable_attribute, *it, default_actual_value);
            }
        }
    }
}

void InitDeviceModelDb::update_attribute(const VariableAttribute& attribute, const DbVariableAttribute& db_attribute,
                                         const std::optional<std::string>& default_actual_value) {
    if (!db_attribute.db_id.has_value()) {
        EVLOG_error << "Can not update attribute: id not found";
        return;
    }

    static const std::string update_attribute_statement =
        "UPDATE VARIABLE_ATTRIBUTE SET MUTABILITY_ID=@mutability_id, PERSISTENT=@persistent, CONSTANT=@constant, "
        "TYPE_ID=@type_id WHERE ID=@id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement;
    try {
        update_statement = this->database->new_statement(update_attribute_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + update_attribute_statement);
    }

    update_statement->bind_int("@id", static_cast<int>(db_attribute.db_id.value()));

    if (attribute.mutability.has_value()) {
        update_statement->bind_int("@mutability_id", static_cast<int>(attribute.mutability.value()));
    } else {
        update_statement->bind_null("@mutability_id");
    }

    if (attribute.persistent.has_value()) {
        update_statement->bind_int("@persistent", (attribute.persistent.value() ? 1 : 0));
    } else if (db_attribute.variable_attribute.persistent.has_value()) {
        update_statement->bind_int("@persistent", (db_attribute.variable_attribute.persistent.value() ? 1 : 0));
    } else {
        // Set default value.
        update_statement->bind_int("@persistent", 1);
    }

    if (attribute.constant.has_value()) {
        update_statement->bind_int("@constant", (attribute.constant.value() ? 1 : 0));
    } else if (db_attribute.variable_attribute.constant.has_value()) {
        update_statement->bind_int("@constant", (db_attribute.variable_attribute.constant.value() ? 1 : 0));
    } else {
        // Set default value.
        update_statement->bind_int("@constant", 0);
    }

    if (attribute.type.has_value()) {
        update_statement->bind_int("@type_id", static_cast<int>(attribute.type.value()));
    } else {
        update_statement->bind_null("@type_id");
    }

    if (update_statement->step() != SQLITE_DONE) {
        EVLOG_error << "Could not update variable attribute: " << this->database->get_error_message();
        throw InitDeviceModelDbError("Could not update attribute: " + std::string(this->database->get_error_message()));
    }

    if (attribute.value.has_value() ||
        (attribute.type.has_value() && (attribute.type.value() == AttributeEnum::Actual) &&
         default_actual_value.has_value())) {
        if (!insert_variable_attribute_value(
                static_cast<int64_t>(db_attribute.db_id.value()),
                (attribute.value.has_value() ? attribute.value.value().get() : default_actual_value.value()), false)) {
            EVLOG_error << "Can not update variable attribute (" << db_attribute.db_id.value()
                        << ") value: " << attribute.value.value();
        }
    }
}

void InitDeviceModelDb::delete_attribute(const DbVariableAttribute& attribute) {
    if (!attribute.db_id.has_value()) {
        EVLOG_error << "Could not remove attribute, because id is unknown.";
        return;
    }

    static const std::string delete_attribute_statement = "DELETE FROM VARIABLE_ATTRIBUTE WHERE ID=@attribute_id";

    std::unique_ptr<common::SQLiteStatementInterface> delete_statement;
    try {
        delete_statement = this->database->new_statement(delete_attribute_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + delete_attribute_statement);
    }

    delete_statement->bind_int("@attribute_id", static_cast<int>(attribute.db_id.value()));

    if (delete_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Can not remove attribute: " + std::string(this->database->get_error_message()));
    }
}

bool InitDeviceModelDb::insert_variable_attribute_value(const int64_t& variable_attribute_id,
                                                        const std::string& variable_attribute_value,
                                                        const bool warn_source_not_default) {
    // Insert variable statement.
    // Use 'IS' when value can also be NULL
    // Only update when VALUE_SOURCE is 'default', because otherwise it is already updated by the csms or the user and
    // we don't overwrite that.
    static const std::string statement = "UPDATE VARIABLE_ATTRIBUTE "
                                         "SET VALUE = @value, VALUE_SOURCE = 'default' "
                                         "WHERE ID = @variable_attribute_id "
                                         "AND (VALUE_SOURCE = 'default' OR VALUE_SOURCE = '' OR VALUE_SOURCE IS NULL)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_variable_attribute_statement;
    try {
        insert_variable_attribute_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    insert_variable_attribute_statement->bind_int("@variable_attribute_id",
                                                  static_cast<int32_t>(variable_attribute_id));
    insert_variable_attribute_statement->bind_text("@value", variable_attribute_value,
                                                   ocpp::common::SQLiteString::Transient);

    if (insert_variable_attribute_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not set value '" + variable_attribute_value +
                                     "' of variable attribute id " + std::to_string(variable_attribute_id) + ": " +
                                     std::string(this->database->get_error_message()));
    } else if ((insert_variable_attribute_statement->changes() < 1) && warn_source_not_default) {
        EVLOG_debug << "Could not set value '" + variable_attribute_value + "' of variable attribute id " +
                           std::to_string(variable_attribute_id) + ": value has already changed by other source";
    }

    return true;
}

std::map<ComponentKey, std::vector<DeviceModelVariable>> InitDeviceModelDb::get_all_components_from_db() {
    /* clang-format off */
    const std::string statement =
        "SELECT "
            "c.ID, c.NAME, c.INSTANCE, c.EVSE_ID, c.CONNECTOR_ID, "
            "v.ID, v.NAME, v.INSTANCE, v.REQUIRED, "
            "vc.ID, vc.DATATYPE_ID, vc.MAX_LIMIT, vc.MIN_LIMIT, vc.SUPPORTS_MONITORING, vc.UNIT, vc.VALUES_LIST, "
            "va.ID, va.MUTABILITY_ID, va.PERSISTENT, va.CONSTANT, va.TYPE_ID, va.VALUE, va.VALUE_SOURCE "
        "FROM "
            "COMPONENT c "
            "JOIN VARIABLE v ON v.COMPONENT_ID = c.ID "
            "JOIN VARIABLE_CHARACTERISTICS vc ON vc.VARIABLE_ID = v.ID "
            "JOIN VARIABLE_ATTRIBUTE va ON va.VARIABLE_ID = v.ID";
    /* clang-format on */

    std::unique_ptr<common::SQLiteStatementInterface> select_statement;
    try {
        select_statement = this->database->new_statement(statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + statement);
    }

    std::map<ComponentKey, std::vector<DeviceModelVariable>> components;

    int status;
    while ((status = select_statement->step()) == SQLITE_ROW) {
        ComponentKey component_key;
        component_key.db_id = select_statement->column_int(0);
        component_key.name = select_statement->column_text(1);
        component_key.instance = select_statement->column_text_nullable(2);
        if (select_statement->column_type(3) != SQLITE_NULL) {
            component_key.evse_id = select_statement->column_int(3);
        }

        if (select_statement->column_type(4) != SQLITE_NULL) {
            component_key.connector_id = select_statement->column_int(4);
        }

        bool variable_exists = false;
        DeviceModelVariable new_variable;
        new_variable.db_id = select_statement->column_int(5);
        new_variable.name = select_statement->column_text(6);
        new_variable.instance = select_statement->column_text_nullable(7);

        DeviceModelVariable* variable = nullptr;
        // Check if the variable is already added to the component. If it is, the attribute should be added to the
        // vector of attributes, otherwise the whole variable must be added to the variable vector of the component.
        const auto& it = components.find(component_key);
        if (it != components.end()) {
            // Component found. Now search for variables.
            std::vector<DeviceModelVariable>& variables = it->second;
            for (DeviceModelVariable& v : variables) {
                if (is_same_variable(v, new_variable)) {
                    // Variable found as well. Set variable pointer to the found variable.
                    variable = &v;
                    variable_exists = true;
                }
            }
        }

        if (!variable_exists) {
            // Variable does not exist, add extra information from database.
            if (select_statement->column_type(8) != SQLITE_NULL) {
                new_variable.required = (select_statement->column_int(8) == 1 ? true : false);
            }
            new_variable.variable_characteristics_db_id = select_statement->column_int(9);
            new_variable.characteristics.dataType = static_cast<DataEnum>(select_statement->column_int(10));
            if (select_statement->column_type(11) != SQLITE_NULL) {
                new_variable.characteristics.maxLimit = select_statement->column_double(11);
            }
            if (select_statement->column_type(12) != SQLITE_NULL) {
                new_variable.characteristics.minLimit = select_statement->column_double(12);
            }
            new_variable.characteristics.supportsMonitoring = (select_statement->column_int(13) == 1 ? true : false);
            new_variable.characteristics.unit = select_statement->column_text_nullable(14);
            new_variable.characteristics.valuesList = select_statement->column_text_nullable(15);

            // Variable is new, set variable pointer to this new variable.
            variable = &new_variable;
        }

        DbVariableAttribute attribute;
        attribute.db_id = select_statement->column_int(16);
        if (select_statement->column_type(17) != SQLITE_NULL) {
            attribute.variable_attribute.mutability = static_cast<MutabilityEnum>(select_statement->column_int(17));
        }
        if (select_statement->column_type(18) != SQLITE_NULL) {
            attribute.variable_attribute.persistent = (select_statement->column_int(18) == 1 ? true : false);
        }
        if (select_statement->column_type(19) != SQLITE_NULL) {
            attribute.variable_attribute.constant = (select_statement->column_int(19) == 1 ? true : false);
        }
        if (select_statement->column_type(20) != SQLITE_NULL) {
            attribute.variable_attribute.type = static_cast<AttributeEnum>(select_statement->column_int(20));
        }
        attribute.variable_attribute.value = select_statement->column_text_nullable(21);
        attribute.value_source = select_statement->column_text_nullable(22);

        variable->attributes.push_back(attribute);

        if (!variable_exists) {
            components[component_key].push_back(*variable);
        }
    }

    if (status != SQLITE_DONE) {
        throw InitDeviceModelDbError("Could not get all components from the database: " +
                                     std::string(this->database->get_error_message()));
    }

    return components;
}

std::optional<std::pair<ComponentKey, std::vector<DeviceModelVariable>>>
InitDeviceModelDb::component_exists_in_db(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& db_components,
                                          const ComponentKey& component) {
    for (const auto& db_component : db_components) {
        if (is_same_component_key(db_component.first, component)) {
            return db_component;
        }
    }

    return std::nullopt;
}

bool InitDeviceModelDb::component_exists_in_schemas(
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_schema, const ComponentKey& component) {
    for (const auto& component_in_schema : component_schema) {
        if (is_same_component_key(component, component_in_schema.first)) {
            return true;
        }
    }

    return false;
}

void InitDeviceModelDb::remove_not_existing_components_from_db(
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_config,
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& db_components) {
    for (const auto& component : db_components) {
        if (!component_exists_in_schemas(component_config, component.first)) {
            remove_component_from_db(component.first);
        }
    }
}

bool InitDeviceModelDb::remove_component_from_db(const ComponentKey& component) {
    const std::string& delete_component_statement = "DELETE FROM COMPONENT WHERE ID = @component_id";

    std::unique_ptr<common::SQLiteStatementInterface> delete_statement;
    try {
        delete_statement = this->database->new_statement(delete_component_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + delete_component_statement);
    }

    if (!component.db_id.has_value()) {
        EVLOG_error << "Can not delete component " << component.name << ": no id given";
        return false;
    }

    delete_statement->bind_int("@component_id", static_cast<int>(component.db_id.value()));

    if (delete_statement->step() != SQLITE_DONE) {
        throw InitDeviceModelDbError(this->database->get_error_message());
    }

    return true;
}

void InitDeviceModelDb::update_component_variables(
    const std::pair<ComponentKey, std::vector<DeviceModelVariable>>& db_component_variables,
    const std::vector<DeviceModelVariable>& variables) {
    const std::vector<DeviceModelVariable>& db_variables = db_component_variables.second;
    const ComponentKey& db_component = db_component_variables.first;

    if (!db_component.db_id.has_value()) {
        EVLOG_error << "Can not update component " << db_component.name << ", because database id is unknown.";
        return;
    }

    // Check for variables that do exist in the database but do not exist in the config. They should be removed.
    for (const DeviceModelVariable& db_variable : db_variables) {
        auto it = std::find_if(variables.begin(), variables.end(), [&db_variable](const DeviceModelVariable& variable) {
            return is_same_variable(variable, db_variable);
        });

        if (it == variables.end()) {
            // Variable from db does not exist in config, remove from db.
            delete_variable(db_variable);
        }
    }

    // Check for variables that do exist in the config. If they are not in the database, they should be added.
    // Otherwise, they should be updated.
    for (const DeviceModelVariable& variable : variables) {
        auto it =
            std::find_if(db_variables.begin(), db_variables.end(), [&variable](const DeviceModelVariable& db_variable) {
                return is_same_variable(db_variable, variable);
            });
        if (it == db_variables.end()) {
            // Variable does not exist in the db, add to db
            insert_variable(variable, db_component.db_id.value());
        } else {
            if (is_variable_different(*it, variable)) {
                // Update variable
                update_variable(variable, *it, db_component.db_id.value());
            }
        }
    }
}

std::vector<DbVariableAttribute> InitDeviceModelDb::get_variable_attributes_from_db(const uint64_t& variable_id) {
    std::vector<DbVariableAttribute> attributes;

    static const std::string get_attributes_statement = "SELECT ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID FROM "
                                                        "VARIABLE_ATTRIBUTE WHERE VARIABLE_ID=(@variable_id)";

    std::unique_ptr<common::SQLiteStatementInterface> select_statement;
    try {
        select_statement = this->database->new_statement(get_attributes_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + get_attributes_statement);
    }

    select_statement->bind_int("@variable_id", static_cast<int>(variable_id));

    int status;
    while ((status = select_statement->step()) == SQLITE_ROW) {
        DbVariableAttribute attribute;
        attribute.db_id = select_statement->column_int(0);
        if (select_statement->column_type(1) != SQLITE_NULL) {
            attribute.variable_attribute.mutability = static_cast<MutabilityEnum>(select_statement->column_int(1));
        }

        if (select_statement->column_type(2) != SQLITE_NULL) {
            attribute.variable_attribute.persistent = (select_statement->column_int(2) == 1 ? true : false);
        }

        if (select_statement->column_type(3) != SQLITE_NULL) {
            attribute.variable_attribute.constant = (select_statement->column_int(3) == 1 ? true : false);
        }

        if (select_statement->column_type(4) != SQLITE_NULL) {
            attribute.variable_attribute.type = static_cast<AttributeEnum>(select_statement->column_int(4));
        }

        attributes.push_back(attribute);
    }

    if (status != SQLITE_DONE) {
        throw InitDeviceModelDbError("Error while getting variable attributes from db: " +
                                     std::string(this->database->get_error_message()));
    }

    return attributes;
}

void InitDeviceModelDb::init_sql() {
    static const std::string foreign_keys_on_statement = "PRAGMA foreign_keys = ON;";

    std::unique_ptr<common::SQLiteStatementInterface> statement;
    try {
        statement = this->database->new_statement(foreign_keys_on_statement);
    } catch (const common::QueryExecutionException&) {
        throw InitDeviceModelDbError("Could not create statement " + foreign_keys_on_statement);
    }

    if (statement->step() != SQLITE_DONE) {
        const std::string error =
            "Could not enable foreign keys in sqlite database: " + std::string(this->database->get_error_message());
        EVLOG_error << error;
        throw InitDeviceModelDbError(error);
    }
}

bool operator<(const ComponentKey& l, const ComponentKey& r) {
    return std::tie(l.name, l.evse_id, l.connector_id, l.instance) <
           std::tie(r.name, r.evse_id, r.connector_id, r.instance);
}

void from_json(const json& j, ComponentKey& c) {
    c.name = j.at("name");

    if (j.contains("evse_id")) {
        c.evse_id = j.at("evse_id");
    }

    if (j.contains("connector_id")) {
        c.connector_id = j.at("connector_id");
    }

    if (j.contains("instance")) {
        c.instance = j.at("instance");
    }

    if (j.contains("required")) {
        json const& r = j.at("required");
        for (auto it = r.begin(); it != r.end(); ++it) {
            c.required.push_back(it->get<std::string>());
        }
    }
}

void from_json(const json& j, DeviceModelVariable& c) {
    c.name = j.at("variable_name");
    c.characteristics = j.at("characteristics");
    for (const auto& attribute : j.at("attributes").items()) {
        DbVariableAttribute va;
        va.variable_attribute = attribute.value();
        c.attributes.push_back(va);
    }

    if (j.contains("instance")) {
        c.instance = j.at("instance");
    }

    // Required is normally not in the schema here but somewhere else, but well, if it is occasionally or just later on,
    // it will be added here as well.
    if (j.contains("required")) {
        c.required = j.at("required");
    }

    if (j.contains("default")) {
        // I want the default value as string here as it is stored in the db as a string as well.
        const json& default_value = j.at("default");
        c.default_actual_value = get_string_value_from_json(default_value);
    }
}

///
/// \brief Check integrity of config.
///
/// This will do some checks if the config is correct, for example if all required attributes have a value.
///
/// \param component_configs    Read config from the file system.
///
static void check_integrity(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_configs) {
    for (const auto& [component_key, variables] : component_configs) {
        for (const DeviceModelVariable& variable : variables) {
            if (!variable.required) {
                // Variable is not required, move to next variable.
                continue;
            }

            if (variable.default_actual_value.has_value()) {
                // There is a default value set, so for this required variable, we have a value (maybe there is a
                // value set as well but since we also have a default value, we don't have to check that)
                continue;
            }

            const auto& actual_attribute = std::find_if(
                variable.attributes.begin(), variable.attributes.end(), [](const DbVariableAttribute& attribute) {
                    if (attribute.variable_attribute.type.has_value() &&
                        attribute.variable_attribute.type.value() == AttributeEnum::Actual) {
                        return true;
                    }
                    return false;
                });

            if (actual_attribute == variable.attributes.end()) {
                EVLOG_AND_THROW(InitDeviceModelDbError("Could not find required Actual attribute for variable " +
                                                       get_variable_name_for_logging(variable) + " of component " +
                                                       get_component_name_for_logging(component_key)));
            }

            if (!actual_attribute->variable_attribute.value.has_value()) {
                EVLOG_AND_THROW(InitDeviceModelDbError("No value set for Actual attribute for required variable " +
                                                       get_variable_name_for_logging(variable) + " of component " +
                                                       get_component_name_for_logging(component_key)));
            }
        }
    }
}

/* Below functions check if components, attributes, variables, characteristics are the same / equal in the schema
 * and database. The 'is_same' functions check if two objects are the same, comparing their unique properties. The
 * is_..._different functions check if the objects properties are different (and as a result should be changed in
 * the database).
 */

///
/// \brief Check if the component keys are the same given their unique properties (name, evse id, connector id and
///        instance)
/// \param component_key1   Component key 1
/// \param component_key2   Component key 2
/// \return True if those are the same components.
///
static bool is_same_component_key(const ComponentKey& component_key1, const ComponentKey& component_key2) {
    if ((component_key1.name == component_key2.name) && (component_key1.evse_id == component_key2.evse_id) &&
        (component_key1.connector_id == component_key2.connector_id) &&
        (component_key1.instance == component_key2.instance)) {
        // We did not compare the 'required' here as that does not define a ComponentKey
        return true;
    }

    return false;
}

///
/// \brief Check if the two given attributes are the same  given their unique properties (type)
/// \param attribute1   Attribute 1
/// \param attribute2   Attribute 2
/// \return True when they are the same.
///
static bool is_same_attribute_type(const VariableAttribute attribute1, const VariableAttribute& attribute2) {
    return attribute1.type == attribute2.type;
}

///
/// @brief Check if attribute characteristics are different.
///
/// Will not check for value but only the other characteristics.
///
/// @param attribute1    attribute 1.
/// @param attribute2    attribute 2.
/// @return True if characteristics of attribute are the same.
///
static bool is_attribute_different(const VariableAttribute& attribute1, const VariableAttribute& attribute2) {
    // Constant and persistent are currently not set in the json file.
    if ((attribute1.type == attribute2.type) && /*(attribute1.constant == attribute2.constant) &&*/
        (attribute1.mutability == attribute2.mutability) && (attribute1.value == attribute2.value)
        /* && (attribute1.persistent == attribute2.persistent)*/) {
        return false;
    }
    return true;
}

///
/// \brief Check if a variable has the same attributes, or if there is for example an extra attribute added, removed
/// or
///        changed.
/// \param attributes1 Attributes 1
/// \param attributes2 Attributes 2
/// \return True if they are the same.
///
static bool variable_has_same_attributes(const std::vector<DbVariableAttribute>& attributes1,
                                         const std::vector<DbVariableAttribute>& attributes2) {
    if (attributes1.size() != attributes2.size()) {
        return false;
    }

    for (const DbVariableAttribute& attribute : attributes1) {
        const auto& it =
            std::find_if(attributes2.begin(), attributes2.end(), [&attribute](const DbVariableAttribute& a) {
                if (!is_attribute_different(a.variable_attribute, attribute.variable_attribute)) {
                    return true;
                }
                return false;
            });

        if (it == attributes2.end()) {
            // At least one attribute is different.
            return false;
        }
    }

    // Everything is the same.
    return true;
}

///
/// \brief Check if the given characteristics are different from eachother.
/// \param c1   Characteristics 1
/// \param c2   Characteristics 2
/// \return True if they are different
///
static bool is_characteristics_different(const VariableCharacteristics& c1, const VariableCharacteristics& c2) {
    if ((c1.supportsMonitoring == c2.supportsMonitoring) && (c1.dataType == c2.dataType) &&
        (c1.maxLimit == c2.maxLimit) && (c1.minLimit == c2.minLimit) && (c1.unit == c2.unit) &&
        (c1.valuesList == c2.valuesList)) {
        return false;
    }
    return true;
}

///
/// \brief Check if the two variables are the same given their unique properties (name and instance).
/// \param v1   Variable 1
/// \param v2   Variable 2
/// \return True if they are the same variable.
///
static bool is_same_variable(const DeviceModelVariable& v1, const DeviceModelVariable& v2) {
    return ((v1.name == v2.name) && (v1.instance == v2.instance));
}

///
/// \brief Check if the two given variables are different from eachother.
/// \param v1   Variable 1
/// \param v2   Variable 2
/// \return True if they are different.
///
static bool is_variable_different(const DeviceModelVariable& v1, const DeviceModelVariable& v2) {
    if (is_same_variable(v1, v2) && (v1.required == v2.required) &&
        !is_characteristics_different(v1.characteristics, v2.characteristics) &&
        variable_has_same_attributes(v1.attributes, v2.attributes)) {
        return false;
    }
    return true;
}

///
/// \brief Get string value from json.
///
/// The json value can have different types, but we want it as a string.
///
/// \param value    The json value.
/// \return The string value.
///
static std::string get_string_value_from_json(const json& value) {
    if (value.is_string()) {
        return value;
    } else if (value.is_boolean()) {
        if (value.get<bool>()) {
            // Convert to lower case if that is not the case currently.
            return "true";
        }
        return "false";
    } else if (value.is_array() || value.is_object()) {
        EVLOG_warning << "String value " << value.dump()
                      << " from config is an object or array, but config values should be from a primitive type.";
        // TODO mz throw here or is this ok?
        return value.dump();
    } else {
        return value.dump();
    }
}

///
/// \brief Get a string that describes the component, used for logging.
///
/// This includes the name of the component, the instance, evse id and connector id.
///
/// \param component    The component to get the string from.
/// \return The logging string.
///
static std::string get_component_name_for_logging(const ComponentKey& component) {
    const std::string component_name =
        component.name + (component.instance.has_value() ? ", instance " + component.instance.value() : "") +
        (component.evse_id.has_value() ? ", evse " + std::to_string(component.evse_id.value()) : "") +
        (component.connector_id.has_value() ? ", connector " + std::to_string(component.connector_id.value()) : "");

    return component_name;
}

///
/// \brief Get a string that describes the variable, used for logging.
///
/// This includes the name and the instance of the variable
///
/// \param variable    The variable to get the string from.
/// \return The logging string.
///
static std::string get_variable_name_for_logging(const DeviceModelVariable& variable) {
    const std::string variable_name =
        variable.name + (variable.instance.has_value() ? ", instance" + variable.instance.value() : "");
    return variable_name;
}

} // namespace ocpp::v201
