// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <filesystem>

#include <ocpp/common/database/database_handler_common.hpp>

namespace ocpp::v201 {

struct ComponentKey {
    std::string name;
    std::optional<std::string> instance;
    std::optional<int32_t> evse_id;
    std::optional<int32_t> connector_id;
    std::vector<std::string> required;

    friend bool operator<(const ComponentKey& l, const ComponentKey& r);
};

struct DeviceModelVariableAttribute {
    uint8_t type_id;
    std::optional<uint8_t> mutability;
    std::string value;
};

struct DeviceModelVariableCharacteristics {
    bool supports_monitoring;
    uint8_t data_type_id;
    std::optional<std::string> values_list;
    std::optional<double> min_limit;
    std::optional<double> max_limit;
    std::optional<std::string> unit;
};

struct DeviceModelVariable {
    std::string name;
    DeviceModelVariableCharacteristics characteristics;
    std::vector<DeviceModelVariableAttribute> attributes;
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

void from_json(const json& j, DeviceModelVariableAttribute& c);

void from_json(const json& j, DeviceModelVariableCharacteristics& c);

void from_json(const json& j, DeviceModelVariable& c);

class InitDeviceModelDb : public common::DatabaseHandlerCommon {
private: // Members
    const std::filesystem::path database_path;

public:
    ///
    /// \brief Constructor.
    /// \param database_path        Path to the database.
    /// \param migration_files_path Path to the migration files.
    ///
    InitDeviceModelDb(const std::filesystem::path& database_path, const std::filesystem::path& migration_files_path);

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
    bool insert_components(const std::map<ComponentKey, std::vector<DeviceModelVariable>>& components);

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
    int64_t insert_variable_characteristics(const DeviceModelVariableCharacteristics& characteristics);

    int64_t insert_variable(const std::string& variable_name, const std::string& instance, const int64_t& component_id,
                            const int64_t variable_characteristics_id, const bool required);

    void insert_attributes(const std::vector<DeviceModelVariableAttribute>& attributes, const int64_t& variable_id);

    std::map<ComponentKey, std::vector<VariableAttributeKey>>
    get_component_default_values(const std::filesystem::path& schemas_path);
    std::map<ComponentKey, std::vector<VariableAttributeKey>>
    get_config_values(const std::filesystem::path& config_file_path);
    void insert_variable_attribute_value(const ComponentKey& component_key,
                                         const VariableAttributeKey& variable_attribute_key);

    // DatabaseHandlerCommon interface
protected: // Functions
    virtual void init_sql() override;
};
} // namespace ocpp::v201
