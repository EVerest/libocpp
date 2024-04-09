// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ocpp/common/database/database_schema_updater.hpp>

using namespace ocpp;
using namespace ocpp::common;
using namespace std::string_literals;

class DatabaseMigrationFilesTest : public ::testing::TestWithParam<std::tuple<std::filesystem::path, uint32_t>> {

protected:
    std::shared_ptr<DatabaseConnectionInterface> database;
    const std::filesystem::path migration_files_path;
    const uint32_t max_version;

public:
    DatabaseMigrationFilesTest() :
        database(std::make_unique<DatabaseConnection>("file::memory:?cache=shared")),
        migration_files_path(std::get<std::filesystem::path>(GetParam())),
        max_version(std::get<uint32_t>(GetParam())) {
        EXPECT_EQ(this->database->open_connection(), true);
    }

    void ExpectUserVersion(uint32_t expected_version) {
        auto statement = this->database->new_statement("PRAGMA user_version");

        EXPECT_EQ(statement->step(), SQLITE_ROW);
        EXPECT_EQ(statement->column_int(0), expected_version);
    }

    bool DoesTableExist(std::string_view table) {
        return this->database->clear_table(table.data());
    }

    bool DoesColumnExist(std::string_view table, std::string_view column) {
        return this->database->execute_statement("SELECT "s + column.data() + " FROM " + table.data() + " LIMIT 1;");
    }
};
