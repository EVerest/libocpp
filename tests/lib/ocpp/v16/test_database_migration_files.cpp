// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <lib/ocpp/common/test_database_migration_files.h>

// Apply generic test cases to v16 migrations
INSTANTIATE_TEST_SUITE_P(V16, DatabaseMigrationFilesTest,
                         ::testing::Values(std::make_tuple(std::filesystem::path(MIGRATION_FILES_LOCATION_V16),
                                                           MIGRATION_FILE_VERSION_V16)));

// Apply v16 specific test cases to migrations
using DatabaseMigrationFilesTestV16 = DatabaseMigrationFilesTest;

INSTANTIATE_TEST_SUITE_P(V16, DatabaseMigrationFilesTestV16,
                         ::testing::Values(std::make_tuple(std::filesystem::path(MIGRATION_FILES_LOCATION_V16),
                                                           MIGRATION_FILE_VERSION_V16)));

TEST_P(DatabaseMigrationFilesTestV16, V16_MigrationFile2) {
    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), true);
    this->ExpectUserVersion(2);

    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "STOP_TRANSACTION_MESSAGE_ID"), true);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);

    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "STOP_TRANSACTION_MESSAGE_ID"), false);
}