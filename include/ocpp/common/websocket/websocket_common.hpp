// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_COMMON_HPP
#define OCPP_WEBSOCKET_COMMON_HPP

#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

class Uri {
public:
    Uri();
    void set(std::string const& uri);
    std::string string();

    void set_path(std::string const& path);

    std::string get_hostname() {
        return this->value.get_host();
    }

    websocketpp::uri get_websocketpp_uri() {
        return this->value;
    }

private:
    websocketpp::uri value;
};

} // namespace ocpp

#endif /* OCPP_WEBSOCKET_COMMON_HPP */