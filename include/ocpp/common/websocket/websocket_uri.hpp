// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_URI_HPP
#define OCPP_WEBSOCKET_URI_HPP

#include <ocpp/types/simple.hpp>
#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

class Uri {
public:
    Uri(){};

    // if a `chargepoint_id` is given, it will be checked that it is the same as the one in the URI-path, if set
    // if invalid, it throws `invalid_argument` exception
    static Uri parse_from_string(std::string const& uri,
                                 ChargepointId chargepoint_id); // TODO should return an `expected<>`

    void set_secure(bool secure) {
        this->secure = secure;
    }

    std::string get_hostname() {
        return this->host;
    }

    std::string string() {
        auto uri = get_websocketpp_uri();
        return uri.str();
    }

    websocketpp::uri get_websocketpp_uri() { // FIXME: wrap needed `websocketpp:uri` functionality inside `Uri`
        return websocketpp::uri(this->secure, this->host, this->port, this->path);
    }

private:
    Uri(bool secure, std::string host, uint16_t port, std::string path) :
        secure(secure), host(host), port(port), path(path) {
    }

    bool secure;
    std::string host;
    uint16_t port;
    std::string path;
};

} // namespace ocpp

#endif // OCPP_WEBSOCKET_URI_HPP */