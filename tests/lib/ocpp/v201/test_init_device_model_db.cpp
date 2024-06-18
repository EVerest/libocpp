// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <gtest/gtest.h>

#include <ocpp/v201/device_model_storage_sqlite.hpp>
#include <ocpp/v201/init_device_model_db.hpp>

namespace ocpp::v201 {

class InitDeviceModelDbTest : public ::testing::Test {
protected:
    const std::string DATABASE_PATH = "/tmp/pionix/test_db.db";
    const std::string MIGRATION_FILES_PATH = "/data/work/pionix/workspace/libocpp/config/v201/device_model_migrations";
    const std::string SCHEMAS_PATH = "/data/work/pionix/workspace/libocpp/config/v201/component_schemas";
    const std::string CONFIG_PATH = "/data/work/pionix/workspace/libocpp/config/v201/config.json";
};

TEST_F(InitDeviceModelDbTest, init_db) {
    DeviceModelStorageSqlite device_model_storage(DATABASE_PATH);
    InitDeviceModelDb db = InitDeviceModelDb(DATABASE_PATH, MIGRATION_FILES_PATH, device_model_storage);
    EXPECT_TRUE(db.initialize_database(SCHEMAS_PATH, true));
    EXPECT_TRUE(db.insert_config_and_default_values(SCHEMAS_PATH, CONFIG_PATH));
}

/* To test:
 * - Insert whole configuration
 * - add EVSE / Connector component
 * - remove EVSE / Connector component
 * - add EVSE / Connector Variable
 * - add non-EVSE / Connector Variable
 * - remove EVSE / Connector Variable
 * - change EVSE / Connector Variable
 * - add EVSE / Connector Variable Attribute
 * - remove EVSE / Connector Variable Attribute
 * - change EVSE / Connector Variable Attribute
 * - add EVSE / Connector Variable Characteristic
 * - remove EVSE / Connector Variable Characteristic
 * - change EVSE / Connector Variable Characteristic
 */

} // namespace ocpp::v201
