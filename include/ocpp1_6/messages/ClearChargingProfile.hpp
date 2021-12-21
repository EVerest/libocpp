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
#ifndef OCPP1_6_CLEARCHARGINGPROFILE_HPP
#define OCPP1_6_CLEARCHARGINGPROFILE_HPP

#include <ocpp1_6/ocpp_types.hpp>

namespace ocpp1_6 {
struct ClearChargingProfileRequest : public Message {
    boost::optional<int32_t> id;
    boost::optional<int32_t> connectorId;
    boost::optional<ChargingProfilePurposeType> chargingProfilePurpose;
    boost::optional<int32_t> stackLevel;

    std::string get_type() const {
        return "ClearChargingProfile";
    }

    friend void to_json(json& j, const ClearChargingProfileRequest& k) {
        // the required parts of the message
        j = json({});
        // the optional parts of the message
        if (k.id) {
            j["id"] = k.id.value();
        }
        if (k.connectorId) {
            j["connectorId"] = k.connectorId.value();
        }
        if (k.chargingProfilePurpose) {
            j["chargingProfilePurpose"] =
                conversions::charging_profile_purpose_type_to_string(k.chargingProfilePurpose.value());
        }
        if (k.stackLevel) {
            j["stackLevel"] = k.stackLevel.value();
        }
    }

    friend void from_json(const json& j, ClearChargingProfileRequest& k) {
        // the required parts of the message

        // the optional parts of the message
        if (j.contains("id")) {
            k.id.emplace(j.at("id"));
        }
        if (j.contains("connectorId")) {
            k.connectorId.emplace(j.at("connectorId"));
        }
        if (j.contains("chargingProfilePurpose")) {
            k.chargingProfilePurpose.emplace(
                conversions::string_to_charging_profile_purpose_type(j.at("chargingProfilePurpose")));
        }
        if (j.contains("stackLevel")) {
            k.stackLevel.emplace(j.at("stackLevel"));
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const ClearChargingProfileRequest& k) {
        os << json(k).dump(4);
        return os;
    }
};

struct ClearChargingProfileResponse : public Message {
    ClearChargingProfileStatus status;

    std::string get_type() const {
        return "ClearChargingProfileResponse";
    }

    friend void to_json(json& j, const ClearChargingProfileResponse& k) {
        // the required parts of the message
        j = json{
            {"status", conversions::clear_charging_profile_status_to_string(k.status)},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, ClearChargingProfileResponse& k) {
        // the required parts of the message
        k.status = conversions::string_to_clear_charging_profile_status(j.at("status"));

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const ClearChargingProfileResponse& k) {
        os << json(k).dump(4);
        return os;
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_CLEARCHARGINGPROFILE_HPP
