// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/functional_blocks/security.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/messages/SecurityEventNotification.hpp>
#include <ocpp/v201/utils.hpp>

constexpr int32_t minimum_cert_signing_wait_time_seconds = 250;

namespace ocpp::v201 {

Security::Security(MessageDispatcherInterface<MessageType>& message_dispatcher, DeviceModel& device_model,
                   MessageLogging& logging, EvseSecurity& evse_security,
                   ConnectivityManagerInterface& connectivity_manager, OcspUpdaterInterface& ocsp_updater,
                   SecurityEventCallback security_event_callback) :
    message_dispatcher(message_dispatcher),
    device_model(device_model),
    logging(logging),
    evse_security(evse_security),
    connectivity_manager(connectivity_manager),
    ocsp_updater(ocsp_updater),
    security_event_callback(security_event_callback),
    csr_attempt(1) {
}

Security::~Security() {
    stop();
}

void Security::handle_message(const EnhancedMessage<MessageType>& message) {
    const auto& json_message = message.message;
    switch (message.messageType) {
    case MessageType::CertificateSigned:
        this->handle_certificate_signed_req(json_message);
        break;
    case MessageType::SignCertificateResponse:
        this->handle_sign_certificate_response(json_message);
        break;
    default:
        throw MessageTypeNotImplementedException(message.messageType);
    }
}

void Security::stop() {
    this->certificate_signed_timer.stop();
}

void Security::security_event_notification_req(const CiString<50>& event_type,
                                               const std::optional<CiString<255>>& tech_info,
                                               const bool triggered_internally, const bool critical,
                                               const std::optional<DateTime>& timestamp) {
    EVLOG_debug << "Sending SecurityEventNotification";
    SecurityEventNotificationRequest req;

    req.type = event_type;
    if (timestamp.has_value()) {
        req.timestamp = timestamp.value();
    } else {
        req.timestamp = DateTime();
    }
    req.techInfo = tech_info;
    this->logging.security(json(req).dump());
    if (critical) {
        ocpp::Call<SecurityEventNotificationRequest> call(req);
        this->message_dispatcher.dispatch_call(call);
    }
    if (triggered_internally and this->security_event_callback != nullptr) {
        this->security_event_callback(event_type, tech_info);
    }
}

void Security::sign_certificate_req(const ocpp::CertificateSigningUseEnum& certificate_signing_use,
                                    const bool initiated_by_trigger_message) {
    if (this->awaited_certificate_signing_use_enum.has_value()) {
        EVLOG_warning
            << "Not sending new SignCertificate.req because still waiting for CertificateSigned.req from CSMS";
        return;
    }

    SignCertificateRequest req;

    std::optional<std::string> common;
    std::optional<std::string> country;
    std::optional<std::string> organization;

    if (certificate_signing_use == ocpp::CertificateSigningUseEnum::ChargingStationCertificate) {
        req.certificateType = ocpp::v201::CertificateSigningUseEnum::ChargingStationCertificate;
        common =
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::ChargeBoxSerialNumber);
        organization =
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::OrganizationName);
        country =
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::ISO15118CtrlrCountryName);
    } else {
        req.certificateType = ocpp::v201::CertificateSigningUseEnum::V2GCertificate;
        common = this->device_model.get_optional_value<std::string>(ControllerComponentVariables::ISO15118CtrlrSeccId);
        organization = this->device_model.get_optional_value<std::string>(
            ControllerComponentVariables::ISO15118CtrlrOrganizationName);
        country =
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::ISO15118CtrlrCountryName);
    }

    if (!common.has_value()) {
        EVLOG_warning << "Missing configuration of commonName to generate CSR";
        return;
    }

    if (!country.has_value()) {
        EVLOG_warning << "Missing configuration country to generate CSR";
        return;
    }

    if (!organization.has_value()) {
        EVLOG_warning << "Missing configuration of organizationName to generate CSR";
        return;
    }

    bool should_use_tpm =
        this->device_model.get_optional_value<bool>(ControllerComponentVariables::UseTPM).value_or(false);

    const auto result = this->evse_security.generate_certificate_signing_request(
        certificate_signing_use, country.value(), organization.value(), common.value(), should_use_tpm);

    if (result.status != GetCertificateSignRequestStatus::Accepted or !result.csr.has_value()) {
        EVLOG_error << "CSR generation was unsuccessful for sign request: "
                    << ocpp::conversions::certificate_signing_use_enum_to_string(certificate_signing_use);

        std::string gen_error = "Sign certificate req failed due to:" +
                                ocpp::conversions::generate_certificate_signing_request_status_to_string(result.status);
        this->security_event_notification_req(ocpp::security_events::CSRGENERATIONFAILED,
                                              std::optional<CiString<255>>(gen_error), true, true);
        return;
    }

    req.csr = result.csr.value();

    this->awaited_certificate_signing_use_enum = certificate_signing_use;

    ocpp::Call<SignCertificateRequest> call(req);
    this->message_dispatcher.dispatch_call(call, initiated_by_trigger_message);
}

