// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v21/messages/GetCRL.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v21 {

std::string GetCRLRequest::get_type() const {
    return "GetCRL";
}

void to_json(json& j, const GetCRLRequest& k) {
    // the required parts of the message
    j = json{
        {"certificateHashData", k.certificateHashData},
        {"requestId", k.requestId},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetCRLRequest& k) {
    // the required parts of the message
    k.certificateHashData = j.at("certificateHashData");
    k.requestId = j.at("requestId");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetCRLRequest \p k to the given output stream \p os
/// \returns an output stream with the GetCRLRequest written to
std::ostream& operator<<(std::ostream& os, const GetCRLRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetCRLResponse::get_type() const {
    return "GetCRLResponse";
}

void to_json(json& j, const GetCRLResponse& k) {
    // the required parts of the message
    j = json{
        {"requestId", k.requestId},
        {"status", ocpp::v201::conversions::generic_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetCRLResponse& k) {
    // the required parts of the message
    k.requestId = j.at("requestId");
    k.status = ocpp::v201::conversions::string_to_generic_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetCRLResponse \p k to the given output stream \p os
/// \returns an output stream with the GetCRLResponse written to
std::ostream& operator<<(std::ostream& os, const GetCRLResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v21
} // namespace ocpp
