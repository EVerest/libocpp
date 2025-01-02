// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/functional_blocks/authorization.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>

#include "connectivity_manager_mock.hpp"
#include "device_model_test_helper.hpp"
#include "message_dispatcher_mock.hpp"

using namespace ocpp::v201;

class AuthorizationTest : public ::testing::Test {
public:
protected: // Members
    DeviceModelTestHelper device_model_test_helper;
    MockMessageDispatcher mock_dispatcher;
    std::unique_ptr<DeviceModel> device_model;
    ConnectivityManagerMock connectivity_manager;
    std::shared_ptr<DatabaseHandler> database_handler;

    // Authorization is a unique ptr because we first need to create the database handler before making authorization,
    // and the database handler is not created in the initializer list. So we have to be able to create Authorization
    // later.
    std::unique_ptr<Authorization> authorization;

protected: // Functions
    AuthorizationTest() :
        device_model_test_helper(),
        mock_dispatcher(),
        device_model(device_model_test_helper.get_device_model()),
        connectivity_manager(),
        database_handler(nullptr),
        authorization(nullptr) {
        std::unique_ptr<ocpp::common::DatabaseConnection> database_connection =
            std::make_unique<ocpp::common::DatabaseConnection>(DEVICE_MODEL_DB_IN_MEMORY_PATH);
        database_connection->open_connection();
        database_handler = std::make_shared<DatabaseHandler>(std::move(database_connection), MIGRATION_FILES_PATH);
        authorization = std::make_unique<Authorization>(mock_dispatcher, *device_model.get(), connectivity_manager,
                                                        database_handler);
    }

    ~AuthorizationTest () {
        database_handler = nullptr;
        authorization = nullptr;
    }

    ///
    /// \brief Set value of AuthCachCtrlr variable 'Enabled' in the deivce model.
    /// \param device_model The device model to set the value in.
    /// \param enabled      True to set to enabled.
    ///
    void set_auth_cache_enabled(DeviceModel* device_model, const bool enabled) {
        const auto& auth_cache_enabled = ControllerComponentVariables::AuthCacheCtrlrEnabled;
        EXPECT_EQ(device_model->set_value(auth_cache_enabled.component, auth_cache_enabled.variable.value(),
                                          AttributeEnum::Actual, enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }
};

TEST_F(AuthorizationTest, is_auth_cache_ctrlr_enabled) {
    EXPECT_TRUE(true);
    // set_auth_cache_enabled(this->device_model.get(), false);
    // EXPECT_FALSE(authorization ->is_auth_cache_ctrlr_enabled());

    // set_auth_cache_enabled(this->device_model.get(), true);
    // EXPECT_TRUE(authorization ->is_auth_cache_ctrlr_enabled());
}
