// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/GetChargingProfiles.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string GetChargingProfilesRequest::get_type() const {
    return "GetChargingProfiles";
}

void to_json(json& j, const GetChargingProfilesRequest& k) {
    // the required parts of the message
    j = json{
        {"requestId", k.requestId},
        {"chargingProfile", k.chargingProfile},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.evseId) {
        j["evseId"] = k.evseId.value();
    }
}

void from_json(const json& j, GetChargingProfilesRequest& k) {
    // the required parts of the message
    k.requestId = j.at("requestId");
    k.chargingProfile = j.at("chargingProfile");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("evseId")) {
        k.evseId.emplace(j.at("evseId"));
    }
}

/// \brief Writes the string representation of the given GetChargingProfilesRequest \p k to the given output stream \p
/// os \returns an output stream with the GetChargingProfilesRequest written to
std::ostream& operator<<(std::ostream& os, const GetChargingProfilesRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetChargingProfilesResponse::get_type() const {
    return "GetChargingProfilesResponse";
}

void to_json(json& j, const GetChargingProfilesResponse& k) {
    // the required parts of the message
    j = json{
        {"status", conversions::get_charging_profile_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
}

void from_json(const json& j, GetChargingProfilesResponse& k) {
    // the required parts of the message
    k.status = conversions::string_to_get_charging_profile_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
}

/// \brief Writes the string representation of the given GetChargingProfilesResponse \p k to the given output stream \p
/// os \returns an output stream with the GetChargingProfilesResponse written to
std::ostream& operator<<(std::ostream& os, const GetChargingProfilesResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
