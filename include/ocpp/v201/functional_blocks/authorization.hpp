// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include "ocpp/v201/messages/ClearCache.hpp"
#include <ocpp/v201/messages/Authorize.hpp>

#include <ocpp/v201/database_handler.hpp>
#include <ocpp/v201/message_dispatcher.hpp>
#include <ocpp/v201/message_handler.hpp>

namespace ocpp::v201 {
class AuthorizationInterface : public MessageHandlerInterface {
public:
    virtual ~AuthorizationInterface() {};

    virtual AuthorizeResponse authorize_req(const IdToken id_token, const std::optional<CiString<5500>>& certificate,
                                            const std::optional<std::vector<OCSPRequestData>>& ocsp_request_data) = 0;
    virtual void trigger_authorization_cache_cleanup() = 0;
    ///\brief Calculate and update the authorization cache size in the device model
    ///
    virtual void update_authorization_cache_size() = 0;
    virtual bool is_auth_cache_ctrlr_enabled() = 0;
    virtual void authorization_cache_insert_entry(const std::string& id_token_hash,
                                                  const IdTokenInfo& id_token_info) = 0;
    virtual std::optional<AuthorizationCacheEntry> authorization_cache_get_entry(const std::string& id_token_hash) = 0;
    virtual void authorization_cache_delete_entry(const std::string& id_token_hash) = 0;
    virtual void authorization_cache_update_last_used(const std::string& id_token_hash) = 0;
};

class Authorization : public AuthorizationInterface {
private: // Members
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    DeviceModel& device_model;
    ConnectivityManager& connectivity_manager;
    std::shared_ptr<DatabaseHandler> database_handler;

    // threads and synchronization
    bool auth_cache_cleanup_required;
    std::condition_variable auth_cache_cleanup_cv;
    std::mutex auth_cache_cleanup_mutex;
    std::thread auth_cache_cleanup_thread;
    std::atomic_bool stop_auth_cache_cleanup_handler;

public:
    Authorization(MessageDispatcherInterface<MessageType>& message_dispatcher, DeviceModel& device_model,
                  ConnectivityManager& connectivity_manager, std::shared_ptr<DatabaseHandler> database_handler);
    ~Authorization();
    void handle_message(const ocpp::EnhancedMessage<MessageType>& message) override;
    AuthorizeResponse authorize_req(const IdToken id_token, const std::optional<ocpp::CiString<5500>>& certificate,
                                    const std::optional<std::vector<OCSPRequestData>>& ocsp_request_data) override;
    void trigger_authorization_cache_cleanup() override;
    void update_authorization_cache_size() override;
    bool is_auth_cache_ctrlr_enabled() override;
    void authorization_cache_insert_entry(const std::string& id_token_hash, const IdTokenInfo& id_token_info) override;
    std::optional<AuthorizationCacheEntry> authorization_cache_get_entry(const std::string& id_token_hash) override;
    void authorization_cache_delete_entry(const std::string& id_token_hash) override;
    void authorization_cache_update_last_used(const std::string& id_token_hash) override;

private: // Functions
    void handle_clear_cache_req(Call<ClearCacheRequest> call);
    void cache_cleanup_handler();
};
} // namespace ocpp::v201
