// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <filesystem>

namespace ocpp::v201 {
class InitDeviceModelDb
{
private:    // Members
    const std::filesystem::path database_path;
public:
    ///
    /// \brief Constructor.
    /// \param database_path    Path to the database.
    ///
    InitDeviceModelDb(const std::filesystem::path& database_path);

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
    bool insert_config_and_default_values(const std::filesystem::path& schemas_path, const std::filesystem::path& config_path);
private:    // Functions
    bool execute_init_sql(const bool delete_db_if_exists);
};
}   // namespace ocpp::v201
