// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/SecurityEventNotification.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string SecurityEventNotificationRequest::get_type() const {
    return "SecurityEventNotification";
}

void to_json(json& j, const SecurityEventNotificationRequest& k) {
    // the required parts of the message
    j = json{
        {"type", k.type},
        {"timestamp", k.timestamp.to_rfc3339()},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.techInfo) {
        j["techInfo"] = k.techInfo.value();
    }
}

void from_json(const json& j, SecurityEventNotificationRequest& k) {
    // the required parts of the message
    k.type = j.at("type");
    k.timestamp = ocpp::DateTime(std::string(j.at("timestamp")));

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("techInfo")) {
        k.techInfo.emplace(j.at("techInfo"));
    }
}

/// \brief Writes the string representation of the given SecurityEventNotificationRequest \p k to the given output
/// stream \p os \returns an output stream with the SecurityEventNotificationRequest written to
std::ostream& operator<<(std::ostream& os, const SecurityEventNotificationRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string SecurityEventNotificationResponse::get_type() const {
    return "SecurityEventNotificationResponse";
}

void to_json(json& j, const SecurityEventNotificationResponse& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, SecurityEventNotificationResponse& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given SecurityEventNotificationResponse \p k to the given output
/// stream \p os \returns an output stream with the SecurityEventNotificationResponse written to
std::ostream& operator<<(std::ostream& os, const SecurityEventNotificationResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
