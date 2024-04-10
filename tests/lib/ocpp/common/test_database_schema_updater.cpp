// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "database_testing_utils.hpp"
#include <fstream>
#include <ocpp/common/database/database_schema_updater.hpp>

struct MigrationFile {
    std::string_view name;
    std::string_view content;
};

static constexpr MigrationFile migration_file_up_1_valid{
    "1_up-initial.sql",
    "PRAGMA foreign_keys = ON; CREATE TABLE TEST_TABLE1(FIELD1 TEXT PRIMARY KEY NOT NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_up_1_valid_empty_name{
    "1_up.sql",
    "PRAGMA foreign_keys = ON; CREATE TABLE TEST_TABLE1(FIELD1 TEXT PRIMARY KEY NOT NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_up_1_invalid{
    "1_up-initial.sql", "PRAGMA foreign_keys = ON; CREATE TABLE <invalid> TEST_TABLE1(FIELD1 TEXT PRIMARY KEY NOT "
                        "NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_up_2_valid{
    "2_up-add_table.sql", "CREATE TABLE TEST_TABLE2(FIELD1 TEXT PRIMARY KEY NOT NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_down_2_valid{"2_down-drop_table.sql", "DROP TABLE TEST_TABLE2;"};

static constexpr MigrationFile migration_file_up_3_valid{
    "3_up-add_table.sql", "CREATE TABLE TEST_TABLE3(FIELD1 TEXT PRIMARY KEY NOT NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_down_3_valid{"3_down-drop_table.sql", "DROP TABLE TEST_TABLE3;"};

static constexpr MigrationFile migration_file_up_4_valid{
    "4_up-add_table.sql", "CREATE TABLE TEST_TABLE4(FIELD1 TEXT PRIMARY KEY NOT NULL, FIELD2 INT NOT NULL);"};

static constexpr MigrationFile migration_file_down_4_valid{"4_down-drop_table.sql", "DROP TABLE TEST_TABLE4;"};

static constexpr std::string_view table1{"TEST_TABLE1"};
static constexpr std::string_view table2{"TEST_TABLE2"};
static constexpr std::string_view table3{"TEST_TABLE3"};
static constexpr std::string_view table4{"TEST_TABLE3"};

class DatabaseSchemaUpdaterTest : public DatabaseTestingUtils {

protected:
    std::filesystem::path migration_files_path;

public:
    DatabaseSchemaUpdaterTest() :
        DatabaseTestingUtils(),
        migration_files_path(std::filesystem::temp_directory_path() / "database_schema_test" / "core_migrations") {
        std::filesystem::create_directories(migration_files_path);
        EXPECT_EQ(this->database->open_connection(), true);
    }

    ~DatabaseSchemaUpdaterTest() {
        std::filesystem::remove_all(migration_files_path);
    }

    void WriteMigrationFile(const MigrationFile& file) {
        std::ofstream stream{this->migration_files_path / file.name};
        stream << file.content;
    }
};

TEST_F(DatabaseSchemaUpdaterTest, FolderDoesNotExist) {
    DatabaseSchemaUpdater updater{this->database.get()};
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path / "invalid", 1), false);
}

TEST_F(DatabaseSchemaUpdaterTest, TargetVersionInvalid) {
    DatabaseSchemaUpdater updater{this->database.get()};
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 0), false);
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyInitialMigrationFile) {

    this->WriteMigrationFile(migration_file_up_1_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);

    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyInitialMigrationFileEmptyName) {

    this->WriteMigrationFile(migration_file_up_1_valid_empty_name);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);

    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyInitialMigrationFileAlreadyUpToDate) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->SetUserVersion(1);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);

    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyInitialMigrationFileVersionToHigh) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->SetUserVersion(2);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);

    this->ExpectUserVersion(2);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyInvalidInitialMigrationFile) {

    this->WriteMigrationFile(migration_file_up_1_invalid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, MissingInitialMigrationFile) {

    this->WriteMigrationFile(migration_file_up_2_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false);
}

TEST_F(DatabaseSchemaUpdaterTest, SequenceNotValidUnevenNrOfFiles) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_up_3_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, SequenceNotValidNotEnoughFiles) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_down_2_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, SequenceNotValidMissingDownFile) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_up_3_valid);
    this->WriteMigrationFile(migration_file_up_4_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, SequenceNotValidMissingUpFile) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_down_2_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);
    this->WriteMigrationFile(migration_file_down_4_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), false);
    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), false);

    this->ExpectUserVersion(0);
    EXPECT_EQ(this->DoesTableExist(table1), false); // Database was not changed
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyMultipleMigrationFilesStepByStep) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_up_3_valid);
    this->WriteMigrationFile(migration_file_down_2_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), false);
    EXPECT_EQ(this->DoesTableExist(table3), false);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), true);
    this->ExpectUserVersion(2);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), true);
    EXPECT_EQ(this->DoesTableExist(table3), false);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), true);
    this->ExpectUserVersion(3);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), true);
    EXPECT_EQ(this->DoesTableExist(table3), true);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 2), true);
    this->ExpectUserVersion(2);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), true);
    EXPECT_EQ(this->DoesTableExist(table3), false);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), false);
    EXPECT_EQ(this->DoesTableExist(table3), false);
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyMultipleMigrationFilesAtOnce) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_up_3_valid);
    this->WriteMigrationFile(migration_file_down_2_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), true);
    this->ExpectUserVersion(3);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), true);
    EXPECT_EQ(this->DoesTableExist(table3), true);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), false);
    EXPECT_EQ(this->DoesTableExist(table3), false);
}

TEST_F(DatabaseSchemaUpdaterTest, ApplyMultipleMigrationFilesAtOnceWithFailure) {

    this->WriteMigrationFile(migration_file_up_1_valid);
    this->WriteMigrationFile(migration_file_up_2_valid);
    this->WriteMigrationFile(migration_file_up_3_valid);
    this->WriteMigrationFile(migration_file_down_2_valid);
    this->WriteMigrationFile(migration_file_down_3_valid);

    DatabaseSchemaUpdater updater{this->database.get()};

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 1), true);
    this->ExpectUserVersion(1);
    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), false);

    EXPECT_EQ(this->database->execute_statement(migration_file_up_2_valid.content.data()), true);

    EXPECT_EQ(this->DoesTableExist(table2), true);

    EXPECT_EQ(updater.apply_migration_files(this->migration_files_path, 3), false);

    EXPECT_EQ(this->DoesTableExist(table1), true);
    EXPECT_EQ(this->DoesTableExist(table2), true);
    EXPECT_EQ(this->DoesTableExist(table3), false);

    this->ExpectUserVersion(1);
}
