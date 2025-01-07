// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/functional_blocks/authorization.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>

#include "connectivity_manager_mock.hpp"
#include "device_model_test_helper.hpp"
#include "evse_security_mock.hpp"
#include "message_dispatcher_mock.hpp"
#include "mocks/database_handler_mock.hpp"

using namespace ocpp::v201;
using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class AuthorizationTest : public ::testing::Test {
public:
protected: // Members
    DeviceModelTestHelper device_model_test_helper;
    MockMessageDispatcher mock_dispatcher;
    DeviceModel* device_model;
    ::testing::NiceMock<ConnectivityManagerMock> connectivity_manager;
    ::testing::NiceMock<std::shared_ptr<ocpp::v201::DatabaseHandlerMock>> database_handler_mock;
    std::shared_ptr<ocpp::EvseSecurityMock> evse_security;

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
        evse_security(std::make_shared<ocpp::EvseSecurityMock>()),
        authorization(std::make_unique<Authorization>(mock_dispatcher, *device_model, connectivity_manager,
                                                      database_handler_mock, evse_security)) {
    }

    ~AuthorizationTest() {
    }

    ///
    /// \brief Set value of AuthCachCtrlr variable 'Enabled' in the device model.
    /// \param device_model The device model to set the value in.
    /// \param enabled      True to set to enabled.
    ///
    void set_auth_cache_enabled(DeviceModel* device_model, const bool enabled) {
        const auto& auth_cache_enabled = ControllerComponentVariables::AuthCacheCtrlrEnabled;
        EXPECT_EQ(device_model->set_value(auth_cache_enabled.component, auth_cache_enabled.variable.value(),
                                          AttributeEnum::Actual, enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    ///
    /// \brief Set value of AuthCtrlr variable 'Enabled' in the device model.
    /// \param device_model The device model to set the value in.
    /// \param enabled      True to set to enabled.
    ///
    void set_auth_ctrlr_enabled(DeviceModel* device_model, const bool enabled) {
        const auto& auth_ctrlr_enabled = ControllerComponentVariables::AuthCtrlrEnabled;
        EXPECT_EQ(device_model->set_value(auth_ctrlr_enabled.component, auth_ctrlr_enabled.variable.value(),
                                          AttributeEnum::Actual, enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    ///
    /// \brief Set value of LocalAuthListCtrlr variable 'Enabled' in the device model.
    /// \param device_model The device model to set the value in.
    /// \param enabled      True to set to enabled.
    ///
    void set_local_auth_list_ctrlr_enabled(DeviceModel* device_model, const bool enabled) {
        const auto& local_auth_list_ctrlr_enabled = ControllerComponentVariables::LocalAuthListCtrlrEnabled;
        EXPECT_EQ(device_model->set_value(local_auth_list_ctrlr_enabled.component,
                                          local_auth_list_ctrlr_enabled.variable.value(), AttributeEnum::Actual,
                                          enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    void disable_remote_authorization(DeviceModel* device_model, const bool disabled) {
        const auto& disable_remote_authorization = ControllerComponentVariables::DisableRemoteAuthorization;
        EXPECT_EQ(device_model->set_value(disable_remote_authorization.component,
                                          disable_remote_authorization.variable.value(), AttributeEnum::Actual,
                                          disabled ? "true" : "false", "default", true),
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

TEST_F(AuthorizationTest, update_authorization_cache_size_exception) {
    auto& auth_cache_size = ControllerComponentVariables::AuthCacheStorage;
    this->device_model->set_read_only_value(auth_cache_size.component, auth_cache_size.variable.value(),
                                            AttributeEnum::Actual, "42", "test");
    std::optional<int> size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(size.value(), 42);

    // Throw DatabaseException when requesting the binary size of the authorization cache. Application should not crash!
    EXPECT_CALL(*this->database_handler_mock, authorization_cache_get_binary_size())
        .WillRepeatedly(Throw(ocpp::common::DatabaseException("Database exception thrown!!")));

    this->authorization->update_authorization_cache_size();

    size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    // Value is not changed because the exception was thrown.
    EXPECT_EQ(size.value(), 42);
}

TEST_F(AuthorizationTest, update_authorization_cache_size_exception2) {
    auto& auth_cache_size = ControllerComponentVariables::AuthCacheStorage;
    this->device_model->set_read_only_value(auth_cache_size.component, auth_cache_size.variable.value(),
                                            AttributeEnum::Actual, "42", "test");
    std::optional<int> size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    EXPECT_EQ(size.value(), 42);

    // Throw other exception when requesting the binary size of the authorization cache. Application should not crash!
    EXPECT_CALL(*this->database_handler_mock, authorization_cache_get_binary_size())
        .WillRepeatedly(Throw(std::out_of_range("out of range exception thrown!!")));

    this->authorization->update_authorization_cache_size();

    size = device_model->get_optional_value<int>(auth_cache_size, AttributeEnum::Actual);
    ASSERT_TRUE(size.has_value());
    // Value is not changed because the exception was thrown.
    EXPECT_EQ(size.value(), 42);
}

TEST_F(AuthorizationTest, validate_token_accepted_central_token) {
    // Set AuthCtrlr::Enabled to true
    this->set_auth_ctrlr_enabled(this->device_model, true);
    IdToken id_token;
    // For a central token, an authorize request should not be sent.
    id_token.type = IdTokenEnum::Central;
    id_token.idToken = "test_token";
    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Accepted);
}

TEST_F(AuthorizationTest, validate_token_accepted_auth_ctrlr_disabled) {
    // Set AuthCtrlr::Enabled to false: no authorize request should be sent, just accept token.
    this->set_auth_ctrlr_enabled(this->device_model, false);
    IdToken id_token;
    id_token.type = IdTokenEnum::ISO14443;
    id_token.idToken = "test_token";
    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Accepted);
}

TEST_F(AuthorizationTest, validate_token_unknown) {
    // Set AuthCtrlr::Enabled to true
    this->set_auth_ctrlr_enabled(this->device_model, true);
    // Local auth list is disabled.
    this->set_local_auth_list_ctrlr_enabled(this->device_model, false);
    // Auth cache is disabled.
    this->set_auth_cache_enabled(this->device_model, false);
    // And remote authorization is also disabled.
    this->disable_remote_authorization(this->device_model, true);
    // But the websocket is connected.
    EXPECT_CALL(this->connectivity_manager, is_websocket_connected()).WillRepeatedly(Return(true));
    // Because almost everything is disabled, authorization can not be done and status is 'unknown'.
    IdToken id_token;
    id_token.type = IdTokenEnum::ISO14443;
    id_token.idToken = "test_token";

    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, validate_token_local_auth_list_enabled_accepted) {
    // Validate token with the local auth list: accepted
    // Set AuthCtrlr::Enabled to true
    this->set_auth_ctrlr_enabled(this->device_model, true);
    // Local auth list is enabled.
    this->set_local_auth_list_ctrlr_enabled(this->device_model, true);

    IdTokenInfo id_token_info_result;
    id_token_info_result.status = AuthorizationStatusEnum::Accepted;

    EXPECT_CALL(*this->database_handler_mock, get_local_authorization_list_entry(_))
        .WillRepeatedly(Return(id_token_info_result));

    IdToken id_token;
    id_token.type = IdTokenEnum::ISO14443;
    id_token.idToken = "test_token";

    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Accepted);
}

TEST_F(AuthorizationTest, validate_token_local_auth_list_enabled_unknown_no_remote_authorization) {
    // Validate token with the local auth list: unknown because remote authorization is not enabled and token info
    // status is not accepted. Set AuthCtrlr::Enabled to true
    this->set_auth_ctrlr_enabled(this->device_model, true);
    // Local auth list is enabled.
    this->set_local_auth_list_ctrlr_enabled(this->device_model, true);
    // Disable remote authorization.
    this->disable_remote_authorization(this->device_model, true);

    IdTokenInfo id_token_info_result;
    id_token_info_result.status = AuthorizationStatusEnum::Invalid;

    EXPECT_CALL(*this->database_handler_mock, get_local_authorization_list_entry(_))
        .WillRepeatedly(Return(id_token_info_result));

    IdToken id_token;
    id_token.type = IdTokenEnum::ISO14443;
    id_token.idToken = "test_token";

    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Unknown);
}

TEST_F(AuthorizationTest, validate_token_local_auth_list_enabled_unknown_websocket_disconnected) {
    // Validate token with the local auth list: unknown the websocket is not connected and token info
    // status is not accepted. Set AuthCtrlr::Enabled to true
    this->set_auth_ctrlr_enabled(this->device_model, true);
    // Local auth list is enabled.
    this->set_local_auth_list_ctrlr_enabled(this->device_model, true);
    // Remote authorization is enabled.
    this->disable_remote_authorization(this->device_model, false);
    // But the websocket is disconnected so it is not possible to authorize the request.
    EXPECT_CALL(this->connectivity_manager, is_websocket_connected()).WillRepeatedly(Return(false));

    IdTokenInfo id_token_info_result;
    id_token_info_result.status = AuthorizationStatusEnum::Invalid;

    EXPECT_CALL(*this->database_handler_mock, get_local_authorization_list_entry(_))
        .WillRepeatedly(Return(id_token_info_result));

    IdToken id_token;
    id_token.type = IdTokenEnum::ISO14443;
    id_token.idToken = "test_token";

    EXPECT_EQ(authorization->validate_token(id_token, std::nullopt, std::nullopt).idTokenInfo.status,
              AuthorizationStatusEnum::Unknown);
}
