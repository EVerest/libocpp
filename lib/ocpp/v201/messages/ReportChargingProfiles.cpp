// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/ReportChargingProfiles.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string ReportChargingProfilesRequest::get_type() const {
    return "ReportChargingProfiles";
}

void to_json(json& j, const ReportChargingProfilesRequest& k) {
    // the required parts of the message
    j = json{
        {"requestId", k.requestId},
        {"chargingLimitSource", conversions::charging_limit_source_enum_to_string(k.chargingLimitSource)},
        {"chargingProfile", k.chargingProfile},
        {"evseId", k.evseId},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.tbc) {
        j["tbc"] = k.tbc.value();
    }
}

void from_json(const json& j, ReportChargingProfilesRequest& k) {
    // the required parts of the message
    k.requestId = j.at("requestId");
    k.chargingLimitSource = conversions::string_to_charging_limit_source_enum(j.at("chargingLimitSource"));
    for (auto val : j.at("chargingProfile")) {
        k.chargingProfile.push_back(val);
    }
    k.evseId = j.at("evseId");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("tbc")) {
        k.tbc.emplace(j.at("tbc"));
    }
}

/// \brief Writes the string representation of the given ReportChargingProfilesRequest \p k to the given output stream
/// \p os \returns an output stream with the ReportChargingProfilesRequest written to
std::ostream& operator<<(std::ostream& os, const ReportChargingProfilesRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string ReportChargingProfilesResponse::get_type() const {
    return "ReportChargingProfilesResponse";
}

void to_json(json& j, const ReportChargingProfilesResponse& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, ReportChargingProfilesResponse& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given ReportChargingProfilesResponse \p k to the given output stream
/// \p os \returns an output stream with the ReportChargingProfilesResponse written to
std::ostream& operator<<(std::ostream& os, const ReportChargingProfilesResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
