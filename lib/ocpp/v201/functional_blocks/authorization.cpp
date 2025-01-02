// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/common/constants.hpp>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/functional_blocks/authorization.hpp>

ocpp::v201::Authorization::Authorization(MessageDispatcherInterface<MessageType>& message_dispatcher,
                                         DeviceModel& device_model, ConnectivityManager& connectivity_manager,
                                         std::shared_ptr<DatabaseHandler> database_handler) :
    message_dispatcher(message_dispatcher),
    device_model(device_model),
    connectivity_manager(connectivity_manager),
    database_handler(database_handler) {
    this->auth_cache_cleanup_thread = std::thread(&Authorization::cache_cleanup_handler, this);
}

ocpp::v201::Authorization::~Authorization() {
    {
        std::scoped_lock lk(this->auth_cache_cleanup_mutex);
        this->stop_auth_cache_cleanup_handler = true;
    }
    this->auth_cache_cleanup_cv.notify_one();
    this->auth_cache_cleanup_thread.join();
}

void ocpp::v201::Authorization::handle_message(const ocpp::EnhancedMessage<MessageType>& message) {
    const auto& json_message = message.message;
    if (message.messageType == MessageType::ClearCache) {
        this->handle_clear_cache_req(json_message);
    } else {
        throw MessageTypeNotImplementedException(message.messageType);
    }
}

ocpp::v201::AuthorizeResponse
ocpp::v201::Authorization::authorize_req(const IdToken id_token, const std::optional<ocpp::CiString<5500>>& certificate,
                                         const std::optional<std::vector<OCSPRequestData>>& ocsp_request_data) {
    AuthorizeRequest req;
    req.idToken = id_token;
    req.certificate = certificate;
    req.iso15118CertificateHashData = ocsp_request_data;

    AuthorizeResponse response;
    response.idTokenInfo.status = AuthorizationStatusEnum::Unknown;

    if (!this->connectivity_manager.is_websocket_connected()) {
        return response;
    }

    ocpp::Call<AuthorizeRequest> call(req);
    auto future = this->message_dispatcher.dispatch_call_async(call);

    if (future.wait_for(DEFAULT_WAIT_FOR_FUTURE_TIMEOUT) == std::future_status::timeout) {
        EVLOG_warning << "Waiting for DataTransfer.conf(Authorize) future timed out!";
        return response;
    }

    const auto enhanced_message = future.get();

    if (enhanced_message.messageType != MessageType::AuthorizeResponse) {
        return response;
    }

    try {
        ocpp::CallResult<AuthorizeResponse> call_result = enhanced_message.message;
        return call_result.msg;
    } catch (const EnumConversionException& e) {
        EVLOG_error << "EnumConversionException during handling of message: " << e.what();
        auto call_error = CallError(enhanced_message.uniqueId, "FormationViolation", e.what(), json({}));
        this->message_dispatcher.dispatch_call_error(call_error);
        return response;
    }
}

void ocpp::v201::Authorization::trigger_authorization_cache_cleanup() {
    {
        std::scoped_lock lk(this->auth_cache_cleanup_mutex);
        this->auth_cache_cleanup_required = true;
    }
    this->auth_cache_cleanup_cv.notify_one();
}

void ocpp::v201::Authorization::update_authorization_cache_size() {
    auto& auth_cache_size = ControllerComponentVariables::AuthCacheStorage;
    if (auth_cache_size.variable.has_value()) {
        try {
            auto size = this->database_handler->authorization_cache_get_binary_size();
            this->device_model.set_read_only_value(auth_cache_size.component, auth_cache_size.variable.value(),
                                                   AttributeEnum::Actual, std::to_string(size),
                                                   VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL);
        } catch (const common::DatabaseException& e) {
            EVLOG_warning << "Could not get authorization cache binary size from database: " << e.what();
        } catch (const std::exception& e) {
            EVLOG_warning << "Could not get authorization cache binary size from database" << e.what();
        }
    }
}

