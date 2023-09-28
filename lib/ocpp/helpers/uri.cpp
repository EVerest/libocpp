// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <boost/algorithm/string/predicate.hpp>
#include <string>

namespace helpers {

std::string URIAppendPath(std::string uri, std::string path) {
    // remove ending "/" from `uri`
    if ( boost::algorithm::ends_with(uri, "/") ) {
        uri.pop_back();
    }

    // add initial "/" to `path`
    if ( ! boost::algorithm::starts_with(path, "/")) {
        path = "/" + path;
    }
    
    return uri.append(path);
}

} // namespace helpers