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
const static std::map<std::string, uint8_t> DATATYPE_ENCODINGS = {
    { "string", 0 },
    { "decimal", 1 },
    { "integer", 2 },
    { "dateTime", 3 },
    { "boolean", 4 },
    { "OptionList", 5 },
    { "SequenceList", 6 },
    { "MemberList", 7 }
};

const static std::map<std::string, uint8_t> MUTABILITY_ENCODINGS = {
    { "ReadOnly", 0 },
    { "WriteOnly", 1 },
    { "ReadWrite", 2 },
};

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
                                     const std::filesystem::path& migration_files_path) :
    common::DatabaseHandlerCommon(std::make_unique<common::DatabaseConnection>(database_path), migration_files_path,
                                  DEVICE_MODEL_MIGRATION_FILE_VERSION),
    database_path(database_path) {
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

    std::map<ComponentKey, std::vector<DeviceModelVariable>> component_schemas =
        get_all_component_schemas(schemas_path);

    if (!insert_components(component_schemas)) {
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

bool InitDeviceModelDb::insert_components(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& components) {
    bool success = true;

    for (std::pair<ComponentKey, std::vector<DeviceModelVariable>> component : components) {
        if (!insert_component(component.first, component.second)) {
            // TODO did one of the functions not throw here? E.g. should I add a throw catch or just re throw???
            EVLOG_error << "Could not insert component" << component.first.name;
            success = false;
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

    insert_component_statement->bind_text("@name", component_key.name);
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
        insert_component_statement->bind_text("@instance", component_key.instance.value());
    } else {
        insert_component_statement->bind_null("@instance");
    }

    if (insert_component_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    const int64_t component_id = this->database->get_last_inserted_rowid();

    // Loop over the properties of this component.
    for (const DeviceModelVariable& variable : component_variables) {
        const std::string variable_name = variable.name;
        EVLOG_debug << "-- Inserting variable " << variable_name;
        std::string instance;
        if (variable.instance.has_value()) {
            instance = variable.instance.value();
        }

        // Add variable characteristic
        const int64_t variable_characteristics_id = insert_variable_characteristics(variable.characteristics);

        // Add variable
        const int64_t variable_id =
            insert_variable(variable_name, instance, component_id, variable_characteristics_id, variable.required);

        insert_attributes(variable.attributes, variable_id);
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

int64_t InitDeviceModelDb::insert_variable_characteristics(const DeviceModelVariableCharacteristics& characteristics) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_CHARACTERISTICS (DATATYPE_ID, MAX_LIMIT, MIN_LIMIT, SUPPORTS_MONITORING, "
        "UNIT, VALUES_LIST) VALUES (@datatype_id, @max_limit, @min_limit, @supports_monitoring, @unit, @values_list)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_characteristics_statement =
        this->database->new_statement(statement);

    const uint8_t data_type = characteristics.data_type_id;
    insert_characteristics_statement->bind_int("@datatype_id", data_type);

    const uint8_t supports_monitoring = (characteristics.supports_monitoring ? 1 : 0);
    insert_characteristics_statement->bind_int("@supports_monitoring", supports_monitoring);

    if (characteristics.unit.has_value()) {
        insert_characteristics_statement->bind_text("@unit", characteristics.unit.value());
    } else {
        insert_characteristics_statement->bind_null("@unit");
    }

    if (characteristics.values_list.has_value()) {
        insert_characteristics_statement->bind_text("@values_list", characteristics.values_list.value());
    } else {
        insert_characteristics_statement->bind_null("@values_list");
    }

    if (characteristics.max_limit.has_value()) {
        insert_characteristics_statement->bind_double("@max_limit", characteristics.max_limit.value());
    } else {
        insert_characteristics_statement->bind_null("@max_limit");
    }

    if (characteristics.min_limit.has_value()) {
        insert_characteristics_statement->bind_double("@min_limit", characteristics.min_limit.value());
    } else {
        insert_characteristics_statement->bind_null("@min_limit");
    }

    if (insert_characteristics_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    return this->database->get_last_inserted_rowid();
}

int64_t InitDeviceModelDb::insert_variable(const std::string& variable_name, const std::string& instance,
                                           const int64_t& component_id, const int64_t variable_characteristics_id,
                                           const bool required) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE (NAME, INSTANCE, COMPONENT_ID, VARIABLE_CHARACTERISTICS_ID, REQUIRED) VALUES "
        "(@name, @instance, @component_id, @variable_characteristics_id, @required)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_variable_statement =
        this->database->new_statement(statement);

    insert_variable_statement->bind_text("@name", variable_name);
    insert_variable_statement->bind_int("@component_id", component_id);
    insert_variable_statement->bind_int("@variable_characteristics_id", variable_characteristics_id);

    if (!instance.empty()) {
        insert_variable_statement->bind_text("@instance", instance);
    } else {
        insert_variable_statement->bind_null("@instance");
    }

    const uint8_t required_int = (required ? 1 : 0);
    insert_variable_statement->bind_int("@required", required_int);

    if (insert_variable_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }

    return this->database->get_last_inserted_rowid();
}

void InitDeviceModelDb::insert_attributes(const std::vector<DeviceModelVariableAttribute>& attributes,
                                          const int64_t& variable_id) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_ATTRIBUTE (VARIABLE_ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID) "
        "VALUES(@variable_id, @mutability_id, @persistent, @constant, @type_id)";
    for (const DeviceModelVariableAttribute& attribute : attributes) {
        std::unique_ptr<common::SQLiteStatementInterface> insert_attributes_statement =
            this->database->new_statement(statement);

        insert_attributes_statement->bind_int("@variable_id", variable_id);
        insert_attributes_statement->bind_int("@persistent", 1);
        insert_attributes_statement->bind_int("@constant", 0);

        if (attribute.mutability.has_value()) {
            insert_attributes_statement->bind_int("@mutability_id", attribute.mutability.value());
        } else {
            insert_attributes_statement->bind_null("@mutability_id");
        }

        insert_attributes_statement->bind_int("@type_id", attribute.type_id);

        if (insert_attributes_statement->step() != SQLITE_DONE) {
            throw common::QueryExecutionException(this->database->get_error_message());
        }
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

    insert_variable_attribute_statement->bind_text("@value", variable_attribute_key.value);
    insert_variable_attribute_statement->bind_text("@component_name", component_key.name);
    if (component_key.instance.has_value()) {
        insert_variable_attribute_statement->bind_text("@component_instance", component_key.instance.value());
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

    insert_variable_attribute_statement->bind_text("@variable_name", variable_attribute_key.name);

    if (variable_attribute_key.instance.has_value()) {
        insert_variable_attribute_statement->bind_text("@variable_instance", variable_attribute_key.instance.value());
    } else {
        insert_variable_attribute_statement->bind_null("@variable_instance");
    }

    insert_variable_attribute_statement->bind_int("@type_id", type_id);

    if (insert_variable_attribute_statement->step() != SQLITE_DONE) {
        throw common::QueryExecutionException(this->database->get_error_message());
    }
}

void InitDeviceModelDb::init_sql() {
    // TODO
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

void from_json(const json& j, DeviceModelVariableAttribute& c) {
    const auto& type_id_it = VARIABLE_ATTRIBUTE_TYPE_ENCODING.find(j.at("type"));
    if (type_id_it != VARIABLE_ATTRIBUTE_TYPE_ENCODING.end()) {
        c.type_id = type_id_it->second;
    }

    if (j.contains("mutability")) {
        const auto& mutability_it = MUTABILITY_ENCODINGS.find(j.at("mutability"));
        if (mutability_it != MUTABILITY_ENCODINGS.end()) {
            c.mutability = mutability_it->second;
        }
    }
}

void from_json(const json& j, DeviceModelVariableCharacteristics& c) {
    c.supports_monitoring = j.at("supportsMonitoring");
    const auto& data_type_it = DATATYPE_ENCODINGS.find(j.at("dataType"));

    if (data_type_it != DATATYPE_ENCODINGS.end()) {
        c.data_type_id = data_type_it->second;
    }

    if (j.contains("minLimit")) {
        c.min_limit = j.at("minLimit");
    }

    if (j.contains("maxLimit")) {
        c.max_limit = j.at("maxLimit");
    }

    if (j.contains("valuesList")) {
        c.values_list = j.at("valuesList");
    }

    if (j.contains("unit")) {
        c.unit = j.at("unit");
    }
}

void from_json(const json& j, DeviceModelVariable& c) {
    c.name = j.at("variable_name");
    c.characteristics = j.at("characteristics");
    c.attributes = j.at("attributes");

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
