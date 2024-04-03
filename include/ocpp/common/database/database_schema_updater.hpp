// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/common/database/database_connection.hpp>
#include <ocpp/common/support_older_cpp_versions.hpp>

namespace ocpp::common {

class DatabaseSchemaUpdater {
protected:
    std::shared_ptr<DatabaseConnectionInterface> database;

public:
    explicit DatabaseSchemaUpdater(std::shared_ptr<DatabaseConnectionInterface> database) noexcept;

    bool apply_migration_files(const fs::path& migration_file_directory, uint32_t target_version);

private:
    uint32_t get_user_version();
    void set_user_version(uint32_t version);
};

} // namespace ocpp