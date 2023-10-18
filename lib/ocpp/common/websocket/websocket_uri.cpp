// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest

#include "ocpp/types/simple.hpp"
#include <ocpp/common/websocket/websocket_uri.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>
#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

Uri Uri::parse_from_string(std::string const& uri, ChargepointId chargepoint_id) {
    auto uri_temp = websocketpp::uri(uri);

    if (!uri_temp.get_valid()) {
        throw std::invalid_argument("Uri constructor: given `uri` is invalid");
    }

    const auto& hostname = uri_temp.get_host();
    if (!hostname.empty()) {
        if (hostname != chargepoint_id) {
            throw std::invalid_argument(
                "Uri constructor: the chargepoint-ID in the `uri`-path is different to the defined one");
        }

        chargepoint_id = hostname;
    }

    auto uri_return = Uri(uri_temp.get_secure(), uri_temp.get_host(), uri_temp.get_port(), uri_temp.get_resource());

    return uri_return;
}

} // namespace ocpp