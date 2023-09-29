// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/websocket/websocket_common.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <string>

namespace ocpp {

Uri::Uri() {
}

void Uri::set(std::string uri) {
    this->value = uri;
}

std::string Uri::string() {
    return this->value;
}

void Uri::append_path(std::string path) {
    // remove ending "/" from `uri`

    if ( boost::algorithm::ends_with(this->value, "/") ) {
        this->value.pop_back();
    }

    // add initial "/" to `path`
    if ( ! boost::algorithm::starts_with(path, "/")) {
        path = "/" + path;
    }
    
    this->value.append(path);
}

}