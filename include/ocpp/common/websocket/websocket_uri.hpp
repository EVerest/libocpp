// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_URI_HPP
#define OCPP_WEBSOCKET_URI_HPP

#include <string>
#include <string_view>
#include <websocketpp/uri.hpp>

namespace ocpp {

class Uri {
public:
    Uri(){};

    // parse_and_validate parses the URI and checks
    // 1. general validity of the URI
    // 2. scheme fits to given `security_profile`
    //
    // Backwards-compatibility: The path (of the URI) can contain the `chargepoint_id` as last segment.
    //
    // It throws `std::invalid_argument` for several checks
    static Uri parse_and_validate(std::string uri, std::string chargepoint_id, int security_profile);

    void set_secure(bool secure) {
        this->secure = secure;
    }

    std::string get_hostname() {
        return this->host;
    }
    std::string get_chargepoint_id() {
        return this->chargepoint_id;
    }

    std::string string() {
        auto uri = get_websocketpp_uri();
        return uri.str();
    }

    websocketpp::uri get_websocketpp_uri() { // FIXME: wrap needed `websocketpp:uri` functionality inside `Uri`
        return websocketpp::uri(this->secure, this->host, this->port,
                                this->path_without_chargepoint_id /* is normalized with ending slash */ +
                                    this->chargepoint_id);
    }

private:
    Uri(bool secure, const std::string& host, uint16_t port, const std::string& path_without_chargepoint_id,
        const std::string& chargepoint_id) :
        secure(secure),
        host(host),
        port(port),
        path_without_chargepoint_id(path_without_chargepoint_id),
        chargepoint_id(chargepoint_id) {
    }

    bool secure;
    std::string host;
    uint16_t port;
    std::string path_without_chargepoint_id;
    std::string chargepoint_id;
};

} // namespace ocpp

#endif // OCPP_WEBSOCKET_URI_HPP */
