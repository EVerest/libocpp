// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/ctrlr_component_variables.hpp>

#define private public  // Make everything in security.hpp public so we can trigger the timer.
#include <ocpp/v201/functional_blocks/security.hpp>
#undef private
#include <ocpp/v201/messages/Reset.hpp>

#include "connectivity_manager_mock.hpp"
#include "device_model_test_helper.hpp"
#include "evse_security_mock.hpp"
#include "message_dispatcher_mock.hpp"
#include "ocsp_updater_mock.hpp"

using namespace ocpp;
using namespace ocpp::v201;
using ::testing::_;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::Return;

class SecurityTest : public ::testing::Test {
public:
protected: // Members
    DeviceModelTestHelper device_model_test_helper;
    DeviceModel* device_model;
    MockMessageDispatcher mock_dispatcher;
    // TODO mz mock logging???
    ocpp::MessageLogging logging;
    ocpp::EvseSecurityMock evse_security;
    ConnectivityManagerMock connectivity_manager;
    OcspUpdaterMock ocsp_updater;
    MockFunction<void(const ocpp::CiString<50>& event_type, const std::optional<ocpp::CiString<255>>& tech_info)>
        security_event_callback_mock;
    Security security;

protected: // Functions
    SecurityTest() :
        device_model_test_helper(),
        device_model(device_model_test_helper.get_device_model()),
        logging(false, "", "", false, false, false, false, false, false, nullptr),
        security(mock_dispatcher, *device_model, logging, evse_security, connectivity_manager, ocsp_updater,
                 security_event_callback_mock.AsStdFunction()) {
    }

    ocpp::EnhancedMessage<MessageType> create_example_certificate_signed_request(
        const std::string& certificate_chain = "",
        const std::optional<ocpp::v201::CertificateSigningUseEnum> certificate_type = std::nullopt) {
        CertificateSignedRequest request;
        request.certificateChain = certificate_chain;
        request.certificateType = certificate_type;
        ocpp::Call<CertificateSignedRequest> call(request);
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.messageType = MessageType::CertificateSigned;
        enhanced_message.message = call;
        return enhanced_message;
    }

    ocpp::EnhancedMessage<MessageType> create_example_sign_certificate_response(
        const GenericStatusEnum status = GenericStatusEnum::Accepted) {
        SignCertificateResponse response;
        response.status = status;
        ocpp::CallResult<SignCertificateResponse> call_result;
        call_result.msg = response;
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.messageType = MessageType::SignCertificateResponse;
        enhanced_message.message = call_result;
        return enhanced_message;
    }

    void set_update_certificate_symlinks_enabled(DeviceModel* device_model, const bool enabled) {
        const auto& update_certificate_symlinks = ControllerComponentVariables::UpdateCertificateSymlinks;
        EXPECT_EQ(device_model->set_value(update_certificate_symlinks.component,
                                          update_certificate_symlinks.variable.value(), AttributeEnum::Actual,
                                          enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    void set_security_profile(DeviceModel* device_model, const int profile) {
        const auto& security_profile = ControllerComponentVariables::SecurityProfile;
        EXPECT_EQ(device_model->set_value(security_profile.component, security_profile.variable.value(),
                                          AttributeEnum::Actual, std::to_string(profile), "default", true),
                  SetVariableStatusEnum::Accepted);
    }
};

TEST_F(SecurityTest, handle_message_not_implemented) {
    // Try to handle a message with the wrong type, should throw an exception.
    ResetRequest request;
    request.type = ResetEnum::Immediate;
    ocpp::Call<ResetRequest> call(request);
    ocpp::EnhancedMessage<MessageType> enhanced_message;
    enhanced_message.messageType = MessageType::Reset;
    enhanced_message.message = call;

    EXPECT_THROW(security.handle_message(enhanced_message), MessageTypeNotImplementedException);
}

TEST_F(SecurityTest, handle_message_certificate_signed_v2gcertificate) {
    set_update_certificate_symlinks_enabled(this->device_model, true);

    // Leaf certificate should be updated.
    EXPECT_CALL(evse_security, update_leaf_certificate("", ocpp::CertificateSigningUseEnum::V2GCertificate))
        .WillOnce(Return(ocpp::InstallCertificateResult::Accepted));
    // For V2G certificates, OCSP cache update should be triggered.
    EXPECT_CALL(ocsp_updater, trigger_ocsp_cache_update()).Times(1);
    // For V2G certificates, a symlink update should be triggered when that is set in the device model
    EXPECT_CALL(evse_security, update_certificate_links(ocpp::CertificateSigningUseEnum::V2GCertificate)).Times(1);
    // As updating the leaf certificate is accepted, the call result will be 'Accepted'.
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CertificateSignedResponse>();
        EXPECT_EQ(response.status, CertificateSignedStatusEnum::Accepted);
    }));

    security.handle_message(
        create_example_certificate_signed_request("", ocpp::v201::CertificateSigningUseEnum::V2GCertificate));
}

TEST_F(SecurityTest, handle_message_certificate_signed_v2gcertificate_symlinks_disabled) {
    set_update_certificate_symlinks_enabled(this->device_model, false);

    // Leaf certificate should be updated.
    EXPECT_CALL(evse_security, update_leaf_certificate("", ocpp::CertificateSigningUseEnum::V2GCertificate))
        .WillOnce(Return(ocpp::InstallCertificateResult::Accepted));
    // For V2G certificates, OCSP cache update should be triggered.
    EXPECT_CALL(ocsp_updater, trigger_ocsp_cache_update()).Times(1);
    // For V2G certificates, a symlink update should not be triggered when it is not set in the device model
    EXPECT_CALL(evse_security, update_certificate_links(ocpp::CertificateSigningUseEnum::V2GCertificate)).Times(0);
    // As updating the leaf certificate is accepted, the call result will be 'Accepted'.
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CertificateSignedResponse>();
        EXPECT_EQ(response.status, CertificateSignedStatusEnum::Accepted);
    }));

    security.handle_message(
        create_example_certificate_signed_request("", ocpp::v201::CertificateSigningUseEnum::V2GCertificate));
}

