// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#include <ocpp/common/types.hpp>
#include <ocpp/common/websocket/websocket_plain.hpp>

#include <everest/logging.hpp>

#include <boost/optional/optional.hpp>

#include <memory>
#include <stdexcept>

namespace ocpp {

extern websocketpp::close::status::value close_reason_to_value(WebsocketCloseReason reason);
extern WebsocketCloseReason value_to_close_reason(websocketpp::close::status::value value);

WebsocketPlain::WebsocketPlain(const WebsocketConnectionOptions& connection_options) : WebsocketBase() {
    set_connection_options(connection_options);

    EVLOG_debug << "Initialised WebsocketPlain with URI: " << this->connection_options.csms_uri.string();
}

void WebsocketPlain::set_connection_options(const WebsocketConnectionOptions& connection_options) {
    switch (connection_options.security_profile) { // `switch` used to lint on missing enum-values
    case security::SecurityProfile::OCPP_1_6_ONLY_UNSECURED_TRANSPORT_WITHOUT_BASIC_AUTHENTICATION:
    case security::SecurityProfile::UNSECURED_TRANSPORT_WITH_BASIC_AUTHENTICATION:
        break;
    case security::SecurityProfile::TLS_WITH_BASIC_AUTHENTICATION:
    case security::SecurityProfile::TLS_WITH_CLIENT_SIDE_CERTIFICATES:
        throw std::invalid_argument("`security_profile` is not a plain, unsecured one.");
    default:
        throw std::invalid_argument("unknown `security_profile`, value = " +
                                    std::to_string(connection_options.security_profile));
    }

    set_connection_options_base(connection_options);
    this->connection_options.csms_uri.set_secure(false);
}

bool WebsocketPlain::connect() {
    if (!this->initialized()) {
        return false;
    }

    EVLOG_info << "Connecting to plain websocket at uri: " << this->connection_options.csms_uri.string()
               << " with security profile: " << this->connection_options.security_profile;

    this->ws_client.clear_access_channels(websocketpp::log::alevel::all);
    this->ws_client.clear_error_channels(websocketpp::log::elevel::all);
    this->ws_client.init_asio();
    this->ws_client.start_perpetual();

    websocket_thread.reset(new websocketpp::lib::thread(&client::run, &this->ws_client));

    this->connect_plain();
    return true;
}

bool WebsocketPlain::send(const std::string& message) {
    if (!this->initialized()) {
        EVLOG_error << "Could not send message because websocket is not properly initialized.";
        return false;
    }

    websocketpp::lib::error_code ec;

    this->ws_client.send(this->handle, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        EVLOG_error << "Error sending message over plain websocket: " << ec.message();

        this->failed_callback(WebsocketCloseReason::AbnormalClose); // TODO!
        EVLOG_info << "(plain) Called reconnect()";
        return false;
    }

    EVLOG_debug << "Sent message over plain websocket: " << message;

    return true;
}

void WebsocketPlain::reconnect() {
    if (this->shutting_down) {
        EVLOG_info << "Not reconnecting because the websocket is being shutdown.";
        return;
    }

    this->connect_plain();

    // // TODO(kai): notify message queue that connection is down and a reconnect is imminent?

    // // TODO(kai): complete error handling, especially making sure that a reconnect is only attempted in reasonable
    // // circumstances

    // // TODO: spec-conform reconnect, refer to status codes from:
    // // https://github.com/zaphoyd/websocketpp/blob/master/websocketpp/close.hpp
}

void WebsocketPlain::connect_plain() {
    websocketpp::lib::error_code ec;

    const client::connection_ptr con = this->ws_client.get_connection(
        std::make_shared<websocketpp::uri>(this->connection_options.csms_uri.get_websocketpp_uri()), ec);

    if (ec) {
        EVLOG_error << "Connection initialization error for plain websocket: " << ec.message();
    }

    if (this->connection_options.hostName.has_value()) {
        EVLOG_info << "User-Host is set to " << this->connection_options.hostName.value();
        con->append_header("User-Host", this->connection_options.hostName.value());
    }

    if (this->connection_options.security_profile == 0) {
        EVLOG_debug << "Connecting with security profile: 0";
    } else if (this->connection_options.security_profile == 1) {
        EVLOG_debug << "Connecting with security profile: 1";
        std::optional<std::string> authorization_header = this->getAuthorizationHeader();
        if (authorization_header) {
            con->append_header("Authorization", authorization_header.value());
        } else {
            throw std::runtime_error("No authorization key provided when connecting with security profile: 1");
        }
    } else {
        throw std::runtime_error("Cannot connect with plain websocket with security profile > 1");
    }

    this->handle = con->get_handle();

    con->set_open_handler(websocketpp::lib::bind(&WebsocketPlain::on_open_plain, this, &this->ws_client,
                                                 websocketpp::lib::placeholders::_1));
    con->set_fail_handler(websocketpp::lib::bind(&WebsocketPlain::on_fail_plain, this, &this->ws_client,
                                                 websocketpp::lib::placeholders::_1));
    con->set_close_handler(websocketpp::lib::bind(&WebsocketPlain::on_close_plain, this, &this->ws_client,
                                                  websocketpp::lib::placeholders::_1));
    con->set_message_handler(websocketpp::lib::bind(&WebsocketPlain::on_message_plain, this,
                                                    websocketpp::lib::placeholders::_1,
                                                    websocketpp::lib::placeholders::_2));
    con->set_pong_timeout(this->connection_options.pong_timeout_s * 1000); // pong timeout in ms
    con->set_pong_timeout_handler(websocketpp::lib::bind(&WebsocketPlain::on_pong_timeout, this,
                                                         websocketpp::lib::placeholders::_1,
                                                         websocketpp::lib::placeholders::_2));

    con->add_subprotocol(conversions::ocpp_protocol_version_to_string(this->connection_options.ocpp_version));
    std::lock_guard<std::mutex> lk(this->connection_mutex);
    this->ws_client.connect(con);
}

void WebsocketPlain::on_open_plain(client* c, websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lk(this->connection_mutex);
    (void)c; // client is not used in this function
    EVLOG_info << "OCPP client successfully connected to plain websocket server";
    this->m_is_connected = true;
    this->set_websocket_ping_interval(this->connection_options.ping_interval_s);
    this->connected_callback(this->connection_options.security_profile);
}

void WebsocketPlain::on_message_plain(websocketpp::connection_hdl hdl, client::message_ptr msg) {
    if (!this->initialized()) {
        EVLOG_error << "Message received but plain websocket has not been correctly initialized. Discarding message.";
        return;
    }
    try {
        auto message = msg->get_payload();
        this->message_callback(message);
    } catch (websocketpp::exception const& e) {
        EVLOG_error << "Plain websocket exception on receiving message: " << e.what();
    }
}

void WebsocketPlain::on_close_plain(client* c, websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lk(this->connection_mutex);
    this->m_is_connected = false;
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    auto error_code = con->get_ec();
    const auto remote_close_code = con->get_remote_close_code();
    EVLOG_info << "Closed plain websocket connection with code: " << error_code << " ("
               << websocketpp::close::status::get_string(remote_close_code)
               << "), reason: " << con->get_remote_close_reason();
    // dont reconnect on normal code
    if (remote_close_code != websocketpp::close::status::normal) {
        this->closed_callback(WebsocketCloseReason::AbnormalClose);
    } else {
        this->closed_callback(value_to_close_reason(remote_close_code));
    }
}

void WebsocketPlain::on_fail_plain(client* c, websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lk(this->connection_mutex);
    this->m_is_connected = false;
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    const auto ec = con->get_ec();
    this->log_on_fail(ec, con->get_transport_ec(), con->get_response_code());

    this->failed_callback(value_to_close_reason(con->get_remote_close_code()));
}

void WebsocketPlain::close(WebsocketCloseReason code, const std::string& reason, const bool stop_perpetual) {
    if (closed) {
        // Already closed...
        return;
    }

    EVLOG_info << "Closing plain websocket.";
    websocketpp::lib::error_code ec;

    if (stop_perpetual) {
        this->ws_client.stop_perpetual();
    }

    this->closed = true;

    this->ws_client.close(this->handle, close_reason_to_value(code), reason, ec);
    if (ec) {
        EVLOG_error << "Error initiating close of plain websocket: " << ec.message();
        // on_close_plain won't be called here so we have to call the closed_callback manually
        this->closed_callback(WebsocketCloseReason::AbnormalClose);
    } else {
        EVLOG_info << "Closed plain websocket successfully.";
    }
}

void WebsocketPlain::ping() {
    if (this->m_is_connected) {
        auto con = this->ws_client.get_con_from_hdl(this->handle);
        websocketpp::lib::error_code error_code;
        con->ping(this->connection_options.ping_payload, error_code);
    }
}

} // namespace ocpp
