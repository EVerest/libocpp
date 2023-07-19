// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/database_handler.hpp>
#include <ocpp/common/message_queue.hpp>
#include <ocpp/v201/types.hpp>

namespace fs = std::filesystem;

namespace ocpp {
namespace v201 {

DatabaseHandler::DatabaseHandler(const fs::path& database_path, const fs::path& sql_init_path) :
    sql_init_path(sql_init_path) {
    if (!fs::exists(database_path)) {
        fs::create_directories(database_path);
    }
    this->database_file_path = database_path / "cp.db";
};

DatabaseHandler::~DatabaseHandler() {
    if (sqlite3_close_v2(this->db) == SQLITE_OK) {
        EVLOG_debug << "Successfully closed database file";
    } else {
        EVLOG_error << "Error closing database file: " << sqlite3_errmsg(this->db);
    }
}

void DatabaseHandler::sql_init() {
    EVLOG_debug << "Running SQL initialization script.";
    std::ifstream t(this->sql_init_path.string());
    std::stringstream init_sql;

    init_sql << t.rdbuf();

    char* err = NULL;

    if (sqlite3_exec(this->db, init_sql.str().c_str(), NULL, NULL, &err) != SQLITE_OK) {
        EVLOG_error << "Could not create tables: " << sqlite3_errmsg(this->db);
        throw std::runtime_error("Database access error");
    }
}

bool DatabaseHandler::clear_table(const std::string& table_name) {
    char* err_msg = 0;
    std::string sql = "DELETE FROM " + table_name + ";";
    if (sqlite3_exec(this->db, sql.c_str(), NULL, NULL, &err_msg) != SQLITE_OK) {
        return false;
    }
    return true;
}

void DatabaseHandler::open_connection() {
    if (sqlite3_open(this->database_file_path.c_str(), &this->db) != SQLITE_OK) {
        EVLOG_error << "Error opening database at " << this->database_file_path.c_str() << ": " << sqlite3_errmsg(db);
        throw std::runtime_error("Could not open database at provided path.");
    }
    EVLOG_debug << "Established connection to Database.";
    this->sql_init();
}

void DatabaseHandler::close_connection() {
    if (sqlite3_close(this->db) == SQLITE_OK) {
        EVLOG_debug << "Successfully closed database file";
    } else {
        EVLOG_error << "Error closing database file: " << sqlite3_errmsg(this->db);
    }
}

void DatabaseHandler::insert_auth_cache_entry(const std::string& id_token_hash, const IdTokenInfo& id_token_info) {
    std::string sql = "INSERT OR REPLACE INTO AUTH_CACHE (ID_TOKEN_HASH, ID_TOKEN_INFO) VALUES "
                      "(@id_token_hash, @id_token_info)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
        EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
        return;
    }

    const auto id_token_info_str = json(id_token_info).dump();
    sqlite3_bind_text(stmt, 1, id_token_hash.c_str(), id_token_hash.length(), NULL);
    sqlite3_bind_text(stmt, 2, id_token_info_str.c_str(), id_token_info_str.length(), NULL);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        EVLOG_error << "Could not insert into AUTH_CACHE table: " << sqlite3_errmsg(db);
        return;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        EVLOG_error << "Error inserting into AUTH_CACHE table: " << sqlite3_errmsg(this->db);
        return;
    }
}

std::optional<IdTokenInfo> DatabaseHandler::get_auth_cache_entry(const std::string& id_token_hash) {
    try {
        std::string sql = "SELECT ID_TOKEN_INFO FROM AUTH_CACHE WHERE ID_TOKEN_HASH = @id_token_hash";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            return std::nullopt;
        }

        sqlite3_bind_text(stmt, 1, id_token_hash.c_str(), id_token_hash.length(), NULL);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            return std::nullopt;
        }
        IdTokenInfo id_token_info =
            json::parse(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        return id_token_info;
    } catch (const json::exception& e) {
        EVLOG_warning << "Could not parse data of IdTokenInfo: " << e.what();
        return std::nullopt;
    } catch (const std::exception& e) {
        EVLOG_error << "Unknown Error while parsing IdTokenInfo: " << e.what();
        return std::nullopt;
    }
}

void DatabaseHandler::delete_auth_cache_entry(const std::string& id_token_hash) {
    try {
        std::string sql = "DELETE FROM AUTH_CACHE WHERE ID_TOKEN_HASH = @id_token_hash";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            return;
        }

        sqlite3_bind_text(stmt, 1, id_token_hash.c_str(), id_token_hash.length(), NULL);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            EVLOG_error << "Could not delete from table: " << sqlite3_errmsg(this->db);
        }
        if (sqlite3_finalize(stmt) != SQLITE_OK) {
            EVLOG_error << "Error deleting from table: " << sqlite3_errmsg(this->db);
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Exception while deleting from auth cache table: " << e.what();
    }
}

bool DatabaseHandler::clear_authorization_cache() {
    return this->clear_table("AUTH_CACHE");
}

void DatabaseHandler::insert_availability(const int32_t evse_id, std::optional<int32_t> connector_id,
                                          const OperationalStatusEnum& operational_status, const bool replace) {
    std::string sql = "INSERT OR REPLACE INTO AVAILABILITY (EVSE_ID, CONNECTOR_ID, OPERATIONAL_STATUS) VALUES "
                      "(@evse_id, @connector_id, @operational_status)";

    if (replace) {
        const std::string or_replace = "OR REPLACE";
        const std::string or_ignore = "OR IGNORE";
        size_t pos = sql.find(or_replace);
        sql.replace(pos, or_replace.length(), or_ignore);
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
        EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
        throw std::runtime_error("Error while inserting availability into database");
    }

    const auto operational_status_str = conversions::operational_status_enum_to_string(operational_status);
    sqlite3_bind_int(stmt, 1, evse_id);

    if (connector_id.has_value()) {
        sqlite3_bind_int(stmt, 2, connector_id.value());
    } else {
        sqlite3_bind_null(stmt, 2);
    }
    sqlite3_bind_text(stmt, 3, operational_status_str.c_str(), operational_status_str.length(), NULL);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        EVLOG_error << "Could not insert into AVAILABILITY table: " << sqlite3_errmsg(db);
        return;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        EVLOG_error << "Error inserting into AVAILABILITY table: " << sqlite3_errmsg(this->db);
        return;
    }
}

