// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <nlohmann/json.hpp>

#include <ocpp1_6/messages/CertificateSigned.hpp>
#include <ocpp1_6/ocpp_types.hpp>

using json = nlohmann::json;

namespace ocpp1_6 {

std::string CertificateSignedRequest::get_type() const {
    return "CertificateSigned";
}

void to_json(json& j, const CertificateSignedRequest& k) {
    // the required parts of the message
    j = json{
        {"certificateChain", k.certificateChain},
    };
    // the optional parts of the message
}

void from_json(const json& j, CertificateSignedRequest& k) {
    // the required parts of the message
    k.certificateChain = j.at("certificateChain");

    // the optional parts of the message
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
        {"status", k.status},
    };
    // the optional parts of the message
}

void from_json(const json& j, CertificateSignedResponse& k) {
    // the required parts of the message
    k.status = j.at("status");

    // the optional parts of the message
}

/// \brief Writes the string representation of the given CertificateSignedResponse \p k to the given output stream \p os
/// \returns an output stream with the CertificateSignedResponse written to
std::ostream& operator<<(std::ostream& os, const CertificateSignedResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace ocpp1_6
