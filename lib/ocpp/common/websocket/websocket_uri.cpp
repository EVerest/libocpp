// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/types.hpp>
#include <ocpp/common/websocket/websocket_uri.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>
#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

Uri Uri::parse_and_validate(std::string uri, std::string chargepoint_id, int security_profile) {
    // workaround for required schema in `websocketpp::uri()`
    bool scheme_added_workaround = false;
    if (uri.find("://") == std::string::npos) {
        scheme_added_workaround = true;
        uri = "ws://" + uri;
    }

    auto uri_temp = websocketpp::uri(uri);
    if (!uri_temp.get_valid()) {
        throw std::invalid_argument("given `uri` is invalid");
    }

    if (!scheme_added_workaround) {
        switch (security_profile) { // `switch` to lint for unused enum-values
        case security::SecurityProfile::UNSECURED_TRANSPORT_WITH_BASIC_AUTHENTICATION:
            if (uri_temp.get_secure()) {
                throw std::invalid_argument("secure schema in URI does not fit with insecure security-profile");
            }
        case security::SecurityProfile::TLS_WITH_BASIC_AUTHENTICATION:
        case security::SecurityProfile::TLS_WITH_CLIENT_SIDE_CERTIFICATES:
            if (!uri_temp.get_secure()) {
                throw std::invalid_argument("insecure schema in URI does not fit with secure security-profile");
            }
        }
    }

    const auto& path = uri_temp.get_resource();
    if (!path.empty()) {
        if (path != chargepoint_id) {
            throw std::invalid_argument(
                "Uri constructor: the chargepoint-ID in the `uri`-path is different to the defined one");
        }

        chargepoint_id = path;
    }

    return Uri(uri_temp.get_secure(), uri_temp.get_host(), uri_temp.get_port(), chargepoint_id);
}

} // namespace ocpp
