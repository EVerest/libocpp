// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/logging.hpp>

#include <ocpp1_6/charge_point_configuration.hpp>
#include <ocpp1_6/pki_handler.hpp>
#include <ocpp1_6/websocket/websocket_tls.hpp>

namespace ocpp1_6 {

WebsocketTLS::WebsocketTLS(std::shared_ptr<ChargePointConfiguration> configuration) : WebsocketBase(configuration) {
    this->reconnect_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::seconds(configuration->getWebsocketReconnectInterval()))
                                      .count();
}

bool WebsocketTLS::connect(int32_t security_profile) {
    if (!this->initialized()) {
        return false;
    }
    this->uri = this->configuration->getCentralSystemURI().insert(0, "wss://");
    EVLOG_info << "Connecting TLS websocket to uri: " << this->uri;
    EVLOG_info << "Connecting TLS websocket, hostname: " << this->get_hostname(uri);
    this->wss_client.clear_access_channels(websocketpp::log::alevel::all);
    this->wss_client.clear_error_channels(websocketpp::log::elevel::all);
    this->wss_client.init_asio();
    this->wss_client.start_perpetual();

    this->wss_client.set_tls_init_handler(websocketpp::lib::bind(&WebsocketTLS::on_tls_init, this,
                                                                 this->get_hostname(this->uri),
                                                                 websocketpp::lib::placeholders::_1, security_profile));

    websocket_thread.reset(new websocketpp::lib::thread(&tls_client::run, &this->wss_client));

    this->reconnect_callback = [this, security_profile](const websocketpp::lib::error_code& ec) {
        EVLOG_info << "Reconnecting TLS websocket...";

        // close connection before reconnecting
        if (this->is_connected) {
            try {
                EVLOG_debug << "Closing websocket connection";
                this->wss_client.close(this->handle, websocketpp::close::status::normal, "");
            } catch (std::exception& e) {
                EVLOG_error << "Error on TLS close: " << e.what();
            }
        }

        {
            std::lock_guard<std::mutex> lk(this->reconnect_mutex);
            if (this->reconnect_timer) {
                this->reconnect_timer.get()->cancel();
            }
            this->reconnect_timer = nullptr;
        }
        this->connect_tls(security_profile);
    };

    this->connect_tls(security_profile);
    return true;
}

bool WebsocketTLS::send(const std::string& message) {
    if (!this->initialized()) {
        EVLOG_error << "Could not send message because websocket is not properly initialized.";
        return false;
    }

    websocketpp::lib::error_code ec;

    this->wss_client.send(this->handle, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        EVLOG_error << "Error sending message over TLS websocket: " << ec.message();

        this->reconnect(ec, this->reconnect_interval_ms);
        EVLOG_info << "(TLS) Called reconnect()";
        return false;
    }

    EVLOG_debug << "Sent message over TLS websocket: " << message;

    return true;
}

void WebsocketTLS::reconnect(std::error_code reason, long delay) {
    if (this->shutting_down) {
        EVLOG_info << "Not reconnecting because the websocket is being shutdown.";
        return;
    }

    // TODO(kai): notify message queue that connection is down and a reconnect is imminent?
    {
        std::lock_guard<std::mutex> lk(this->reconnect_mutex);
        if (!this->reconnect_timer) {
            EVLOG_info << "Reconnecting in: " << delay << "ms";
            this->reconnect_timer = this->wss_client.set_timer(delay, this->reconnect_callback);
        } else {
            EVLOG_debug << "Reconnect timer already running";
        }
    }

    // TODO(kai): complete error handling, especially making sure that a reconnect is only attempted in reasonable
    // circumstances
    switch (reason.value()) {
    case websocketpp::close::status::force_tcp_drop:
        /* code */
        break;

    default:
        break;
    }

    // TODO: spec-conform reconnect, refer to status codes from:
    // https://github.com/zaphoyd/websocketpp/blob/master/websocketpp/close.hpp
}