bool ocpp::v201::Authorization::is_auth_cache_ctrlr_enabled() {
    return this->device_model.get_optional_value<bool>(ControllerComponentVariables::AuthCacheCtrlrEnabled)
        .value_or(false);
}

void ocpp::v201::Authorization::authorization_cache_insert_entry(const std::string& id_token_hash,
                                                                 const IdTokenInfo& id_token_info) {
    this->database_handler->authorization_cache_insert_entry(id_token_hash, id_token_info);
}

std::optional<ocpp::v201::AuthorizationCacheEntry> ocpp::v201::Authorization::authorization_cache_get_entry(const std::string &id_token_hash)
{
    return this->database_handler->authorization_cache_get_entry(id_token_hash);
}

void ocpp::v201::Authorization::authorization_cache_delete_entry(const std::string &id_token_hash)
{
    this->database_handler->authorization_cache_delete_entry(id_token_hash);
}

void ocpp::v201::Authorization::authorization_cache_update_last_used(const std::string &id_token_hash)
{
    this->database_handler->authorization_cache_update_last_used(id_token_hash);
}

void ocpp::v201::Authorization::handle_clear_cache_req(Call<ClearCacheRequest> call) {
    ClearCacheResponse response;
    response.status = ClearCacheStatusEnum::Rejected;

    if (this->device_model.get_optional_value<bool>(ControllerComponentVariables::AuthCacheCtrlrEnabled)
            .value_or(true)) {
        try {
            this->database_handler->authorization_cache_clear();
            this->update_authorization_cache_size();
            response.status = ClearCacheStatusEnum::Accepted;
        } catch (common::DatabaseException& e) {
            auto call_error = CallError(call.uniqueId, "InternalError",
                                        "Database error while clearing authorization cache", json({}, true));
            this->message_dispatcher.dispatch_call_error(call_error);
            return;
        }
    }

    ocpp::CallResult<ClearCacheResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);
}

void ocpp::v201::Authorization::cache_cleanup_handler() {
    // Run the update once so the ram variable gets initialized
    this->update_authorization_cache_size();

    while (true) {
        {
            // Wait for next wakeup or timeout
            std::unique_lock lk(this->auth_cache_cleanup_mutex);
            if (this->auth_cache_cleanup_cv.wait_for(lk, std::chrono::minutes(15), [&]() {
                    return this->stop_auth_cache_cleanup_handler or this->auth_cache_cleanup_required;
                })) {
                EVLOG_debug << "Triggered authorization cache cleanup";
            } else {
                EVLOG_debug << "Time based authorization cache cleanup";
            }
            this->auth_cache_cleanup_required = false;
        }

        if (this->stop_auth_cache_cleanup_handler) {
            break;
        }

        auto lifetime = this->device_model.get_optional_value<int>(ControllerComponentVariables::AuthCacheLifeTime);
        try {
            this->database_handler->authorization_cache_delete_expired_entries(
                lifetime.has_value() ? std::optional<std::chrono::seconds>(*lifetime) : std::nullopt);

            auto meta_data = this->device_model.get_variable_meta_data(
                ControllerComponentVariables::AuthCacheStorage.component,
                ControllerComponentVariables::AuthCacheStorage.variable.value());

            if (meta_data.has_value()) {
                auto max_storage = meta_data->characteristics.maxLimit;
                if (max_storage.has_value()) {
                    while (this->database_handler->authorization_cache_get_binary_size() > max_storage.value()) {
                        this->database_handler->authorization_cache_delete_nr_of_oldest_entries(1);
                    }
                }
            }
        } catch (const common::DatabaseException& e) {
            EVLOG_warning << "Could not delete expired authorization cache entries from database: " << e.what();
        } catch (const std::exception& e) {
            EVLOG_warning << "Could not delete expired authorization cache entries from database: " << e.what();
        }

        this->update_authorization_cache_size();
    }
}
