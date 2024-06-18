// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/init_device_model_db.hpp>

#include <cstdint>
#include <fstream>
#include <map>
#include <string>

#include <everest/logging.hpp>

const static std::string STANDARDIZED_COMPONENT_SCHEMAS_DIR = "standardized";
const static std::string CUSTOM_COMPONENT_SCHEMAS_DIR = "custom";

#define DEVICE_MODEL_MIGRATION_FILE_VERSION 1 // TODO get from cmake

namespace ocpp::v201 {
/* clang-format off */
const static std::map<std::string, uint8_t> VARIABLE_ATTRIBUTE_TYPE_ENCODING = {
    { "Actual", 0},
    { "Target", 1},
    { "MinSet", 2},
    { "MaxSet", 3}
};
/* clang-format on */

static bool is_same_component_key(const ComponentKey& component_key1, const ComponentKey& component_key2) {
    if ((component_key1.name == component_key2.name) && (component_key1.evse_id == component_key2.evse_id) &&
        (component_key1.connector_id == component_key2.connector_id) &&
        (component_key1.instance == component_key2.instance)) {
        // We did not compare the 'required' here as that does not define a ComponentKey
        return true;
    }

    return false;
}

static bool is_same_variable_attribute_key(const VariableAttributeKey& attribute_key1,
                                           const VariableAttributeKey& attribute_key2) {
    if ((attribute_key1.attribute_type == attribute_key2.attribute_type) &&
        (attribute_key1.instance == attribute_key2.instance) && (attribute_key1.name == attribute_key2.name)) {
        // We did not compare the 'value' here as we want to check if the attribute is the same and not the value of the
        // attribute.
        return true;
    }

    return false;
}

static bool is_same_attribute(const VariableAttribute attribute1, const VariableAttribute& attribute2) {
    return attribute1.type == attribute2.type;
}

/**
 * @brief Check if attribute characteristics are different.
 *
 * Will not check for value but only the other characteristics.
 *
 * @param attribute1    attribute 1.
 * @param attribute2    attribute 2.
 * @return True if characteristics of attribute are the same.
 */
static bool is_attribute_different(const VariableAttribute& attribute1, const VariableAttribute& attribute2) {
    // Constant and persistent are currently not set in the json file.
    if ((attribute1.type == attribute2.type) && /*(attribute1.constant == attribute2.constant) &&*/
        (attribute1.mutability == attribute2.mutability) /* && (attribute1.persistent == attribute2.persistent)*/) {
        return false;
    }
    return true;
}

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

static bool is_characteristics_different(const VariableCharacteristics& c1, const VariableCharacteristics& c2) {
    if ((c1.supportsMonitoring == c2.supportsMonitoring) && (c1.dataType == c2.dataType) &&
        (c1.maxLimit == c2.maxLimit) && (c1.minLimit == c2.minLimit) && (c1.unit == c2.unit) &&
        (c1.valuesList == c2.valuesList)) {
        return false;
    }
    return true;
}

static bool is_same_variable(const DeviceModelVariable& v1, const DeviceModelVariable& v2) {
    return ((v1.name == v2.name) && (v1.instance == v2.instance));
}

static bool is_variable_different(const DeviceModelVariable& v1, const DeviceModelVariable& v2) {
    if (is_same_variable(v1, v2) && (v1.required == v2.required) &&
        !is_characteristics_different(v1.characteristics, v2.characteristics) &&
        variable_has_same_attributes(v1.attributes, v2.attributes)) {
        return false;
    }
    return true;
}

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
        return "";
        // TODO : error!!
    } else {
        return value.dump();
    }
}

InitDeviceModelDb::InitDeviceModelDb(const std::filesystem::path& database_path,
                                     const std::filesystem::path& migration_files_path,
                                     DeviceModelStorage& device_model_storage) :
    common::DatabaseHandlerCommon(std::make_unique<common::DatabaseConnection>(database_path), migration_files_path,
                                  DEVICE_MODEL_MIGRATION_FILE_VERSION),
    database_path(database_path),
    database_exists(std::filesystem::exists(database_path)),
    device_model_storage(device_model_storage) {
}