std::string WebsocketTLS::get_hostname(std::string uri) {
    // FIXME(kai): This only works with a very limited subset of hostnames!
    std::string start = "wss://";
    std::string stop = "/";
    std::string port = ":";
    auto hostname_start_pos = start.length();
    auto hostname_end_pos = uri.find_first_of(stop, hostname_start_pos);

    auto hostname_with_port = uri.substr(hostname_start_pos, hostname_end_pos - hostname_start_pos);
    auto port_pos = hostname_with_port.find_first_of(port);
    if (port_pos != std::string::npos) {
        return hostname_with_port.substr(0, port_pos);
    }
    return hostname_with_port;
}

// TLS
bool WebsocketTLS::verify_certificate(std::string hostname, bool preverified,
                                      boost::asio::ssl::verify_context& context) {

    // FIXME(kai): this does not verify anything at the moment!

    EVLOG_debug << "Hostname: " << hostname;
    EVLOG_debug << "Preverified: " << preverified;
    EVLOG_critical << "Faking certificate verification, always returning true";

    return true;
}
tls_context WebsocketTLS::on_tls_init(std::string hostname, websocketpp::connection_hdl hdl, int32_t security_profile) {
    tls_context context = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        // FIXME(kai): choose reasonable defaults, they can probably be stricter than this set of options!
        // it is recommended to only accept TLSv1.2+
        context->set_options(boost::asio::ssl::context::default_workarounds | //
                             boost::asio::ssl::context::no_sslv2 |            //
                             boost::asio::ssl::context::no_sslv3 |            //
                             boost::asio::ssl::context::no_tlsv1 |            //
                             boost::asio::ssl::context::no_tlsv1_1 |          //
                             boost::asio::ssl::context::single_dh_use);

        EVLOG_debug << "List of ciphers that will be accepted by this TLS connection: "
                    << this->configuration->getSupportedCiphers12() << ":"
                    << this->configuration->getSupportedCiphers13();

        auto set_cipher_list_ret =
            SSL_CTX_set_cipher_list(context->native_handle(), this->configuration->getSupportedCiphers12().c_str());
        if (set_cipher_list_ret != 1) {
            EVLOG_critical << "SSL_CTX_set_cipher_list return value: " << set_cipher_list_ret;
            throw std::runtime_error("Could not set TLSv1.2 cipher list");
        }

        set_cipher_list_ret =
            SSL_CTX_set_ciphersuites(context->native_handle(), this->configuration->getSupportedCiphers13().c_str());
        if (set_cipher_list_ret != 1) {
            EVLOG_critical << "SSL_CTX_set_cipher_list return value: " << set_cipher_list_ret;
            throw std::runtime_error("Could not set TLSv1.3 cipher list");
        }

        if (security_profile == 3) {
            SSL_CTX_use_certificate(
                context->native_handle(),
                load_from_file(this->configuration->getPkiHandler()->getFile(CLIENT_SIDE_CERTIFICATE_FILE))->x509);
        }

        context->set_verify_mode(boost::asio::ssl::verify_peer);
        SSL_CTX_load_verify_locations(context->native_handle(),
                                      this->configuration->getPkiHandler()->getFile(CS_ROOT_CA_FILE).c_str(), NULL);

    } catch (std::exception& e) {
        EVLOG_error << "Error on TLS init: " << e.what();
        throw std::runtime_error("Could not properly initialize TLS connection.");
    }
    return context;
}
void WebsocketTLS::connect_tls(int32_t security_profile) {
    EVLOG_info << "Connecting to TLS websocket at: " << this->uri;
    websocketpp::lib::error_code ec;

    tls_client::connection_ptr con = this->wss_client.get_connection(this->uri, ec);

    if (ec) {
        EVLOG_error << "Connection initialization error for TLS websocket: " << ec.message();
        return;
    }

    if (security_profile == 2) {
        EVLOG_debug << "Connecting with security profile: 2";
        boost::optional<std::string> authorization_header = this->getAuthorizationHeader();
        if (authorization_header != boost::none) {
            con->append_header("Authorization", authorization_header.get());
        } else {
            throw std::runtime_error("No authorization key provided when connecting with security profile 2 or 3.");
        }
    } else if (security_profile == 3) {
        EVLOG_debug << "Connecting with security profile: 3";
    } else {
        throw std::runtime_error("Can not connect with TLS websocket with security profile not being 2 or 3.");
    }

    this->handle = con->get_handle();

    con->set_open_handler(websocketpp::lib::bind(&WebsocketTLS::on_open_tls, this, &this->wss_client,
                                                 websocketpp::lib::placeholders::_1, security_profile));
    con->set_fail_handler(websocketpp::lib::bind(&WebsocketTLS::on_fail_tls, this, &this->wss_client,
                                                 websocketpp::lib::placeholders::_1,
                                                 security_profile != this->configuration->getSecurityProfile()));
    con->set_close_handler(websocketpp::lib::bind(&WebsocketTLS::on_close_tls, this, &this->wss_client,
                                                  websocketpp::lib::placeholders::_1));
    con->set_message_handler(websocketpp::lib::bind(
        &WebsocketTLS::on_message_tls, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

    con->add_subprotocol("ocpp1.6");

    this->wss_client.connect(con);
}
void WebsocketTLS::on_open_tls(tls_client* c, websocketpp::connection_hdl hdl, int32_t security_profile) {
    EVLOG_info << "Connected to TLS websocket successfully. Executing connected callback";
    this->is_connected = true;
    this->configuration->setSecurityProfile(2);
    this->connected_callback();
}
void WebsocketTLS::on_message_tls(websocketpp::connection_hdl hdl, tls_client::message_ptr msg) {
    if (!this->initialized()) {
        EVLOG_error << "Message received but TLS websocket has not been correctly initialized. Discarding message.";
        return;
    }
    try {
        auto message = msg->get_payload();
        this->message_callback(message);
    } catch (websocketpp::exception const& e) {
        EVLOG_error << "TLS websocket exception on receiving message: " << e.what();
    }
}
void WebsocketTLS::on_close_tls(tls_client* c, websocketpp::connection_hdl hdl) {
    this->is_connected = false;
    tls_client::connection_ptr con = c->get_con_from_hdl(hdl);
    auto error_code = con->get_ec();

    EVLOG_info << "Closed TLS websocket connection with code: " << error_code << " ("
               << websocketpp::close::status::get_string(con->get_remote_close_code())
               << "), reason: " << con->get_remote_close_reason();
    // dont reconnect on normal close
    if (error_code != std::error_code()) {
        this->reconnect(error_code, this->reconnect_interval_ms);
    }
    this->disconnected_callback();
}
void WebsocketTLS::on_fail_tls(tls_client* c, websocketpp::connection_hdl hdl, bool try_once) {
    tls_client::connection_ptr con = c->get_con_from_hdl(hdl);
    auto error_code = con->get_ec();
    auto transport_ec = con->get_transport_ec();
    EVLOG_error << "Failed to connect to TLS websocket server " << con->get_response_header("Server")
                << ", code: " << error_code.value() << ", reason: " << error_code.message();
    EVLOG_error << "Failed to connect to TLS websocket server "
                << ", code: " << transport_ec.value() << ", reason: " << transport_ec.message()
                << ", category: " << transport_ec.category().name();

    // move fallback ca to /certs if it exists
    if (boost::filesystem::exists(CS_ROOT_CA_FILE_BACKUP_FILE)) {
        EVLOG_debug << "Connection with new CA was not successful - Including fallback CA";
        boost::filesystem::path new_path =
            CS_ROOT_CA_FILE_BACKUP_FILE.parent_path() / CS_ROOT_CA_FILE_BACKUP_FILE.filename();
        std::rename(CS_ROOT_CA_FILE_BACKUP_FILE.c_str(), new_path.c_str());
    }

    if (!try_once) {
        this->reconnect(error_code, this->reconnect_interval_ms);
    } else {
        this->disconnected_callback();
    }
}

} // namespace ocpp1_6
