// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <boost/optional/optional.hpp>
#include <nlohmann/json.hpp>

#include <ocpp1_6/messages/LogStatusNotification.hpp>
#include <ocpp1_6/ocpp_types.hpp>

using json = nlohmann::json;

namespace ocpp1_6 {

std::string LogStatusNotificationRequest::get_type() const {
    return "LogStatusNotification";
}

void to_json(json& j, const LogStatusNotificationRequest& k) {
    // the required parts of the message
    j = json{
        {"status", k.status},
    };
    // the optional parts of the message
    if (k.requestId) {
        j["requestId"] = k.requestId.value();
    }
}

void from_json(const json& j, LogStatusNotificationRequest& k) {
    // the required parts of the message
    k.status = j.at("status");

    // the optional parts of the message
    if (j.contains("requestId")) {
        k.requestId.emplace(j.at("requestId"));
    }
}

/// \brief Writes the string representation of the given LogStatusNotificationRequest \p k to the given output stream \p
/// os \returns an output stream with the LogStatusNotificationRequest written to
std::ostream& operator<<(std::ostream& os, const LogStatusNotificationRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string LogStatusNotificationResponse::get_type() const {
    return "LogStatusNotificationResponse";
}

void to_json(json& j, const LogStatusNotificationResponse& k) {
    // the required parts of the message
    j = json({});
    // the optional parts of the message
}

void from_json(const json& j, LogStatusNotificationResponse& k) {
    // the required parts of the message

    // the optional parts of the message
}

/// \brief Writes the string representation of the given LogStatusNotificationResponse \p k to the given output stream
/// \p os \returns an output stream with the LogStatusNotificationResponse written to
std::ostream& operator<<(std::ostream& os, const LogStatusNotificationResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace ocpp1_6