TEST_F(SecurityTest, handle_message_certificate_signed_v2gcertificate_update_leaf_not_accepted) {
    set_update_certificate_symlinks_enabled(this->device_model, true);

    // Leaf certificate should be updated, returns 'Expired'.
    EXPECT_CALL(evse_security, update_leaf_certificate("", ocpp::CertificateSigningUseEnum::V2GCertificate))
        .WillOnce(Return(ocpp::InstallCertificateResult::Expired));
    // For V2G certificates, OCSP cache update should be triggered, but only when updating leaf certificate is accepted.
    EXPECT_CALL(ocsp_updater, trigger_ocsp_cache_update()).Times(0);
    // For V2G certificates, a symlink update should be triggered when that is set in the device model
    EXPECT_CALL(evse_security, update_certificate_links(ocpp::CertificateSigningUseEnum::V2GCertificate)).Times(1);
    // As updating the leaf certificate is accepted, the call result will be 'Accepted'.
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CertificateSignedResponse>();
        EXPECT_EQ(response.status, CertificateSignedStatusEnum::Rejected);
    }));
    // Install certificate is not accepted, this should trigger a security event notification.
    EXPECT_CALL(security_event_callback_mock,
                Call(CiString<50>("InvalidChargingStationCertificate"),
                     std::optional<CiString<255>>(ocpp::conversions::install_certificate_result_to_string(
                         ocpp::InstallCertificateResult::Expired))));

    security.handle_message(
        create_example_certificate_signed_request("", ocpp::v201::CertificateSigningUseEnum::V2GCertificate));
}

TEST_F(SecurityTest, handle_message_certificate_signed_chargingstationcertificate_accepted_securityprofile_3) {
    set_update_certificate_symlinks_enabled(this->device_model, true);
    set_security_profile(this->device_model, 3);

    // Leaf certificate should be updated.
    EXPECT_CALL(evse_security, update_leaf_certificate("", ocpp::CertificateSigningUseEnum::ChargingStationCertificate))
        .WillOnce(Return(ocpp::InstallCertificateResult::Accepted));
    // For Charging Station certificates, OCSP cache update should NOT be triggered.
    EXPECT_CALL(ocsp_updater, trigger_ocsp_cache_update()).Times(0);
    // For V2G certificates, a symlink update should NOT be triggered, also not when it is set in the device model.
    EXPECT_CALL(evse_security, update_certificate_links(ocpp::CertificateSigningUseEnum::V2GCertificate)).Times(0);
    // As updating the leaf certificate is accepted, the call result will be 'Accepted'.
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CertificateSignedResponse>();
        EXPECT_EQ(response.status, CertificateSignedStatusEnum::Accepted);
    }));
    // The connectivity manager should be informed of the changed certificate (because of security profile 3)
    EXPECT_CALL(connectivity_manager, on_charging_station_certificate_changed()).Times(1);
    // A security event notification should be sent (because of security profile 3)
    EXPECT_CALL(security_event_callback_mock,
                Call(CiString<50>("ReconfigurationOfSecurityParameters"),
                     std::optional<CiString<255>>("Changed charging station certificate")));

    security.handle_message(create_example_certificate_signed_request(
        "", ocpp::v201::CertificateSigningUseEnum::ChargingStationCertificate));
}

TEST_F(SecurityTest, handle_message_certificate_signed_chargingstationcertificate_accepted_securityprofile_1) {
    set_update_certificate_symlinks_enabled(this->device_model, true);
    set_security_profile(this->device_model, 1);

    // Leaf certificate should be updated.
    EXPECT_CALL(evse_security, update_leaf_certificate("", ocpp::CertificateSigningUseEnum::ChargingStationCertificate))
        .WillOnce(Return(ocpp::InstallCertificateResult::Accepted));
    // For Charging Station certificates, OCSP cache update should NOT be triggered.
    EXPECT_CALL(ocsp_updater, trigger_ocsp_cache_update()).Times(0);
    // For V2G certificates, a symlink update should NOT be triggered, also not when it is set in the device model.
    EXPECT_CALL(evse_security, update_certificate_links(ocpp::CertificateSigningUseEnum::V2GCertificate)).Times(0);
    // As updating the leaf certificate is accepted, the call result will be 'Accepted'.
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CertificateSignedResponse>();
        EXPECT_EQ(response.status, CertificateSignedStatusEnum::Accepted);
    }));
    // The connectivity manager should NOT be informed of the changed certificate (because of security profile < 3)
    EXPECT_CALL(connectivity_manager, on_charging_station_certificate_changed()).Times(0);
    // A security event notification should NOT be sent (because of security profile < 3)
    EXPECT_CALL(security_event_callback_mock, Call(_, _)).Times(0);

    // When no certificate type is given, charging station certificate type is used.
    security.handle_message(create_example_certificate_signed_request(
        "", std::nullopt));
}

TEST_F(SecurityTest, handle_sign_certificate_response_no_request) {
    security.handle_message(create_example_sign_certificate_response());
    timer_stub_stop_called_count = 0;
    timer_stub_timeout_called_count = 0;
    timer_stub_interval_called_count = 0;
    timer_stub_at_called_count = 0;

    EXPECT_EQ(timer_stub_stop_called_count, 0);
    EXPECT_EQ(timer_stub_timeout_called_count, 0);
}
