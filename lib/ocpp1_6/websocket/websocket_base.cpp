// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/logging.hpp>

#include <ocpp1_6/charge_point_configuration.hpp>
#include <ocpp1_6/websocket/websocket_base.hpp>

namespace ocpp1_6 {

WebsocketBase::WebsocketBase(std::shared_ptr<ChargePointConfiguration> configuration) :
    shutting_down(false),
    is_connected(false),
    configuration(configuration),
    connected_callback(nullptr),
    disconnected_callback(nullptr),
    message_callback(nullptr),
    reconnect_timer(nullptr) {
}

WebsocketBase::~WebsocketBase() {
    this->websocket_thread->detach();
}

void WebsocketBase::register_connected_callback(const std::function<void()>& callback) {
    this->connected_callback = callback;
}

void WebsocketBase::register_disconnected_callback(const std::function<void()>& callback) {
    this->disconnected_callback = callback;
}

void WebsocketBase::register_message_callback(const std::function<void(const std::string& message)>& callback) {
    this->message_callback = callback;
}

void WebsocketBase::register_sign_certificate_callback(const std::function<void()>& callback) {
    this->sign_certificate_callback = callback;
}

bool WebsocketBase::initialized() {
    if (this->shutting_down) {
        EVLOG(error) << "Not properly initialized: websocket already shutdown.";
        return false;
    }
    if (this->connected_callback == nullptr) {
        EVLOG(error) << "Not properly initialized: please register connected callback.";
        return false;
    }
    if (this->disconnected_callback == nullptr) {
        EVLOG(error) << "Not properly initialized: please register disconnected callback.";
        return false;
    }
    if (this->message_callback == nullptr) {
        EVLOG(error) << "Not properly initialized: please register message callback.";
        return false;
    }

    return true;
}

void WebsocketBase::disconnect() {
    if (!this->initialized()) {
        EVLOG(error) << "Cannot disconnect a websocket that was not initialized";
        return;
    }
    this->shutting_down = true; // FIXME(kai): this makes the websocket inoperable after a disconnect, however this
                                // might not be a bad thing.
    if (this->reconnect_timer) {
        this->reconnect_timer.get()->cancel();
    }

    EVLOG(info) << "Disconnecting websocket...";
    this->close(websocketpp::close::status::normal, "");
}

boost::optional<std::string> WebsocketBase::getAuthorizationHeader() {
    boost::optional<std::string> auth_header = boost::none;
    auto authorization_key = this->configuration->getAuthorizationKey();
    if (authorization_key != boost::none) {
        EVLOG(debug) << "AuthorizationKey present, encoding authentication header";
        std::string plain_auth_header = this->configuration->getChargePointId() + ":" + authorization_key.value();
        auth_header.emplace(std::string("Basic ") + websocketpp::base64_encode(plain_auth_header));
    }
    return auth_header;
}

} // namespace ocpp1_6