void Security::handle_certificate_signed_req(Call<CertificateSignedRequest> call) {
    // reset these parameters
    this->csr_attempt = 1;
    this->awaited_certificate_signing_use_enum = std::nullopt;
    this->certificate_signed_timer.stop();

    CertificateSignedResponse response;
    response.status = CertificateSignedStatusEnum::Rejected;

    const auto certificate_chain = call.msg.certificateChain.get();
    ocpp::CertificateSigningUseEnum cert_signing_use;

    if (!call.msg.certificateType.has_value() or
        call.msg.certificateType.value() == CertificateSigningUseEnum::ChargingStationCertificate) {
        cert_signing_use = ocpp::CertificateSigningUseEnum::ChargingStationCertificate;
    } else {
        cert_signing_use = ocpp::CertificateSigningUseEnum::V2GCertificate;
    }

    const auto result = this->evse_security.update_leaf_certificate(certificate_chain, cert_signing_use);

    if (result == ocpp::InstallCertificateResult::Accepted) {
        response.status = CertificateSignedStatusEnum::Accepted;
        // For V2G certificates, also trigger an OCSP cache update
        if (cert_signing_use == ocpp::CertificateSigningUseEnum::V2GCertificate) {
            this->ocsp_updater.trigger_ocsp_cache_update();
        }
    }

    // Trigger a symlink update for V2G certificates
    if ((cert_signing_use == ocpp::CertificateSigningUseEnum::V2GCertificate) and
        this->device_model.get_optional_value<bool>(ControllerComponentVariables::UpdateCertificateSymlinks)
            .value_or(false)) {
        this->evse_security.update_certificate_links(cert_signing_use);
    }

    ocpp::CallResult<CertificateSignedResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);

    if (result != ocpp::InstallCertificateResult::Accepted) {
        this->security_event_notification_req("InvalidChargingStationCertificate",
                                              ocpp::conversions::install_certificate_result_to_string(result), true,
                                              true);
    }

    // reconnect with new certificate if valid and security profile is 3
    if (response.status == CertificateSignedStatusEnum::Accepted and
        cert_signing_use == ocpp::CertificateSigningUseEnum::ChargingStationCertificate and
        this->device_model.get_value<int>(ControllerComponentVariables::SecurityProfile) == 3) {
        this->connectivity_manager.on_charging_station_certificate_changed();

        const auto& security_event = ocpp::security_events::RECONFIGURATIONOFSECURITYPARAMETERS;
        std::string tech_info = "Changed charging station certificate";
        this->security_event_notification_req(CiString<50>(security_event), CiString<255>(tech_info), true,
                                              utils::is_critical(security_event));
    }
}

void Security::handle_sign_certificate_response(CallResult<SignCertificateResponse> call_result) {
    if (!this->awaited_certificate_signing_use_enum.has_value()) {
        EVLOG_warning
            << "Received SignCertificate.conf while not awaiting a CertificateSigned.req . This should not happen.";
        return;
    }

    if (call_result.msg.status == GenericStatusEnum::Accepted) {
        // set timer waiting for certificate signed
        const auto cert_signing_wait_minimum =
            this->device_model.get_optional_value<int>(ControllerComponentVariables::CertSigningWaitMinimum);
        const auto cert_signing_repeat_times =
            this->device_model.get_optional_value<int>(ControllerComponentVariables::CertSigningRepeatTimes);

        if (!cert_signing_wait_minimum.has_value()) {
            EVLOG_warning << "No CertSigningWaitMinimum is configured, will not attempt to retry SignCertificate.req "
                             "in case CSMS doesn't send CertificateSigned.req";
            return;
        }
        if (!cert_signing_repeat_times.has_value()) {
            EVLOG_warning << "No CertSigningRepeatTimes is configured, will not attempt to retry SignCertificate.req "
                             "in case CSMS doesn't send CertificateSigned.req";
            return;
        }

        if (this->csr_attempt > cert_signing_repeat_times.value()) {
            this->csr_attempt = 1;
            this->certificate_signed_timer.stop();
            this->awaited_certificate_signing_use_enum = std::nullopt;
            return;
        }
        int retry_backoff_milliseconds =
            std::max(minimum_cert_signing_wait_time_seconds, 1000 * cert_signing_wait_minimum.value()) *
            std::pow(2, this->csr_attempt); // prevent immediate repetition in case of value 0
        this->certificate_signed_timer.timeout(
            [this]() {
                EVLOG_info << "Did not receive CertificateSigned.req in time. Will retry with SignCertificate.req";
                this->csr_attempt++;
                const auto current_awaited_certificate_signing_use_enum =
                    this->awaited_certificate_signing_use_enum.value();
                this->awaited_certificate_signing_use_enum.reset();
                this->sign_certificate_req(current_awaited_certificate_signing_use_enum);
            },
            std::chrono::milliseconds(retry_backoff_milliseconds));
    } else {
        this->awaited_certificate_signing_use_enum = std::nullopt;
        this->csr_attempt = 1;
        EVLOG_warning << "SignCertificate.req has not been accepted by CSMS";
    }
}

} // namespace ocpp::v201
