/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef OCPP1_6_CHANGECONFIGURATION_HPP
#define OCPP1_6_CHANGECONFIGURATION_HPP

#include <ocpp1_6/ocpp_types.hpp>

namespace ocpp1_6 {
struct ChangeConfigurationRequest : public Message {
    CiString50Type key;
    CiString500Type value;

    std::string get_type() const {
        return "ChangeConfiguration";
    }

    friend void to_json(json& j, const ChangeConfigurationRequest& k) {
        // the required parts of the message
        j = json{
            {"key", k.key},
            {"value", k.value},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, ChangeConfigurationRequest& k) {
        // the required parts of the message
        k.key = j.at("key");
        k.value = j.at("value");

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const ChangeConfigurationRequest& k) {
        os << json(k).dump(4);
        return os;
    }
};

struct ChangeConfigurationResponse : public Message {
    ConfigurationStatus status;

    std::string get_type() const {
        return "ChangeConfigurationResponse";
    }

    friend void to_json(json& j, const ChangeConfigurationResponse& k) {
        // the required parts of the message
        j = json{
            {"status", conversions::configuration_status_to_string(k.status)},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, ChangeConfigurationResponse& k) {
        // the required parts of the message
        k.status = conversions::string_to_configuration_status(j.at("status"));

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const ChangeConfigurationResponse& k) {
        os << json(k).dump(4);
        return os;
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_CHANGECONFIGURATION_HPP
