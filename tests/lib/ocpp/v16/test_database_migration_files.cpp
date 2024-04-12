// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <lib/ocpp/common/test_database_migration_files.hpp>

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

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);

    // Transaction table should not contain these columns yet
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "START_TRANSACTION_MESSAGE_ID"), false);
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "STOP_TRANSACTION_MESSAGE_ID"), false);

    // We expect to be able to insert into CONNECTORS and TRANSACTIONS table.
    EXPECT_EQ(this->database->execute_statement("INSERT INTO CONNECTORS (ID, AVAILABILITY) VALUES (1, \"\")"), true);

    std::string sql =
        "INSERT INTO TRANSACTIONS "
        "(ID, CONNECTOR, ID_TAG_START, TIME_START, METER_START, CSMS_ACK, METER_LAST, METER_LAST_TIME, LAST_UPDATE)"
        " VALUES "
        "(55, 1,         \"\",         \"\",       1,           0,        0,          \"\",            \"\")";
    EXPECT_EQ(this->database->execute_statement(sql), true);

    // After applying the migration we expect to be at version 2
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), true);
    this->ExpectUserVersion(2);

    // We expect the added columns to exists
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "START_TRANSACTION_MESSAGE_ID"), true);
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "STOP_TRANSACTION_MESSAGE_ID"), true);

    // We should be able to update the transaction we inserted earlier
    EXPECT_EQ(this->database->execute_statement("UPDATE TRANSACTIONS SET METER_LAST=2 WHERE ID=55"), true);
    // We should be able to update the newly introduced field too
    EXPECT_EQ(
        this->database->execute_statement("UPDATE TRANSACTIONS SET START_TRANSACTION_MESSAGE_ID=\"test2\" WHERE ID=55"),
        true);

    // After applying the down migration we expect to be at version 1
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);

    // We expect the added columns to no longer exists
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "START_TRANSACTION_MESSAGE_ID"), false);
    EXPECT_EQ(this->DoesColumnExist("TRANSACTIONS", "STOP_TRANSACTION_MESSAGE_ID"), false);

    // We should still be able to update the transaction we inserted earlier
    EXPECT_EQ(this->database->execute_statement("UPDATE TRANSACTIONS SET METER_LAST=2 WHERE ID=55"), true);
    // We should not be able to update the field from version 2 any longer
    EXPECT_EQ(
        this->database->execute_statement("UPDATE TRANSACTIONS SET START_TRANSACTION_MESSAGE_ID=\"test2\" WHERE ID=55"),
        false);
}