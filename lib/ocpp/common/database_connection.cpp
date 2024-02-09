// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <ocpp/common/database_connection.hpp>

namespace ocpp {

DatabaseConnection::DatabaseConnection(const fs::path& database_file_path) noexcept :
    db(nullptr), database_file_path(database_file_path) {
}

DatabaseConnection::~DatabaseConnection() {
    close_connection();
}

void DatabaseConnection::open_connection() {
    if (sqlite3_open(this->database_file_path.c_str(), &this->db) != SQLITE_OK) {
        EVLOG_error << "Error opening database at " << this->database_file_path.c_str() << ": " << sqlite3_errmsg(db);
        throw std::runtime_error("Could not open database at provided path.");
    }
    EVLOG_debug << "Established connection to Database: " << this->database_file_path;
}

void DatabaseConnection::close_connection() {
    if (this->db != nullptr) {
        if (sqlite3_close_v2(this->db) == SQLITE_OK) {
            EVLOG_debug << "Successfully closed database: " << this->database_file_path;
            this->db = nullptr;
        } else {
            EVLOG_error << "Error closing database file: " << sqlite3_errmsg(this->db);
        }
    }
}

void DatabaseConnection::begin_transaction() {
    char* err_msg = nullptr;
    if (sqlite3_exec(this->db, "BEGIN TRANSACTION", NULL, NULL, &err_msg) != SQLITE_OK) {
        std::string error = "Could not begin transaction: ";
        error + err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error(error);
    }
}

void DatabaseConnection::commit_transaction() {
    char* err_msg = nullptr;
    if (sqlite3_exec(this->db, "COMMIT TRANSACTION", NULL, NULL, &err_msg) != SQLITE_OK) {
        std::string error = "Could not commit transaction: ";
        error + err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error(error);
    }
}

} // namespace ocpp