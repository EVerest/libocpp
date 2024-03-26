// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <optional>

#include <ocpp/v201/messages/GetCertificateStatus.hpp>

using json = nlohmann::json;

namespace ocpp {
namespace v201 {

std::string GetCertificateStatusRequest::get_type() const {
    return "GetCertificateStatus";
}

void to_json(json& j, const GetCertificateStatusRequest& k) {
    // the required parts of the message
    j = json{
        {"ocspRequestData", k.ocspRequestData},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetCertificateStatusRequest& k) {
    // the required parts of the message
    k.ocspRequestData = j.at("ocspRequestData");

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetCertificateStatusRequest \p k to the given output stream \p
/// os \returns an output stream with the GetCertificateStatusRequest written to
std::ostream& operator<<(std::ostream& os, const GetCertificateStatusRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetCertificateStatusResponse::get_type() const {
    return "GetCertificateStatusResponse";
}

void to_json(json& j, const GetCertificateStatusResponse& k) {
    // the required parts of the message
    j = json{
        {"status", conversions::get_certificate_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
    if (k.ocspResult) {
        j["ocspResult"] = k.ocspResult.value();
    }
}

void from_json(const json& j, GetCertificateStatusResponse& k) {
    // the required parts of the message
    k.status = conversions::string_to_get_certificate_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
    if (j.contains("ocspResult")) {
        k.ocspResult.emplace(j.at("ocspResult"));
    }
}

/// \brief Writes the string representation of the given GetCertificateStatusResponse \p k to the given output stream \p
/// os \returns an output stream with the GetCertificateStatusResponse written to
std::ostream& operator<<(std::ostream& os, const GetCertificateStatusResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v201
} // namespace ocpp