InitDeviceModelDb::~InitDeviceModelDb() {
    close_connection();
}

bool InitDeviceModelDb::initialize_database(const std::filesystem::path& schemas_path,
                                            bool delete_db_if_exists = true) {
    if (!execute_init_sql(delete_db_if_exists)) {
        // TODO throw??
        return false;
    }

    // Get existing EVSE and Connector components from the database.
    std::vector<ComponentKey> existing_components;
    DeviceModelMap device_model;
    if (this->database_exists) {

        existing_components = get_all_connector_and_evse_components_fom_db();
        // device_model = device_model_storage.get_device_model();
    }

    // Get component schemas from the filesystem.
    std::map<ComponentKey, std::vector<DeviceModelVariable>> component_schemas =
        get_all_component_schemas(schemas_path);

    // Remove components from db if they do not exist in the component schemas
    if (this->database_exists) {
        remove_not_existing_components_from_db(component_schemas, existing_components);
    }

    if (!insert_components(component_schemas, existing_components)) {
        throw common::DatabaseException("Could not insert components in the database"); // TODO which exception?
    }
    // TODO

    return true;
}

bool InitDeviceModelDb::insert_config_and_default_values(const std::filesystem::path& schemas_path,
                                                         const std::filesystem::path& config_path) {
    std::map<ComponentKey, std::vector<VariableAttributeKey>> default_values =
        get_component_default_values(schemas_path);
    std::map<ComponentKey, std::vector<VariableAttributeKey>> config_values = get_config_values(config_path);

    for (const auto& component_variables : config_values) {
        for (const VariableAttributeKey& attribute_key : component_variables.second) {
            insert_variable_attribute_value(component_variables.first, attribute_key);
        }
    }

    for (const auto& component_variables : default_values) {
        // Compare with config_values if the value is already added. If it is, don't add the default value,
        // otherwise add the default value.
        const auto& it = std::find_if(
            config_values.begin(), config_values.end(),
            [&component_variables](const std::pair<ComponentKey, std::vector<VariableAttributeKey>> config_value) {
                if (is_same_component_key(config_value.first, component_variables.first)) {
                    return true;
                }

                return false;
            });
        bool component_found = true;
        std::vector<VariableAttributeKey> config_attribute_keys;
        if (it == config_values.end()) {
            // Not found, add all default values of this component.
            component_found = false;
        } else {
            config_attribute_keys = it->second;
        }

        for (const VariableAttributeKey& attribute_key : component_variables.second) {
            if (component_found) {
                auto attribute_key_it =
                    std::find_if(config_attribute_keys.begin(), config_attribute_keys.end(),
                                 [attribute_key](const VariableAttributeKey& config_attribute_key) {
                                     if (is_same_variable_attribute_key(attribute_key, config_attribute_key)) {
                                         return true;
                                     }
                                     return false;
                                 });

                if (attribute_key_it != config_attribute_keys.end()) {
                    // Attribute key is found in config, so we should not add a default value to the database.
                    continue;
                }
            }

            // Whole component is not found, or component is found but attribute is not found. Add default value to
            // database.
            insert_variable_attribute_value(component_variables.first, attribute_key);
        }
    }

    return true;
}

bool InitDeviceModelDb::execute_init_sql(const bool delete_db_if_exists) {
    if (delete_db_if_exists) {
        if (std::filesystem::exists(database_path)) {
            if (!std::filesystem::remove(database_path)) {
                EVLOG_error << "Could not remove database " << database_path.u8string();
                // TODO log or return false / throw?
                return false;
            }

            database_exists = false;
        }
    }

    // Connect to the database. This will automatically do the migrations (including the initial sql file).
    open_connection();

    return true;
}

std::vector<std::filesystem::path>
InitDeviceModelDb::get_component_schemas_from_directory(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> component_schema_files;
    for (const auto& p : std::filesystem::directory_iterator(directory)) {
        if (p.path().extension() == ".json") {
            component_schema_files.push_back(p.path());
        }
    }

    return component_schema_files;
}

