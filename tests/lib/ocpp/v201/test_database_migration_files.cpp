// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ocpp/common/database/database_schema_updater.hpp>

using namespace ocpp;
using namespace ocpp::common;

class DatabaseMigrationFilesTest : public ::testing::Test {

protected:
    std::shared_ptr<DatabaseConnectionInterface> database;
    const std::filesystem::path migration_files_path;
    const uint32_t max_version;

public:
    DatabaseMigrationFilesTest() :
        database(std::make_unique<DatabaseConnection>("file::memory:?cache=shared")),
        migration_files_path(MIGRATION_FILES_LOCATION),
        max_version(MIGRATION_FILE_VERSION_V201) {
        EXPECT_EQ(this->database->open_connection(), true);
    }

    void ExpectUserVersion(uint32_t expected_version) {
        auto statement = this->database->new_statement("PRAGMA user_version");

        EXPECT_EQ(statement->step(), SQLITE_ROW);
        EXPECT_EQ(statement->column_int(0), expected_version);
    }
};

TEST_F(DatabaseMigrationFilesTest, ApplyMigrationFilesStepByStep) {
    DatabaseSchemaUpdater updater{this->database.get()};

    for (uint32_t i = 1; i <= this->max_version; i++) {
        EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, i), true);
        this->ExpectUserVersion(i);
    }

    for (uint32_t i = this->max_version; i > 0; i--) {
        EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, i), true);
        this->ExpectUserVersion(i);
    }
}

TEST_F(DatabaseMigrationFilesTest, ApplyMigrationFilesAtOnce) {
    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, this->max_version), true);
    this->ExpectUserVersion(this->max_version);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
}
