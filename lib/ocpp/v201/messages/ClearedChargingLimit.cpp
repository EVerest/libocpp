// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/ClearedChargingLimit.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string ClearedChargingLimitRequest::get_type() const {
    return "ClearedChargingLimit";
}

void to_json(json& j, const ClearedChargingLimitRequest& k) {
    // the required parts of the message
    j = json{
        {"chargingLimitSource", conversions::charging_limit_source_enum_to_string(k.chargingLimitSource)},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.evseId) {
        j["evseId"] = k.evseId.value();
    }
}

void from_json(const json& j, ClearedChargingLimitRequest& k) {
    // the required parts of the message
    k.chargingLimitSource = conversions::string_to_charging_limit_source_enum(j.at("chargingLimitSource"));

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("evseId")) {
        k.evseId.emplace(j.at("evseId"));
    }
}

/// \brief Writes the string representation of the given ClearedChargingLimitRequest \p k to the given output stream \p
/// os \returns an output stream with the ClearedChargingLimitRequest written to
std::ostream& operator<<(std::ostream& os, const ClearedChargingLimitRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string ClearedChargingLimitResponse::get_type() const {
    return "ClearedChargingLimitResponse";
}

void to_json(json& j, const ClearedChargingLimitResponse& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, ClearedChargingLimitResponse& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given ClearedChargingLimitResponse \p k to the given output stream \p
/// os \returns an output stream with the ClearedChargingLimitResponse written to
std::ostream& operator<<(std::ostream& os, const ClearedChargingLimitResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
