// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_COMMON_HPP
#define OCPP_WEBSOCKET_COMMON_HPP

#include <string>

namespace ocpp {

class Uri {
    public:
        Uri();
        void set(std::string uri);
        std::string string();

        void append_path(std::string path);
        std::string get_hostname();
    
    private:
        std::string value;
};

} // namespace ocpp

#endif /* OCPP_WEBSOCKET_COMMON_HPP */