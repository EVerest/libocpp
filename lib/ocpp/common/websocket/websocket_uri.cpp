// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/websocket/websocket_uri.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <stdexcept>
#include <string>
#include <websocketpp/uri.hpp>

namespace ocpp {


// if invalid, it throws `invalid_argument` exception


}

std::string Uri::string() {
    return this->value.str();
}

void Uri::set_path(std::string const & path) {
    this->value = websocketpp::uri(this->value.get_secure(), this->value.get_host(), this->value.get_port(), path);
}

}  /* namespace ocpp */