// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <gtest/gtest.h>

#include <ocpp/v201/init_device_model_db.hpp>

namespace ocpp::v201 {

class InitDeviceModelDbTest : public ::testing::Test {
protected:
    const std::string DATABASE_PATH = "/tmp/pionix/test_db.db";
    const std::string MIGRATION_FILES_PATH = "/tmp/pionix/migrations";
    const std::string SCHEMAS_PATH = "/data/work/pionix/workspace/libocpp/config/v201/component_schemas";
};

TEST_F(InitDeviceModelDbTest, init_db) {
    InitDeviceModelDb db = InitDeviceModelDb(DATABASE_PATH, MIGRATION_FILES_PATH);
    db.initialize_database(SCHEMAS_PATH, true);
}

} // namespace ocpp::v201
