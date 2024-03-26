// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/GetTransactionStatus.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string GetTransactionStatusRequest::get_type() const {
    return "GetTransactionStatus";
}

void to_json(json& j, const GetTransactionStatusRequest& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.transactionId) {
        j["transactionId"] = k.transactionId.value();
    }
}

void from_json(const json& j, GetTransactionStatusRequest& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("transactionId")) {
        k.transactionId.emplace(j.at("transactionId"));
    }
}

/// \brief Writes the string representation of the given GetTransactionStatusRequest \p k to the given output stream \p
/// os \returns an output stream with the GetTransactionStatusRequest written to
std::ostream& operator<<(std::ostream& os, const GetTransactionStatusRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetTransactionStatusResponse::get_type() const {
    return "GetTransactionStatusResponse";
}

void to_json(json& j, const GetTransactionStatusResponse& k) {
    // the required parts of the message
    j = json{
        {"messagesInQueue", k.messagesInQueue},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.ongoingIndicator) {
        j["ongoingIndicator"] = k.ongoingIndicator.value();
    }
}

void from_json(const json& j, GetTransactionStatusResponse& k) {
    // the required parts of the message
    k.messagesInQueue = j.at("messagesInQueue");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("ongoingIndicator")) {
        k.ongoingIndicator.emplace(j.at("ongoingIndicator"));
    }
}

/// \brief Writes the string representation of the given GetTransactionStatusResponse \p k to the given output stream \p
/// os \returns an output stream with the GetTransactionStatusResponse written to
std::ostream& operator<<(std::ostream& os, const GetTransactionStatusResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