std::map<ComponentKey, std::vector<DeviceModelVariable>>
InitDeviceModelDb::get_all_component_schemas(const std::filesystem::path& directory) {
    const std::vector<std::filesystem::path> standardized_component_schema_files =
        get_component_schemas_from_directory(directory / STANDARDIZED_COMPONENT_SCHEMAS_DIR);
    const std::vector<std::filesystem::path> custom_component_schema_files =
        get_component_schemas_from_directory(directory / CUSTOM_COMPONENT_SCHEMAS_DIR);

    std::map<ComponentKey, std::vector<DeviceModelVariable>> standardized_components_map =
        read_component_schemas(standardized_component_schema_files);
    std::map<ComponentKey, std::vector<DeviceModelVariable>> components =
        read_component_schemas(custom_component_schema_files);

    // Merge the two maps so they can be used for the insert_component function with a single iterator. This will use
    // the custom components map as base and add not existing standardized components to the components map. So if the
    // component exists in both, the custom component will be used.
    components.merge(standardized_components_map);

    return components;
}

bool InitDeviceModelDb::insert_components(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& components,
                                          const std::vector<ComponentKey>& existing_components) {
    bool success = true;

    for (std::pair<ComponentKey, std::vector<DeviceModelVariable>> component : components) {
        // Check if component already exists in the database.
        std::optional<ComponentKey> component_db;
        if (this->database_exists && (component.first.name == "EVSE" || component.first.name == "Connector") &&
            (component_db = component_exists_in_db(existing_components, component.first)).has_value()) {
            // Component exists in the database, update component if necessary.
            update_component(component_db.value(), component.first, component.second);
        } else if (!this->database_exists || (component.first.name == "EVSE" || component.first.name == "Connector")) {
            // Database is new or component is evse or connector and component does not exist. Insert component.
            if (!insert_component(component.first, component.second)) {
                // TODO did one of the functions not throw here? E.g. should I add a throw catch or just re throw???
                EVLOG_error << "Could not insert component" << component.first.name;
                success = false;
            }
        }
    }

    return success;
}

bool InitDeviceModelDb::insert_component(const ComponentKey& component_key,
                                         const std::vector<DeviceModelVariable> component_variables) {
    EVLOG_debug << "Inserting component " << component_key.name;

    static const std::string statement = "INSERT OR REPLACE INTO COMPONENT (NAME, INSTANCE, EVSE_ID, CONNECTOR_ID) "
                                         "VALUES (@name, @instance, @evse_id, @connector_id)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_component_statement =
        this->database->new_statement(statement);

    insert_component_statement->bind_text("@name", component_key.name, ocpp::common::SQLiteString::Transient);
    // TODO can a connector id or evse id be 0?
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
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    const int64_t component_id = this->database->get_last_inserted_rowid();

    // Loop over the properties of this component.
    for (const DeviceModelVariable& variable : component_variables) {
        EVLOG_debug << "-- Inserting variable " << variable.name;

        // Add variable
        insert_variable(variable, component_id);
    }

    return true;
}

