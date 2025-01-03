// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/functional_blocks/authorization.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>

#include "connectivity_manager_mock.hpp"
#include "device_model_test_helper.hpp"
#include "message_dispatcher_mock.hpp"
#include "mocks/database_handler_mock.hpp"

using namespace ocpp::v201;
using ::testing::_;
using ::testing::Return;

class AuthorizationTest : public ::testing::Test {
public:
protected: // Members
    DeviceModelTestHelper device_model_test_helper;
    MockMessageDispatcher mock_dispatcher;
    DeviceModel* device_model;
    ::testing::NiceMock<ConnectivityManagerMock> connectivity_manager;
    ::testing::NiceMock<std::shared_ptr<ocpp::v201::DatabaseHandlerMock>> database_handler_mock;

    // Authorization is a unique ptr because we first need to create the database handler before making authorization,
    // and the database handler is not created in the initializer list. So we have to be able to create Authorization
    // later. // TODO mz this might be changed??
    std::unique_ptr<Authorization> authorization;

protected: // Functions
    AuthorizationTest() :
        device_model_test_helper(),
        mock_dispatcher(),
        device_model(device_model_test_helper.get_device_model()),
        connectivity_manager(),
        database_handler_mock(std::make_shared<ocpp::v201::DatabaseHandlerMock>()),
        authorization(std::make_unique<Authorization>(mock_dispatcher, *device_model, connectivity_manager,
                                                      database_handler_mock)) {
    }

    ~AuthorizationTest() {
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

    IdToken get_id_token(const std::string& token = "VALID_ID_TOKEN",
                         const IdTokenEnum token_type = IdTokenEnum::ISO14443) {
        IdToken id_token;
        id_token.idToken = token;
        id_token.type = token_type;
        return id_token;
    }

    ocpp::EnhancedMessage<MessageType>
    create_example_authorize_response(const std::optional<AuthorizeCertificateStatusEnum> certificate_status,
                                      const AuthorizationStatusEnum& status) {
        AuthorizeResponse response;
        response.certificateStatus = certificate_status;
        IdTokenInfo id_token_info;
        id_token_info.status = status;
        response.idTokenInfo = id_token_info;
        ocpp::CallResult<AuthorizeResponse> call_result(response, "uniqueId");
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.messageType = MessageType::AuthorizeResponse;
        enhanced_message.message = call_result;
        return enhanced_message;
    }
};

TEST_F(AuthorizationTest, is_auth_cache_ctrlr_enabled) {
    set_auth_cache_enabled(this->device_model, false);
    EXPECT_FALSE(authorization->is_auth_cache_ctrlr_enabled());

    set_auth_cache_enabled(this->device_model, true);
    EXPECT_TRUE(authorization->is_auth_cache_ctrlr_enabled());
}

TEST_F(AuthorizationTest, authorize_req_websocket_disconnected) {
    ON_CALL(this->connectivity_manager, is_websocket_connected()).WillByDefault(Return(false));
    const AuthorizeResponse response = authorization->authorize_req(get_id_token(), std::nullopt, std::nullopt);
    EXPECT_EQ(response.idTokenInfo.status, AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, authorize_req_wrong_future_message_type) {
    ON_CALL(this->connectivity_manager, is_websocket_connected()).WillByDefault(Return(true));
    ocpp::EnhancedMessage<MessageType> enhanced_message;
    enhanced_message.messageType = MessageType::GetDisplayMessages;
    EXPECT_CALL(mock_dispatcher, dispatch_call_async(_, _))
        .WillOnce(Return(std::async(std::launch::deferred, [enhanced_message]() { return enhanced_message; })));

    const AuthorizeResponse response = authorization->authorize_req(get_id_token(), std::nullopt, std::nullopt);
    EXPECT_EQ(response.idTokenInfo.status, AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, authorize_req_accepted) {
    ON_CALL(this->connectivity_manager, is_websocket_connected()).WillByDefault(Return(true));
    EXPECT_CALL(mock_dispatcher, dispatch_call_async(_, _)).WillOnce(Return(std::async(std::launch::deferred, [this]() {
        return create_example_authorize_response(AuthorizeCertificateStatusEnum::Accepted,
                                                 AuthorizationStatusEnum::Accepted);
    })));

    const AuthorizeResponse response = authorization->authorize_req(get_id_token(), std::nullopt, std::nullopt);
    EXPECT_EQ(response.idTokenInfo.status, AuthorizationStatusEnum::Accepted);
}

TEST_F(AuthorizationTest, authorize_req_exception) {
    ON_CALL(this->connectivity_manager, is_websocket_connected()).WillByDefault(Return(true));
    EXPECT_CALL(mock_dispatcher, dispatch_call_async(_, _)).WillOnce(Return(std::async(std::launch::deferred, [this]() {
        return create_example_authorize_response(AuthorizeCertificateStatusEnum::Accepted,
                                                 static_cast<AuthorizationStatusEnum>(INT32_MAX));
    })));

    const AuthorizeResponse response = authorization->authorize_req(get_id_token(), std::nullopt, std::nullopt);
    EXPECT_EQ(response.idTokenInfo.status, AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, authorize_req_exception2) {
    ON_CALL(this->connectivity_manager, is_websocket_connected()).WillByDefault(Return(true));
    EXPECT_CALL(mock_dispatcher, dispatch_call_async(_, _)).WillOnce(Return(std::async(std::launch::deferred, [this]() {
        return create_example_authorize_response(static_cast<AuthorizeCertificateStatusEnum>(INT32_MAX),
                                                 AuthorizationStatusEnum::Accepted);
    })));

    const AuthorizeResponse response = authorization->authorize_req(get_id_token(), std::nullopt, std::nullopt);
    EXPECT_EQ(response.idTokenInfo.status, AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, update_authorization_cache_size) {
    auto& auth_cache_size = ControllerComponentVariables::AuthCacheStorage;
    this->device_model->set_read_only_value(auth_cache_size.component, auth_cache_size.variable.value(),
                                            AttributeEnum::Actual, "42", "test");
    std::optional<int> size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(size.value(), 42);

    EXPECT_CALL(*this->database_handler_mock, authorization_cache_get_binary_size()).WillRepeatedly(Return(35));
    this->authorization->update_authorization_cache_size();

    size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(size.value(), 35);
}
