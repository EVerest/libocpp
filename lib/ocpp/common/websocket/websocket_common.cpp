// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/websocket/websocket_common.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>
#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {

Uri::Uri():
 value(false, "", "") {  // FIXME: Uri() should be constructed directly with real value
}

// if invalid, it throws `invalid_argument` exception
void Uri::set(std::string const & uri) {
    auto uri_temp = websocketpp::uri(uri);

    if (! uri_temp.get_valid()) {
        throw std::invalid_argument("Uri::set(): `uri` is invalid");
    }

    this->value = uri_temp;
}

std::string Uri::string() {
    return this->value.str();
}

void Uri::set_path(std::string const & path) {
    this->value = websocketpp::uri(this->value.get_secure(), this->value.get_host(), this->value.get_port(), path);
}

}  /* namespace ocpp */