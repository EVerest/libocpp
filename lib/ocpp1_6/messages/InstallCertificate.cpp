// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <ostream>
#include <string>

#include <nlohmann/json.hpp>

#include <ocpp1_6/messages/InstallCertificate.hpp>
#include <ocpp1_6/ocpp_types.hpp>

using json = nlohmann::json;

namespace ocpp1_6 {

std::string InstallCertificateRequest::get_type() const {
    return "InstallCertificate";
}

void to_json(json& j, const InstallCertificateRequest& k) {
    // the required parts of the message
    j = json{
        {"certificateType", k.certificateType},
        {"certificate", k.certificate},
    };
    // the optional parts of the message
}

void from_json(const json& j, InstallCertificateRequest& k) {
    // the required parts of the message
    k.certificateType = j.at("certificateType");
    k.certificate = j.at("certificate");

    // the optional parts of the message
}

/// \brief Writes the string representation of the given InstallCertificateRequest \p k to the given output stream \p os
/// \returns an output stream with the InstallCertificateRequest written to
std::ostream& operator<<(std::ostream& os, const InstallCertificateRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string InstallCertificateResponse::get_type() const {
    return "InstallCertificateResponse";
}

void to_json(json& j, const InstallCertificateResponse& k) {
    // the required parts of the message
    j = json{
        {"status", k.status},
    };
    // the optional parts of the message
}

void from_json(const json& j, InstallCertificateResponse& k) {
    // the required parts of the message
    k.status = j.at("status");

    // the optional parts of the message
}

/// \brief Writes the string representation of the given InstallCertificateResponse \p k to the given output stream \p
/// os \returns an output stream with the InstallCertificateResponse written to
std::ostream& operator<<(std::ostream& os, const InstallCertificateResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace ocpp1_6
