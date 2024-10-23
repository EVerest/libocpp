// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v21/messages/NotifyPeriodicEventStream.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v21 {

std::string NotifyPeriodicEventStreamRequest::get_type() const {
    return "NotifyPeriodicEventStream";
}

void to_json(json& j, const NotifyPeriodicEventStreamRequest& k) {
    // the required parts of the message
    j = json{
        {"data", k.data},
        {"id", k.id},
        {"pending", k.pending},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, NotifyPeriodicEventStreamRequest& k) {
    // the required parts of the message
    for (auto val : j.at("data")) {
        k.data.push_back(val);
    }
    k.id = j.at("id");
    k.pending = j.at("pending");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given NotifyPeriodicEventStreamRequest \p k to the given output
/// stream \p os \returns an output stream with the NotifyPeriodicEventStreamRequest written to
std::ostream& operator<<(std::ostream& os, const NotifyPeriodicEventStreamRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string NotifyPeriodicEventStreamResponse::get_type() const {
    return "NotifyPeriodicEventStreamResponse";
}

void to_json(json& j, const NotifyPeriodicEventStreamResponse& k) {
    // the required parts of the message
    j = json{
        {"status", ocpp::v201::conversions::generic_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
}

void from_json(const json& j, NotifyPeriodicEventStreamResponse& k) {
    // the required parts of the message
    k.status = ocpp::v201::conversions::string_to_generic_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
}

/// \brief Writes the string representation of the given NotifyPeriodicEventStreamResponse \p k to the given output
/// stream \p os \returns an output stream with the NotifyPeriodicEventStreamResponse written to
std::ostream& operator<<(std::ostream& os, const NotifyPeriodicEventStreamResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v21
} // namespace ocpp
