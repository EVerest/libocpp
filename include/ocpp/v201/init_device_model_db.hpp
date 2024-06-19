// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <filesystem>

#include <ocpp/common/database/database_handler_common.hpp>
#include <ocpp/v201/device_model_storage.hpp>

namespace ocpp::v201 {

struct ComponentKey {
    std::optional<uint64_t> db_id;
    std::string name;
    std::optional<std::string> instance;
    std::optional<int32_t> evse_id;
    std::optional<int32_t> connector_id;
    std::vector<std::string> required;

    friend bool operator<(const ComponentKey& l, const ComponentKey& r);
};

struct DbVariableAttribute {
    std::optional<uint64_t> db_id;
    VariableAttribute variable_attribute;
};

struct DeviceModelVariable {
    std::optional<uint64_t> db_id;
    std::optional<uint64_t> variable_characteristics_db_id;
    std::string name;
    VariableCharacteristics characteristics;
    std::vector<DbVariableAttribute> attributes;
    bool required;
    std::optional<std::string> instance;
    std::string default_actual_value;
};

struct VariableAttributeKey {
    std::string name;
    std::optional<std::string> instance;
    std::string attribute_type;
    std::string value;
};

void from_json(const json& j, ComponentKey& c);

void from_json(const json& j, DeviceModelVariable& c);

class InitDeviceModelDb : public common::DatabaseHandlerCommon {
private: // Members
    const std::filesystem::path database_path;
    bool database_exists;
    DeviceModelStorage& device_model_storage;

public:
    ///
    /// \brief Constructor.
    /// \param database_path        Path to the database.
    /// \param migration_files_path Path to the migration files.
    ///
    InitDeviceModelDb(const std::filesystem::path& database_path, const std::filesystem::path& migration_files_path,
                      DeviceModelStorage& device_model_storage, const bool database_exists);

    virtual ~InitDeviceModelDb();

    ///
    /// \brief Initialize the database schema.
    /// \param schemas_path         Path to the database schemas.
    /// \param delete_db_if_exists  Set to true to delete the database if it already exists.
    /// \return True on successful initialization.
    ///
    bool initialize_database(const std::filesystem::path& schemas_path, const bool delete_db_if_exists);

    ///
    /// \brief Insert database configuration and default values.
    /// \param schemas_path Path to the database schemas.
    /// \param config_path  Path to the database config.
    /// \return True on success.
    ///
    bool insert_config_and_default_values(const std::filesystem::path& schemas_path,
                                          const std::filesystem::path& config_path);

private: // Functions
    bool execute_init_sql(const bool delete_db_if_exists);
    std::vector<std::filesystem::path> get_component_schemas_from_directory(const std::filesystem::path& directory);
    std::map<ComponentKey, std::vector<DeviceModelVariable>>
    get_all_component_schemas(const std::filesystem::path& directory);
    bool insert_components(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& components,
                           const std::vector<ComponentKey>& existing_components);

    bool insert_component(const ComponentKey& component_key,
                          const std::vector<DeviceModelVariable> component_variables);
    std::map<ComponentKey, std::vector<DeviceModelVariable>>
    read_component_schemas(const std::vector<std::filesystem::path>& components_schema_path);

    std::vector<DeviceModelVariable> get_all_component_properties(const json& component_properties,
                                                                  std::vector<std::string> required_properties);

    ///
    /// \brief Insert variable characteristics
    /// \param characteristics  The characteristics.
    /// \return The row id of the newly inserted characteristic.
    /// \throws common::RequiredEntryNotFoundException if dataType is not found / not valid.
    /// \throws common::QueryExecutionException if row could not be added to db.
    ///
    void insert_variable_characteristics(const VariableCharacteristics& characteristics, const int64_t& variable_id);
    void update_variable_characteristics(const VariableCharacteristics& characteristics,
                                         const int64_t& characteristics_id, const int64_t& variable_id);

    void insert_variable(const DeviceModelVariable& variable, const uint64_t& component_id);
    void update_variable(const DeviceModelVariable& variable, const DeviceModelVariable db_variable,
                         const uint64_t component_id);
    void delete_variable(const DeviceModelVariable& variable);

    void insert_attribute(const VariableAttribute& attribute, const uint64_t& variable_id);
    void insert_attributes(const std::vector<DbVariableAttribute>& attributes, const uint64_t& variable_id);
    void update_attributes(const std::vector<DbVariableAttribute>& new_attributes,
                           const std::vector<DbVariableAttribute>& db_attributes, const uint64_t& variable_id);
    void update_attribute(const VariableAttribute& attribute, const DbVariableAttribute& db_attribute);
    void delete_attribute(const DbVariableAttribute& attribute);

    std::map<ComponentKey, std::vector<VariableAttributeKey>>
    get_component_default_values(const std::filesystem::path& schemas_path);
    std::map<ComponentKey, std::vector<VariableAttributeKey>>
    get_config_values(const std::filesystem::path& config_file_path);
    void insert_variable_attribute_value(const ComponentKey& component_key,
                                         const VariableAttributeKey& variable_attribute_key);
    std::vector<ComponentKey> get_all_connector_and_evse_components_fom_db();
    std::optional<ComponentKey> component_exists_in_db(const std::vector<ComponentKey>& db_components,
                                                       const ComponentKey& component);
    bool component_exists_in_schemas(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_schema,
                                     const ComponentKey& component);

    /**
     * @brief Remove components from db that do not exist in the component schemas.
     * @param component_schemas The component schemas.
     * @param db_components     The components in the database.
     */
    void remove_not_existing_components_from_db(
        const std::map<ComponentKey, std::vector<DeviceModelVariable>>& component_schemas,
        const std::vector<ComponentKey>& db_components);
    bool remove_component_from_db(const ComponentKey& component);

    void update_component(const ComponentKey& db_component, const ComponentKey& config_component,
                          const std::vector<DeviceModelVariable>& variables);
    std::vector<DeviceModelVariable> get_variables_from_component_from_db(const ComponentKey& db_component);
    std::vector<DbVariableAttribute> get_variable_attributes_from_db(const uint64_t& variable_id);

protected: // Functions
    // DatabaseHandlerCommon interface
    virtual void init_sql() override;
};
} // namespace ocpp::v201
