// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_URI_HPP
#define OCPP_WEBSOCKET_URI_HPP

#include <doctest/doctest.h>
#include <websocketpp/uri.hpp>

#include <string>

namespace ocpp {

class Uri {
public:
    Uri(){};

    // if a `chargepoint_id` is given, it will be checked that it is the same as the one in the URI-path, if set
    // if invalid, it throws `invalid_argument` exception
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
        return websocketpp::uri(this->secure, this->host, this->port, this->chargepoint_id);
    }

private:
    Uri(bool secure, const std::string& host, uint16_t port, const std::string& chargepoint_id) :
        secure(secure), host(host), port(port), chargepoint_id(chargepoint_id) {
    }

    bool secure;
    std::string host;
    uint16_t port;
    std::string chargepoint_id;
};

} // namespace ocpp

namespace ocpp_test {

TEST_CASE("parse_from_string()") {
    auto uri = ocpp::Uri::parse_from_string("wss://test.example.com/cp123", "cp123");
    CHECK(uri.string() == "wss://test.example.com/cp123");
}

} // namespace ocpp_test

#endif // OCPP_WEBSOCKET_URI_HPP */
