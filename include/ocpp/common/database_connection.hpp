// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <sqlite3.h>

#include <ocpp/common/support_older_cpp_versions.hpp>

namespace ocpp {

class DatabaseConnection {
protected:
    sqlite3* db;
    const fs::path database_file_path;

public:
    DatabaseConnection(const fs::path& database_file_path) noexcept;

    virtual ~DatabaseConnection();

    /// \brief Opens the database connection.
    void open_connection();

    /// \brief Closes the database connection.
    void close_connection();

    /// \brief Start a transaction on the database.
    void begin_transaction();

    /// \brief Commits the transaction on the database.
    void commit_transaction();
};

} // namespace ocpp