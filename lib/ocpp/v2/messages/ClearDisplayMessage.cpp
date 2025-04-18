// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v2/messages/ClearDisplayMessage.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v2 {

std::string ClearDisplayMessageRequest::get_type() const {
    return "ClearDisplayMessage";
}

void to_json(json& j, const ClearDisplayMessageRequest& k) {
    // the required parts of the message
    j = json{
        {"id", k.id},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, ClearDisplayMessageRequest& k) {
    // the required parts of the message
    k.id = j.at("id");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given ClearDisplayMessageRequest \p k to the given output stream \p
/// os \returns an output stream with the ClearDisplayMessageRequest written to
std::ostream& operator<<(std::ostream& os, const ClearDisplayMessageRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string ClearDisplayMessageResponse::get_type() const {
    return "ClearDisplayMessageResponse";
}

void to_json(json& j, const ClearDisplayMessageResponse& k) {
    // the required parts of the message
    j = json{
        {"status", conversions::clear_message_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, ClearDisplayMessageResponse& k) {
    // the required parts of the message
    k.status = conversions::string_to_clear_message_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given ClearDisplayMessageResponse \p k to the given output stream \p
/// os \returns an output stream with the ClearDisplayMessageResponse written to
std::ostream& operator<<(std::ostream& os, const ClearDisplayMessageResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v2
} // namespace ocpp