OperationalStatusEnum DatabaseHandler::get_availability(const int32_t evse_id, std::optional<int32_t> connector_id) {
    try {
        std::string sql =
            "SELECT OPERATIONAL_STATUS FROM AVAILABILITY WHERE EVSE_ID = @evse_id AND CONNECTOR_ID = @connector_id;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), sql.size(), &stmt, NULL) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            throw std::runtime_error("Could not get availability");
        }

        sqlite3_bind_int(stmt, 1, evse_id);
        if (connector_id.has_value()) {
            sqlite3_bind_int(stmt, 2, connector_id.value());
        } else {
            sqlite3_bind_null(stmt, 2);
        }

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            throw std::runtime_error("Could not get availability from database");
        }
        const auto operational_status = conversions::string_to_operational_status_enum(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        return operational_status;
    } catch (const std::exception& e) {
        throw std::runtime_error("Could not get availability from database");
    }
}

std::deque<ControlMessage<ocpp::v201::MessageType>> DatabaseHandler::get_transaction_messages()
{
    std::deque<ControlMessage<ocpp::v201::MessageType>> transaction_messages;

    try {
        std::string sql = "SELECT UNIQUE_ID, MESSAGE, MESSAGE_TYPE, MESSAGE_ATTEMPTS, MESSAGE_TIMESTAMP FROM TRANSACTION_QUEUE";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            throw std::runtime_error("Could not get transaction queue");
        }

        int status;
        while ((status = sqlite3_step(stmt)) == SQLITE_ROW) {
            try {
                const std::string message{reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))};
                const std::string unique_id{reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))};
                const std::string message_type{reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))};
                const std::string message_timestamp{reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))};
                const int message_attempts = sqlite3_column_int(stmt, 3);

                json json_message = json::parse(message);

                ControlMessage<ocpp::v201::MessageType> control_message{json_message};
                control_message.message_attempts = message_attempts;
                control_message.timestamp = message_timestamp;
                control_message.messageType = conversions::string_to_messagetype(message_type);
                transaction_messages.push_back(control_message);
            }  catch (const json::exception& e) {
                EVLOG_error << "json parse failed because: " << "(" << e.what() << ")";
            } catch (const std::exception& e) {
                // TODO
            }
        }

        if (status != SQLITE_DONE) {
            EVLOG_error << "Could not get (all) queued transaction messages from database";
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Could not get queued transaction messages from database: ") + e.what());
    }

    return transaction_messages;
}

void DatabaseHandler::insert_transaction_message(const ControlMessage<MessageType>& transaction_message) {
    const std::string sql =
        "INSERT INTO TRANSACTION_QUEUE (UNIQUE_ID, MESSAGE, MESSAGE_TYPE, MESSAGE_ATTEMPTS, MESSAGE_TIMESTAMP) VALUES "
        "(@unique_id, @message, @message_type, @message_attempts, @message_timestamp)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(this->db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
        EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
        throw std::runtime_error("Error while inserting queued transaction message into database");
    }

    const std::string unique_id = transaction_message.uniqueId();
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "unique_id"), unique_id.c_str(),
                      static_cast<int>(unique_id.size()), nullptr);

    const std::string message = transaction_message.message.at(0)
                                    .dump(); // TODO mz why is this an array and check if there is a message in it!!
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "message"), message.c_str(),
                      static_cast<int>(message.size()), nullptr);

    const std::string message_type_str = conversions::messagetype_to_string(transaction_message.messageType);
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "message_type"), message_type_str.c_str(),
                      static_cast<int>(message_type_str.size()), nullptr);

    sqlite3_bind_int(stmt, sqlite3_bind_parameter_index(stmt, "message_attempts"),
                     transaction_message.message_attempts);

    const std::string timestamp = transaction_message.timestamp.to_rfc3339();
    sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "message_timestamp"), timestamp.c_str(),
                      static_cast<int>(timestamp.size()), nullptr);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        EVLOG_error << "Could not insert into TRANSACTION_QUEUE table: " << sqlite3_errmsg(db);
        return;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        EVLOG_error << "Error inserting into TRANSACTION_QUEUE table: " << sqlite3_errmsg(this->db);
        return;
    }
}

void DatabaseHandler::remove_transaction_message(const std::string& unique_id) {
    try {
        std::string sql = "DELETE FROM TRANSACTION_QUEUE WHERE UNIQUE_ID = @unique_id";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(this->db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
            EVLOG_error << "Could not prepare insert statement: " << sqlite3_errmsg(this->db);
            return;
        }

        sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, "unique_id"), unique_id.c_str(),
                          static_cast<int>(unique_id.size()), nullptr);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            EVLOG_error << "Could not delete from table: " << sqlite3_errmsg(this->db);
        }
        if (sqlite3_finalize(stmt) != SQLITE_OK) {
            EVLOG_error << "Error deleting from table: " << sqlite3_errmsg(this->db);
        }
    } catch (const std::exception& e) {
        EVLOG_error << "Exception while deleting from transaction queue table: " << e.what();
    }
}

} // namespace v201
} // namespace ocpp
