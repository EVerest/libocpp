// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/logging.hpp>

#include <ocpp1_6/charge_point_configuration.hpp>
#include <ocpp1_6/websocket/websocket.hpp>

namespace ocpp1_6 {

Websocket::Websocket(std::shared_ptr<ChargePointConfiguration> configuration) : tls(false), shutting_down(false) {

    this->configuration = configuration;

    this->websocket_plain = std::make_unique<WebsocketPlain>(configuration);
    this->websocket_tls = std::make_unique<WebsocketTLS>(configuration);
}

bool Websocket::connect() {
    auto uri = this->configuration->getCentralSystemURI();
    EVLOG(info) << "Connecting to uri: " << uri;
    if (uri.find("ws://") == 0) {
        this->tls = false;
        return this->websocket_plain->connect();
    }
    if (uri.find("wss://") == 0) {
        this->tls = true;
        return this->websocket_tls->connect();
    }

    return false;
}

void Websocket::disconnect() {
    this->shutting_down = true; // FIXME(kai): this makes the websocket inoperable after a disconnect, however this
                                // might not be a bad thing.
    if (this->tls) {
        EVLOG(info) << "Disconnecting TLS websocket...";
        this->websocket_tls->disconnect();
    } else {
        EVLOG(info) << "Disconnecting plain websocket...";
        this->websocket_plain->disconnect();
    }
}
void Websocket::register_connected_callback(const std::function<void()>& callback) {
    this->websocket_plain->register_connected_callback(callback);
    this->websocket_tls->register_connected_callback(callback);
}
void Websocket::register_disconnected_callback(const std::function<void()>& callback) {
    this->websocket_plain->register_disconnected_callback(callback);
    this->websocket_tls->register_disconnected_callback(callback);
}
void Websocket::register_message_callback(const std::function<void(const std::string& message)>& callback) {
    this->websocket_plain->register_message_callback(callback);
    this->websocket_tls->register_message_callback(callback);
}

bool Websocket::send(const std::string& message) {
    if (this->tls) {
        return this->websocket_tls->send(message);
    } else {
        return this->websocket_plain->send(message);
    }

    return true;
}

} // namespace ocpp1_6
