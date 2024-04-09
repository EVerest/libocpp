// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "test_database_migration_files.h"

TEST_P(DatabaseMigrationFilesTest, ApplyMigrationFilesStepByStep) {
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

TEST_P(DatabaseMigrationFilesTest, ApplyMigrationFilesAtOnce) {
    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, this->max_version), true);
    this->ExpectUserVersion(this->max_version);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
}