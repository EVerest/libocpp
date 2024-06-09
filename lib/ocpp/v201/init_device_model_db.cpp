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

    const std::vector<std::filesystem::path> standardized_component_schema_files =
        get_component_schemas_from_directory(schemas_path / STANDARDIZED_COMPONENT_SCHEMAS_DIR);
    const std::vector<std::filesystem::path> custom_component_schema_files =
        get_component_schemas_from_directory(schemas_path / CUSTOM_COMPONENT_SCHEMAS_DIR);

    if (!insert_components(standardized_component_schema_files, custom_component_schema_files)) {
        throw common::DatabaseException("Could not insert components in the database"); // TODO which exception?
    }
    // TODO

    return true;
}

bool InitDeviceModelDb::insert_config_and_default_values(const std::filesystem::path& schemas_path,
                                                         const std::filesystem::path& config_path) {
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

bool InitDeviceModelDb::insert_components(const std::vector<std::filesystem::path>& standardized_components,
                                          const std::vector<std::filesystem::path>& custom_components) {
    bool success = true;

    std::map<ComponentKey, json> standardized_components_map = read_component_schemas(standardized_components);
    std::map<ComponentKey, json> components = read_component_schemas(custom_components);

    // Merge the two maps so they can be used for the insert_component function with a single iterator. This will use
    // the custom components map as base and add not existing standardized components to the components map. So if the
    // component exists in both, the custom component will be used.
    components.merge(standardized_components_map);

    for (std::pair<ComponentKey, json> component : components) {
        if (!insert_component(component.first, component.second)) {
            // TODO did one of the functions not throw here? E.g. should I add a throw catch or just re throw???
            EVLOG_error << "Could not insert component" << component.first.name;
            success = false;
        }
    }

    return success;
}

bool InitDeviceModelDb::insert_component(const ComponentKey& component_key, const json& component_properties) {
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
    for (const auto& [property_key, variable_meta_data] : component_properties.items()) {
        const std::string variable_name = variable_meta_data.at("variable_name");
        EVLOG_debug << "-- Inserting variable " << variable_name;
        std::string instance;
        if (variable_meta_data.contains("instance")) {
            instance = variable_meta_data.at("instance");
        }
        const json& characteristics = variable_meta_data.at("characteristics");
        const json& attributes = variable_meta_data.at("attributes");

        // Add variable characteristic
        const int64_t variable_characteristics_id = insert_variable_characteristics(characteristics);

        const auto it =
            std::find(component_key.required_properties.begin(), component_key.required_properties.end(), property_key);
        bool required = false;
        if (it != component_key.required_properties.end()) {
            required = true;
        }

        // Add variable
        const int64_t variable_id =
            insert_variable(variable_name, instance, component_id, variable_characteristics_id, required);

        insert_attributes(attributes, variable_id);
    }

    return true;
}

std::map<ComponentKey, json>
InitDeviceModelDb::read_component_schemas(const std::vector<std::filesystem::path>& components_schema_path) {
    std::map<ComponentKey, json> components;
    for (const std::filesystem::path& path : components_schema_path) {
        std::ifstream schema_file(path);
        json data = json::parse(schema_file);
        ComponentKey p = data;
        if (data.contains("properties")) {
            components.insert({p, data.at("properties")});
        } else {
            EVLOG_warning << "Component " << data.at("name") << "does not contain any properties";
            continue;
        }
    }

    return components;
}

int64_t InitDeviceModelDb::insert_variable_characteristics(const json& characteristics) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_CHARACTERISTICS (DATATYPE_ID, MAX_LIMIT, MIN_LIMIT, SUPPORTS_MONITORING, "
        "UNIT, VALUES_LIST) VALUES (@datatype_id, @max_limit, @min_limit, @supports_monitoring, @unit, @values_list)";

    std::unique_ptr<common::SQLiteStatementInterface> insert_characteristics_statement =
        this->database->new_statement(statement);

    const std::string data_type = characteristics.at("dataType");
    const auto it = DATATYPE_ENCODINGS.find(data_type);
    uint8_t data_type_encoded = 0;
    if (it != DATATYPE_ENCODINGS.end()) {
        data_type_encoded = it->second;
        insert_characteristics_statement->bind_int("@datatype_id", data_type_encoded);
    } else {
        throw common::RequiredEntryNotFoundException("Could not find datatype " + data_type);
    }

    const uint8_t supports_monitoring = (characteristics.at("supportsMonitoring").get<bool>() ? 1 : 0);
    insert_characteristics_statement->bind_int("@supports_monitoring", supports_monitoring);

    if (characteristics.contains("unit")) {
        insert_characteristics_statement->bind_text("@unit", characteristics.at("unit"));
    } else {
        insert_characteristics_statement->bind_null("@unit");
    }

    if (characteristics.contains("valuesList")) {
        insert_characteristics_statement->bind_text("@values_list", characteristics.at("valuesList"));
    } else {
        insert_characteristics_statement->bind_null("@values_list");
    }

    if (characteristics.contains("maxLimit")) {
        insert_characteristics_statement->bind_int("@max_limit", characteristics.at("maxLimit"));
    } else {
        insert_characteristics_statement->bind_null("@max_limit");
    }

    if (characteristics.contains("minLimit")) {
        insert_characteristics_statement->bind_int("@min_limit", characteristics.at("minLimit"));
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

void InitDeviceModelDb::insert_attributes(const json& attributes, const int64_t& variable_id) {
    static const std::string statement =
        "INSERT OR REPLACE INTO VARIABLE_ATTRIBUTE (VARIABLE_ID, MUTABILITY_ID, PERSISTENT, CONSTANT, TYPE_ID) "
        "VALUES(@variable_id, @mutability_id, @persistent, @constant, @type_id)";
    for (const auto& attribute : attributes.items()) {
        std::string type;
        uint8_t type_encoded;
        if (attribute.value().contains("type")) {
            type = attribute.value().at("type");
            auto it = VARIABLE_ATTRIBUTE_TYPE_ENCODING.find(type);
            if (it == VARIABLE_ATTRIBUTE_TYPE_ENCODING.end()) {
                throw common::RequiredEntryNotFoundException("Could not find type " + type);
            }

            type_encoded = it->second;
        }

        std::string mutability;
        uint8_t mutability_encoded;
        if (attribute.value().contains("mutability")) {
            mutability = attribute.value().at("mutability");
            auto it = MUTABILITY_ENCODINGS.find(mutability);
            if (it == MUTABILITY_ENCODINGS.end()) {
                throw common::RequiredEntryNotFoundException("Could not find mutability " + mutability);
            }

            mutability_encoded = it->second;
        }

        std::unique_ptr<common::SQLiteStatementInterface> insert_attributes_statement =
            this->database->new_statement(statement);

        insert_attributes_statement->bind_int("@variable_id", variable_id);
        insert_attributes_statement->bind_int("@persistent", 1);
        insert_attributes_statement->bind_int("@constant", 0);

        if (!mutability.empty()) {
            insert_attributes_statement->bind_int("@mutability_id", mutability_encoded);
        } else {
            insert_attributes_statement->bind_null("@mutability_id");
        }

        if (!type.empty()) {
            insert_attributes_statement->bind_int("@type_id", type_encoded);
        } else {
            insert_attributes_statement->bind_null("@type_id");
        }

        if (insert_attributes_statement->step() != SQLITE_DONE) {
            throw common::QueryExecutionException(this->database->get_error_message());
        }
    }
}

void InitDeviceModelDb::init_sql() {
    // TODO
}

bool operator<(const ComponentKey& l, const ComponentKey& r) {
    return std::tie(l.name, l.evse_id, l.connector_id, l.instance) <
           std::tie(r.name, r.evse_id, r.connector_id, r.instance);
}

void to_json(json& j, const ComponentKey& c) {
    j = json{{"name", c.name}};

    // TODO can evse id and connector_id be 0??
    if (c.evse_id.has_value()) {
        j.at("evse_id") = c.evse_id.value();
    }

    if (c.connector_id.has_value()) {
        j.at("connector_id") = c.connector_id.value();
    }

    if (c.instance.has_value() && !c.instance->empty()) {
        j["instance"] = c.instance.value();
    }

    if (!c.required_properties.empty()) {
        for (const std::string& property : c.required_properties) {
            j["required"].push_back(property);
        }
    }
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
            c.required_properties.push_back(it->get<std::string>());
        }
    }
}

void to_json(json& j, const VariableAttribute& c) {
}

void from_json(const json& j, VariableAttribute& c) {
    c.value = j.at("value");
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

void to_json(json& j, const VariableCharacteristics& c) {
}

void from_json(const json& j, VariableCharacteristics& c) {
}

void to_json(json& j, const Variable& c) {
}

void from_json(const json& j, Variable& c) {
}

} // namespace ocpp::v201
