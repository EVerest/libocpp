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
#ifndef OCPP1_6_CHANGEAVAILABILITY_HPP
#define OCPP1_6_CHANGEAVAILABILITY_HPP

#include <ocpp1_6/ocpp_types.hpp>

namespace ocpp1_6 {
struct ChangeAvailabilityRequest : public Message {
    int32_t connectorId;
    AvailabilityType type;

    std::string get_type() const {
        return "ChangeAvailability";
    }

    friend void to_json(json& j, const ChangeAvailabilityRequest& k) {
        // the required parts of the message
        j = json{
            {"connectorId", k.connectorId},
            {"type", conversions::availability_type_to_string(k.type)},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, ChangeAvailabilityRequest& k) {
        // the required parts of the message
        k.connectorId = j.at("connectorId");
        k.type = conversions::string_to_availability_type(j.at("type"));

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const ChangeAvailabilityRequest& k) {
        os << json(k).dump(4);
        return os;
    }
};

struct ChangeAvailabilityResponse : public Message {
    AvailabilityStatus status;

    std::string get_type() const {
        return "ChangeAvailabilityResponse";
    }

    friend void to_json(json& j, const ChangeAvailabilityResponse& k) {
        // the required parts of the message
        j = json{
            {"status", conversions::availability_status_to_string(k.status)},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, ChangeAvailabilityResponse& k) {
        // the required parts of the message
        k.status = conversions::string_to_availability_status(j.at("status"));

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const ChangeAvailabilityResponse& k) {
        os << json(k).dump(4);
        return os;
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_CHANGEAVAILABILITY_HPP