std::map<ComponentKey, std::vector<DeviceModelVariable>>
InitDeviceModelDb::read_component_schemas(const std::vector<std::filesystem::path>& components_schema_path) {
    std::map<ComponentKey, std::vector<DeviceModelVariable>> components;
    for (const std::filesystem::path& path : components_schema_path) {
        std::ifstream schema_file(path);
        json data = json::parse(schema_file);
        ComponentKey p = data;
        if (data.contains("properties")) {
            std::vector<DeviceModelVariable> variables =
                get_all_component_properties(data.at("properties"), p.required);
            components.insert({p, variables});
        } else {
            EVLOG_warning << "Component " << data.at("name") << "does not contain any properties";
            continue;
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

    std::unique_ptr<common::SQLiteStatementInterface> insert_characteristics_statement =
        this->database->new_statement(statement);

    insert_characteristics_statement->bind_int("@datatype_id", static_cast<int>(characteristics.dataType));

    insert_characteristics_statement->bind_int("@variable_id", variable_id);

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
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::update_variable_characteristics(const VariableCharacteristics& characteristics,
                                                        const int64_t& characteristics_id, const int64_t& variable_id) {
    static const std::string update_characteristics_statement =
        "UPDATE VARIABLE_CHARACTERISTICS SET DATATYPE_ID=@datatype_id, VARIABLE_ID=@variable_id, MAX_LIMIT=@max_limit, "
        "MIN_LIMIT=@min_limit, SUPPORTS_MONITORING=@supports_monitoring, UNIT=@unit, VALUES_LIST=@values_list WHERE "
        "ID=@characteristics_id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement =
        this->database->new_statement(update_characteristics_statement);

    update_statement->bind_int("@datatype_id", static_cast<int>(characteristics.dataType));

    update_statement->bind_int("@characteristics_id", characteristics_id);
    update_statement->bind_int("@variable_id", variable_id);

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
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::insert_variable(const DeviceModelVariable& variable, const uint64_t& component_id) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE (NAME, INSTANCE, COMPONENT_ID, REQUIRED) VALUES "
        "(@name, @instance, @component_id, @required)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_variable_statement =
        this->database->new_statement(statement);

    insert_variable_statement->bind_text("@name", variable.name, ocpp::common::SQLiteString::Transient);
    insert_variable_statement->bind_int("@component_id", component_id);

    if (variable.instance.has_value() && !variable.instance.value().empty()) {
        insert_variable_statement->bind_text("@instance", variable.instance.value(),
                                             ocpp::common::SQLiteString::Transient);
    } else {
        insert_variable_statement->bind_null("@instance");
    }

    const uint8_t required_int = (variable.required ? 1 : 0);
    insert_variable_statement->bind_int("@required", required_int);

    if (insert_variable_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    const int64_t variable_id = this->database->get_last_inserted_rowid();

    insert_variable_characteristics(variable.characteristics, variable_id);
    insert_attributes(variable.attributes, variable_id);
}

void InitDeviceModelDb::update_variable(const DeviceModelVariable& variable, const DeviceModelVariable db_variable,
                                        const uint64_t component_id) {
    if (!db_variable.db_id.has_value()) {
        EVLOG_error << "Can not update variable " << variable.name << ": database id unknown";
        return;
    }

    static const std::string update_variable_statement =
        "UPDATE VARIABLE SET NAME=@name, INSTANCE=@instance, COMPONENT_ID=@component_id, REQUIRED=@required WHERE "
        "ID=@variable_id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement =
        this->database->new_statement(update_variable_statement);

    update_statement->bind_int("@variable_id", db_variable.db_id.value());
    update_statement->bind_text("@name", variable.name, ocpp::common::SQLiteString::Transient);
    update_statement->bind_int("@component_id", component_id);

    if (variable.instance.has_value() && !variable.instance.value().empty()) {
        update_statement->bind_text("@instance", variable.instance.value(), ocpp::common::SQLiteString::Transient);
    } else {
        update_statement->bind_null("@instance");
    }

    const uint8_t required_int = (variable.required ? 1 : 0);
    update_statement->bind_int("@required", required_int);

    if (update_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    if (db_variable.variable_characteristics_db_id.has_value() &&
        is_characteristics_different(variable.characteristics, db_variable.characteristics)) {
        update_variable_characteristics(variable.characteristics, db_variable.variable_characteristics_db_id.value(),
                                        db_variable.db_id.value());
    }

    if (!variable_has_same_attributes(variable.attributes, db_variable.attributes)) {
        update_attributes(variable.attributes, db_variable.attributes, db_variable.db_id.value());
    }
}

void InitDeviceModelDb::delete_variable(const DeviceModelVariable& variable) {
    if (!variable.db_id.has_value()) {
        EVLOG_error << "Can not remove variable " << variable.name << " from db: id unknown";
        return;
    }

    static const std::string delete_variable_statement = "DELETE FROM VARIABLE WHERE ID=@variable_id";

    std::unique_ptr<common::SQLiteStatementInterface> delete_statement =
        this->database->new_statement(delete_variable_statement);

    delete_statement->bind_int("@variable_id", variable.db_id.value());

    if (delete_statement->step() != SQLITE_DONE) {
        // TODO should we throw here?
        EVLOG_error << "Can not remove variable " << variable.name
                    << " from db: " << this->database->get_error_message();
        // throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::insert_attribute(const VariableAttribute& attribute, const uint64_t& variable_id) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_ATTRIBUTE (VARIABLE_ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID) "
        "VALUES(@variable_id, @mutability_id, @persistent, @constant, @type_id)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_attributes_statement =
        this->database->new_statement(statement);

    insert_attributes_statement->bind_int("@variable_id", variable_id);
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
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::insert_attributes(const std::vector<DbVariableAttribute>& attributes,
                                          const uint64_t& variable_id) {
    for (const DbVariableAttribute& attribute : attributes) {
        insert_attribute(attribute.variable_attribute, variable_id);
    }
}

void InitDeviceModelDb::update_attributes(const std::vector<DbVariableAttribute>& new_attributes,
                                          const std::vector<DbVariableAttribute>& db_attributes,
                                          const uint64_t& variable_id) {
    // First check if there are attributes in the database that are not in the config. They should be removed.
    for (const DbVariableAttribute& db_attribute : db_attributes) {
        const auto& it = std::find_if(
            new_attributes.begin(), new_attributes.end(), [&db_attribute](const DbVariableAttribute& new_attribute) {
                return is_same_attribute(db_attribute.variable_attribute, new_attribute.variable_attribute);
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
                return is_same_attribute(new_attribute.variable_attribute, db_attribute.variable_attribute);
            });

        if (it == db_attributes.end()) {
            // Variable attribute does not exist in the db, add to db.
            insert_attribute(new_attribute.variable_attribute, variable_id);
        } else {
            if (is_attribute_different(new_attribute.variable_attribute, it->variable_attribute)) {
                update_attribute(new_attribute.variable_attribute, *it);
            }
        }
    }
}

void InitDeviceModelDb::update_attribute(const VariableAttribute& attribute, const DbVariableAttribute& db_attribute) {
    if (!db_attribute.db_id.has_value()) {
        EVLOG_error << "Can not update attribute: id not found";
        return;
    }

    static const std::string update_attribute_statement =
        "UPDATE VARIABLE_ATTRIBUTE SET MUTABILITY_ID=@mutability_id, PERSISTENT=@persistent, CONSTANT=@constant, "
        "TYPE_ID=@type_id WHERE ID=@id";

    std::unique_ptr<common::SQLiteStatementInterface> update_statement =
        this->database->new_statement(update_attribute_statement);

    update_statement->bind_int("@id", db_attribute.db_id.value());

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
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::delete_attribute(const DbVariableAttribute& attribute) {
    if (!attribute.db_id.has_value()) {
        EVLOG_error << "Could not remove attribute, because id is unknown.";
        return;
    }

    static const std::string delete_attribute_statement = "DELETE FROM VARIABLE_ATTRIBUTE WHERE ID=@attribute_id";

    std::unique_ptr<common::SQLiteStatementInterface> delete_statement =
        this->database->new_statement(delete_attribute_statement);

    delete_statement->bind_int("@attribute_id", attribute.db_id.value());

    if (delete_statement->step() != SQLITE_DONE) {
        EVLOG_error << "Can not remove attribute from db: " << this->database->get_error_message();
        // TODO should we throw here?
    }
}

std::map<ComponentKey, std::vector<VariableAttributeKey>>
InitDeviceModelDb::get_component_default_values(const std::filesystem::path& schemas_path) {
    std::map<ComponentKey, std::vector<DeviceModelVariable>> components = get_all_component_schemas(schemas_path);

    std::map<ComponentKey, std::vector<VariableAttributeKey>> component_default_values;
    for (auto const& [componentKey, variables] : components) {
        std::vector<VariableAttributeKey> variable_attribute_keys;
        for (const DeviceModelVariable& variable : variables) {
            if (!variable.default_actual_value.empty()) {
                VariableAttributeKey key;
                key.name = variable.name;
                if (variable.instance.has_value()) {
                    key.instance = variable.instance.value();
                }
                key.attribute_type = "Actual";
                key.value = variable.default_actual_value;
                variable_attribute_keys.push_back(key);
            }
        }

        component_default_values.insert({componentKey, variable_attribute_keys});
    }

    return component_default_values;
}

std::map<ComponentKey, std::vector<VariableAttributeKey>>
InitDeviceModelDb::get_config_values(const std::filesystem::path& config_file_path) {
    std::map<ComponentKey, std::vector<VariableAttributeKey>> config_values;
    std::ifstream config_file(config_file_path);
    json config_json = json::parse(config_file);
    for (const auto& j : config_json.items()) {
        ComponentKey p = j.value();
        std::vector<VariableAttributeKey> attribute_keys;
        for (const auto& variable : j.value().at("variables").items()) {
            for (const auto& attributes : variable.value().at("attributes").items()) {
                VariableAttributeKey key;
                key.name = variable.value().at("variable_name");
                key.attribute_type = attributes.key();
                key.value = get_string_value_from_json(attributes.value());
                if (variable.value().contains("instance")) {
                    key.instance = variable.value().at("instance");
                }
                attribute_keys.push_back(key);
            }
        }

        config_values.insert({p, attribute_keys});
    }

    return config_values;
}

void InitDeviceModelDb::insert_variable_attribute_value(const ComponentKey& component_key,
                                                        const VariableAttributeKey& variable_attribute_key) {
    static const std::string statement = "UPDATE VARIABLE_ATTRIBUTE "
                                         "SET VALUE = @value "
                                         "WHERE VARIABLE_ID = ("
                                         "SELECT VARIABLE.ID "
                                         "FROM VARIABLE "
                                         "JOIN COMPONENT ON COMPONENT.ID = VARIABLE.COMPONENT_ID "
                                         "WHERE COMPONENT.NAME = @component_name "
                                         "AND COMPONENT.INSTANCE IS @component_instance "
                                         "AND COMPONENT.EVSE_ID IS @evse_id "
                                         "AND COMPONENT.CONNECTOR_ID IS @connector_id "
                                         "AND VARIABLE.NAME = @variable_name "
                                         "AND VARIABLE.INSTANCE IS @variable_instance) "
                                         "AND TYPE_ID = @type_id";

    uint8_t type_id;

    const auto& it = VARIABLE_ATTRIBUTE_TYPE_ENCODING.find(variable_attribute_key.attribute_type);
    if (it != VARIABLE_ATTRIBUTE_TYPE_ENCODING.end()) {
        type_id = it->second;
    } else {
        EVLOG_error << "Could not find type " << variable_attribute_key.attribute_type << " of component "
                    << component_key.name << " and variable " << variable_attribute_key.name;
        // TODO throw?? Or what to do here??? Just not add the whole thing?
        return;
    }

    std::unique_ptr<common::SQLiteStatementInterface> insert_variable_attribute_statement =
        this->database->new_statement(statement);

    insert_variable_attribute_statement->bind_text("@value", variable_attribute_key.value,
                                                   ocpp::common::SQLiteString::Transient);
    insert_variable_attribute_statement->bind_text("@component_name", component_key.name,
                                                   ocpp::common::SQLiteString::Transient);
    if (component_key.instance.has_value()) {
        insert_variable_attribute_statement->bind_text("@component_instance", component_key.instance.value(),
                                                       ocpp::common::SQLiteString::Transient);
    } else {
        insert_variable_attribute_statement->bind_null("@component_instance");
    }

    if (component_key.evse_id.has_value()) {
        insert_variable_attribute_statement->bind_int("@evse_id", component_key.evse_id.value());
    } else {
        insert_variable_attribute_statement->bind_null("@evse_id");
    }

    if (component_key.connector_id.has_value()) {
        insert_variable_attribute_statement->bind_int("@connector_id", component_key.connector_id.value());
    } else {
        insert_variable_attribute_statement->bind_null("@connector_id");
    }

    insert_variable_attribute_statement->bind_text("@variable_name", variable_attribute_key.name,
                                                   ocpp::common::SQLiteString::Transient);

    if (variable_attribute_key.instance.has_value()) {
        insert_variable_attribute_statement->bind_text("@variable_instance", variable_attribute_key.instance.value(),
                                                       ocpp::common::SQLiteString::Transient);
    } else {
        insert_variable_attribute_statement->bind_null("@variable_instance");
    }

    insert_variable_attribute_statement->bind_int("@type_id", type_id);

    if (insert_variable_attribute_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

std::vector<ComponentKey> InitDeviceModelDb::get_all_connector_and_evse_components_fom_db() {
    /* clang-format off */
    // const std::string statement =
    //     "SELECT "
    //         "c.ID as component_id, c.NAME as component_name, c.INSTANCE as component_instance, c.EVSE_ID as evse_id, "
    //         "c.CONNECTOR_ID as connector_id, "
    //         "v.ID as variable_id, v.NAME as variable_name, v.REQUIRED as required, "
    //         "vc.ID as characteristics_id, vc.DATATYPE_ID as characteristics_datatype, vc.MAX_LIMIT as max_limit, "
    //         "vc.MIN_LIMIT as min_limit, vc.SUPPORTS_MONITORING as supports_monitoring, vc.UNIT as unit, "
    //         "vc.VALUES_LIST as values_list, "
    //         "va.ID as attribute_id, va.MUTABILITY_ID as mutability, va.PERSISTENT as persistent, "
    //         "va.CONSTANT as constant, va.TYPE_ID as attribute_type, va.VALUE as value "
    //     "FROM "
    //         "COMPONENT c "
    //         "JOIN VARIABLE v ON v.COMPONENT_ID = c.ID "
    //         "JOIN VARIABLE_CHARACTERISTICS vc ON v.VARIABLE_CHARACTERISTICS_ID = vc.ID "
    //         "JOIN VARIABLE_ATTRIBUTE va ON va.VARIABLE_ID = v.ID WHERE c.NAME == 'EVSE' COLLATE NOCASE "
    //             "OR c.NAME == 'Connector' COLLATE NOCASE ";
    /* clang-format on */
    std::vector<ComponentKey> components;

    const std::string statement = "SELECT ID, NAME, INSTANCE, EVSE_ID, CONNECTOR_ID FROM COMPONENT "
                                  "WHERE NAME == 'EVSE' COLLATE NOCASE OR NAME == 'Connector' COLLATE NOCASE ";

    std::unique_ptr<common::SQLiteStatementInterface> select_statement = this->database->new_statement(statement);

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

        components.push_back(component_key);
    }

    if (status != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    return components;
}

std::optional<ComponentKey> InitDeviceModelDb::component_exists_in_db(const std::vector<ComponentKey>& db_components,
                                                                      const ComponentKey& component) {
    for (const ComponentKey& db_component : db_components) {
        if (is_same_component_key(db_component, component)) {
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
    const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_schemas,
    const std::vector<ComponentKey>& db_components) {
    for (const ComponentKey& component : db_components) {
        if (!component_exists_in_schemas(component_schemas, component)) {
            remove_component_from_db(component);
        }
    }
}

bool InitDeviceModelDb::remove_component_from_db(const ComponentKey& component) {
    const std::string& delete_component_statement = "DELETE FROM COMPONENT WHERE ID = @component_id";
    std::unique_ptr<common::SQLiteStatementInterface> delete_statement =
        this->database->new_statement(delete_component_statement);

    if (!component.db_id.has_value()) {
        EVLOG_error << "Can not delete component " << component.name << ": no id given";
        return false;
    }

    delete_statement->bind_int("@component_id", component.db_id.value());

    if (delete_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    return true;
}

void InitDeviceModelDb::update_component(const ComponentKey& db_component, const ComponentKey& config_component,
                                         const std::vector<DeviceModelVariable>& variables) {
    if (!db_component.db_id.has_value()) {
        EVLOG_error << "Can not update component " << db_component.name << ", because database id is unknown.";
        // TODO get component id here or just return???
        return;
    }

    const std::vector<DeviceModelVariable> db_variables = get_variables_from_component_from_db(db_component);

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

std::vector<DeviceModelVariable>
InitDeviceModelDb::get_variables_from_component_from_db(const ComponentKey& db_component) {
    if (!db_component.db_id.has_value()) {
        EVLOG_error << "Can not update component " << db_component.name << ", because database id is unknown.";
        // TODO get component id here or just return???
        return {};
    }

    std::vector<DeviceModelVariable> variables;

    static const std::string select_variable_statement =
        "SELECT v.ID, v.NAME, v.INSTANCE, v.REQUIRED, vc.ID, vc.DATATYPE_ID, vc.MAX_LIMIT, vc.MIN_LIMIT, "
        "vc.SUPPORTS_MONITORING, vc.UNIT, vc.VALUES_LIST FROM VARIABLE v LEFT JOIN VARIABLE_CHARACTERISTICS vc "
        "ON v.ID=vc.VARIABLE_ID WHERE v.COMPONENT_ID=@component_id";

    std::unique_ptr<common::SQLiteStatementInterface> select_statement =
        this->database->new_statement(select_variable_statement);
    select_statement->bind_int("@component_id", db_component.db_id.value());

    int status;
    while ((status = select_statement->step()) == SQLITE_ROW) {
        DeviceModelVariable variable;
        variable.db_id = select_statement->column_int(0);
        variable.name = select_statement->column_text(1);
        if (select_statement->column_type(2) != SQLITE_NULL) {
            variable.instance = select_statement->column_text(2);
        }
        variable.required = (select_statement->column_int(3) == 1 ? true : false);
        variable.variable_characteristics_db_id = select_statement->column_int(4);
        variable.characteristics.dataType = static_cast<DataEnum>(select_statement->column_int(5));
        if (select_statement->column_type(6) != SQLITE_NULL) {
            variable.characteristics.maxLimit = select_statement->column_double(6);
        }
        if (select_statement->column_type(7) != SQLITE_NULL) {
            variable.characteristics.minLimit = select_statement->column_double(7);
        }
        variable.characteristics.supportsMonitoring = (select_statement->column_int(8) == 1 ? true : false);
        if (select_statement->column_type(9) != SQLITE_NULL) {
            variable.characteristics.unit = select_statement->column_text(9);
        }
        if (select_statement->column_type(10) != SQLITE_NULL) {
            variable.characteristics.valuesList = select_statement->column_text(10);
        }

        variables.push_back(variable);
    }

    if (status != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    for (DeviceModelVariable& variable : variables) {
        std::vector<DbVariableAttribute> attributes = get_variable_attributes_from_db(variable.db_id.value());
        variable.attributes = attributes;
    }

    return variables;
}

std::vector<DbVariableAttribute> InitDeviceModelDb::get_variable_attributes_from_db(const uint64_t& variable_id) {
    std::vector<DbVariableAttribute> attributes;

    static const std::string get_attributes_statement = "SELECT ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID FROM "
                                                        "VARIABLE_ATTRIBUTE WHERE VARIABLE_ID=(@variable_id)";

    std::unique_ptr<common::SQLiteStatementInterface> select_statement =
        this->database->new_statement(get_attributes_statement);
    select_statement->bind_int("@variable_id", variable_id);

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
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    return attributes;
}

void InitDeviceModelDb::init_sql() {
    static const std::string foreign_keys_on_statement = "PRAGMA foreign_keys = ON;";
    std::unique_ptr<common::SQLiteStatementInterface> statement =
        this->database->new_statement(foreign_keys_on_statement);

    if (statement->step() != SQLITE_DONE) {
        EVLOG_error << "Could not enable foreign keys in sqlite database: " << this->database->get_error_message();
        throw common::QueryExecutionException(this->database->get_error_message());
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
    c.characteristics = j.at("characteristics"); // TODO fix warning
    for (const auto& attribute : j.at("attributes").items()) {
        DbVariableAttribute va;
        va.variable_attribute = attribute.value();
        c.attributes.push_back(va);
    }
    // c.attributes = j.at("attributes");

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
        if (default_value.is_string()) {
            c.default_actual_value = default_value;
        } else if (default_value.is_boolean()) {
            if (default_value.get<bool>()) {
                // Convert to lower case if that is not the case currently.
                c.default_actual_value = "true";
            } else {
                // Convert to lower case if that is not the case currently.
                c.default_actual_value = "false";
            }
        } else if (default_value.is_array() || default_value.is_object()) {
            // TODO : error!!
        } else {
            c.default_actual_value = default_value.dump();
        }
    }
}

} // namespace ocpp::v201
