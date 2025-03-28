// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v2/messages/CertificateSigned.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v2 {

std::string CertificateSignedRequest::get_type() const {
    return "CertificateSigned";
}

void to_json(json& j, const CertificateSignedRequest& k) {
    // the required parts of the message
    j = json{
        {"certificateChain", k.certificateChain},
    };
    // the optional parts of the message
    if (k.certificateType) {
        j["certificateType"] = conversions::certificate_signing_use_enum_to_string(k.certificateType.value());
    }
    if (k.requestId) {
        j["requestId"] = k.requestId.value();
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, CertificateSignedRequest& k) {
    // the required parts of the message
    k.certificateChain = j.at("certificateChain");

    // the optional parts of the message
    if (j.contains("certificateType")) {
        k.certificateType.emplace(conversions::string_to_certificate_signing_use_enum(j.at("certificateType")));
    }
    if (j.contains("requestId")) {
        k.requestId.emplace(j.at("requestId"));
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given CertificateSignedRequest \p k to the given output stream \p os
/// \returns an output stream with the CertificateSignedRequest written to
std::ostream& operator<<(std::ostream& os, const CertificateSignedRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string CertificateSignedResponse::get_type() const {
    return "CertificateSignedResponse";
}

void to_json(json& j, const CertificateSignedResponse& k) {
    // the required parts of the message
    j = json{
        {"status", conversions::certificate_signed_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, CertificateSignedResponse& k) {
    // the required parts of the message
    k.status = conversions::string_to_certificate_signed_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given CertificateSignedResponse \p k to the given output stream \p os
/// \returns an output stream with the CertificateSignedResponse written to
std::ostream& operator<<(std::ostream& os, const CertificateSignedResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v2
} // namespace ocpp
