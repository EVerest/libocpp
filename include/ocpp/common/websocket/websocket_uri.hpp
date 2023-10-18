// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_URI_HPP
#define OCPP_WEBSOCKET_URI_HPP

#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

class Uri {
public:
    Uri();

    void set_path(std::string const& path);

    std::string get_hostname() {
    }

    websocketpp::uri get_websocketpp_uri() {
    }

private:
};

} // namespace ocpp

#endif // OCPP_WEBSOCKET_URI_HPP */