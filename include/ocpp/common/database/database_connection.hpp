// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <sqlite3.h>

#include <ocpp/common/support_older_cpp_versions.hpp>

#include "sqlite_statement.hpp"

namespace ocpp::common {

class DatabaseConnectionInterface {
public:
    virtual ~DatabaseConnectionInterface() = default;

    /// \brief Opens the database connection.
    virtual void open_connection() = 0;

    /// \brief Closes the database connection.
    virtual void close_connection() = 0;

    /// \brief Start a transaction on the database.
    virtual void begin_transaction() = 0;

    /// \brief Commits the transaction on the database.
    virtual void commit_transaction() = 0;

    /// \brief Rolls back the transaction on the database.
    virtual void rollback_transaction() = 0;

    virtual bool execute_statement(const std::string& statement) = 0;

    virtual std::unique_ptr<SQLiteStatementInterface> new_statement(const std::string &sql) = 0;

    virtual const char* get_error_message() = 0;

    virtual bool clear_table(const std::string &table) = 0;

    virtual int64_t get_last_inserted_rowid() = 0;
};

class DatabaseConnection : public DatabaseConnectionInterface {
private:
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

    void rollback_transaction();

    bool execute_statement(const std::string& statement);

    std::unique_ptr<SQLiteStatementInterface> new_statement(const std::string &sql);

    const char* get_error_message();

    bool clear_table(const std::string &table);

    int64_t get_last_inserted_rowid();
};

} // namespace ocpp::common