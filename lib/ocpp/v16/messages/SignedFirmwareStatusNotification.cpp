// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v16/enums.hpp>
#include <ocpp/v16/messages/SignedFirmwareStatusNotification.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp::v16 {

std::string SignedFirmwareStatusNotificationRequest::get_type() const {
    return "SignedFirmwareStatusNotification";
}

void to_json(json& j, const SignedFirmwareStatusNotificationRequest& k) {
    // the required parts of the message
    j = json{
        {"status", k.status.string()},
    };
    // the optional parts of the message
    if (k.requestId) {
        j["requestId"] = k.requestId.value();
    }
}

void from_json(const json& j, SignedFirmwareStatusNotificationRequest& k) {
    // the required parts of the message
    k.status = FirmwareStatusEnumType(j.at("status"));

    // the optional parts of the message
    if (j.contains("requestId")) {
        k.requestId.emplace(j.at("requestId"));
    }
}

/// \brief Writes the string representation of the given SignedFirmwareStatusNotificationRequest \p k to the given
/// output stream \p os \returns an output stream with the SignedFirmwareStatusNotificationRequest written to
std::ostream& operator<<(std::ostream& os, const SignedFirmwareStatusNotificationRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string SignedFirmwareStatusNotificationResponse::get_type() const {
    return "SignedFirmwareStatusNotificationResponse";
}

void to_json(json& j, const SignedFirmwareStatusNotificationResponse& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    (void)k; // no elements to unpack, silence unused parameter warning
}

void from_json(const json& j, SignedFirmwareStatusNotificationResponse& k) {
    // the required parts of the message

    // the optional parts of the message
    // no elements to unpack, silence unused parameter warning
    (void)j;
    (void)k;
}

/// \brief Writes the string representation of the given SignedFirmwareStatusNotificationResponse \p k to the given
/// output stream \p os \returns an output stream with the SignedFirmwareStatusNotificationResponse written to
std::ostream& operator<<(std::ostream& os, const SignedFirmwareStatusNotificationResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace ocpp::v16
