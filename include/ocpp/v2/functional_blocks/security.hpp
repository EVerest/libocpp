// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <optional>

#include <ocpp/v2/message_handler.hpp>
#include <ocpp/v2/ocsp_updater.hpp>

namespace ocpp {
class MessageLogging;

namespace v2 {
struct FunctionalBlockContext;

struct CertificateSignedRequest;
struct CertificateSignedResponse;
struct GetInstalledCertificateIdsRequest;
struct Get15118EVCertificateRequest;
struct Get15118EVCertificateResponse;
struct InstallCertificateRequest;
struct DeleteCertificateRequest;
struct SignCertificateResponse;

typedef std::function<void(const CiString<50>& event_type, const std::optional<CiString<255>>& tech_info)>
    SecurityEventCallback;

class SecurityInterface : public MessageHandlerInterface {

public:
    virtual ~SecurityInterface() override {
    }
    virtual void security_event_notification_req(const CiString<50>& event_type,
                                                 const std::optional<CiString<255>>& tech_info,
                                                 const bool triggered_internally, const bool critical,
                                                 const std::optional<DateTime>& timestamp = std::nullopt) = 0;
    virtual void sign_certificate_req(const ocpp::CertificateSigningUseEnum& certificate_signing_use,
                                      const bool initiated_by_trigger_message = false) = 0;
    virtual void stop_certificate_signed_timer() = 0;
    virtual void init_certificate_expiration_check_timers() = 0;
    virtual void stop_certificate_expiration_check_timers() = 0;

    virtual Get15118EVCertificateResponse
    on_get_15118_ev_certificate_request(const Get15118EVCertificateRequest& request) = 0;
};

class Security : public SecurityInterface {
public:
    Security(const FunctionalBlockContext& functional_block_context, MessageLogging& logging,
             OcspUpdaterInterface& ocsp_updater, SecurityEventCallback security_event_callback);
    virtual ~Security();
    void handle_message(const EnhancedMessage<MessageType>& message) override;
    virtual void stop_certificate_signed_timer() override;
    void init_certificate_expiration_check_timers() override;
    void stop_certificate_expiration_check_timers() override;
    Get15118EVCertificateResponse
    on_get_15118_ev_certificate_request(const Get15118EVCertificateRequest& request) override;

    /* OCPP message requests */
    virtual void security_event_notification_req(const CiString<50>& event_type,
                                                 const std::optional<CiString<255>>& tech_info,
                                                 const bool triggered_internally, const bool critical,
                                                 const std::optional<DateTime>& timestamp = std::nullopt) override;
    virtual void sign_certificate_req(const ocpp::CertificateSigningUseEnum& certificate_signing_use,
                                      const bool initiated_by_trigger_message = false) override;

private: // Members
    const FunctionalBlockContext& context;
    MessageLogging& logging;
    OcspUpdaterInterface& ocsp_updater;

    SecurityEventCallback security_event_callback;

    int csr_attempt;
    std::optional<ocpp::CertificateSigningUseEnum> awaited_certificate_signing_use_enum;
    Everest::SteadyTimer certificate_signed_timer;
    Everest::SteadyTimer client_certificate_expiration_check_timer;
    Everest::SteadyTimer v2g_certificate_expiration_check_timer;

private: // Functions
    /* OCPP message handlers */

    // Functional Block A: Security
    void handle_certificate_signed_req(Call<CertificateSignedRequest> call);
    void handle_sign_certificate_response(CallResult<SignCertificateResponse> call_result);

    // Functional Block M: ISO 15118 Certificate Management
    void handle_get_installed_certificate_ids_req(Call<GetInstalledCertificateIdsRequest> call);
    void handle_install_certificate_req(Call<InstallCertificateRequest> call);
    void handle_delete_certificate_req(Call<DeleteCertificateRequest> call);

    // Internal helper functions

    /// \brief Helper function to determine if a certificate installation should be allowed
    /// \param cert_type is the certificate type to be checked
    /// \return true if it should be allowed
    bool should_allow_certificate_install(InstallCertificateUseEnum cert_type) const;
    void scheduled_check_client_certificate_expiration();
    void scheduled_check_v2g_certificate_expiration();
};
} // namespace v2
} // namespace ocpp
