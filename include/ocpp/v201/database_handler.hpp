// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_V201_DATABASE_HANDLER_HPP
#define OCPP_V201_DATABASE_HANDLER_HPP

#include "sqlite3.h"
#include <filesystem>
#include <fstream>
#include <memory>

#include <ocpp/v201/ocpp_types.hpp>

#include <everest/logging.hpp>

namespace fs = std::filesystem;

namespace ocpp {
namespace v201 {

class DatabaseHandler {

private:
    sqlite3* db;
    fs::path database_file_path;
    fs::path sql_init_path;

    void sql_init();

public:
    DatabaseHandler(const fs::path& database_path, const fs::path& sql_init_path);
    ~DatabaseHandler();

    /// \brief Opens connection to database file
    void open_connection();

    /// \brief Closes connection to database file
    void close_connection();

    /// \brief
    /// \param id_token_hash
    /// \param id_token_info
    void insert_auth_cache_entry(const std::string& id_token_hash, const IdTokenInfo& id_token_info);

    /// \brief Gets cache entry for given \p id_token_hash if present
    /// \param id_token_hash
    /// \return
    boost::optional<IdTokenInfo> get_auth_cache_entry(const std::string& id_token_hash);
};

} // namespace v201
} // namespace ocpp

#endif