// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v2/messages/MeterValues.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v2 {

std::string MeterValuesRequest::get_type() const {
    return "MeterValues";
}

void to_json(json& j, const MeterValuesRequest& k) {
    // the required parts of the message
    j = json{
        {"evseId", k.evseId},
        {"meterValue", k.meterValue},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, MeterValuesRequest& k) {
    // the required parts of the message
    k.evseId = j.at("evseId");
    for (auto val : j.at("meterValue")) {
        k.meterValue.push_back(val);
    }

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given MeterValuesRequest \p k to the given output stream \p os
/// \returns an output stream with the MeterValuesRequest written to
std::ostream& operator<<(std::ostream& os, const MeterValuesRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string MeterValuesResponse::get_type() const {
    return "MeterValuesResponse";
}

void to_json(json& j, const MeterValuesResponse& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, MeterValuesResponse& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given MeterValuesResponse \p k to the given output stream \p os
/// \returns an output stream with the MeterValuesResponse written to
std::ostream& operator<<(std::ostream& os, const MeterValuesResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v2
} // namespace ocpp
