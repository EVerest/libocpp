// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v2/messages/GetInstalledCertificateIds.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v2 {

std::string GetInstalledCertificateIdsRequest::get_type() const {
    return "GetInstalledCertificateIds";
}

void to_json(json& j, const GetInstalledCertificateIdsRequest& k) {
    // the required parts of the message
    j = json({}, true);
    // the optional parts of the message
    if (k.certificateType) {
        if (j.size() == 0) {
            j = json{{"certificateType", json::array()}};
        } else {
            j["certificateType"] = json::array();
        }
        for (auto val : k.certificateType.value()) {
            j["certificateType"].push_back(conversions::get_certificate_id_use_enum_to_string(val));
        }
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetInstalledCertificateIdsRequest& k) {
    // the required parts of the message

    // the optional parts of the message
    if (j.contains("certificateType")) {
        json arr = j.at("certificateType");
        std::vector<GetCertificateIdUseEnum> vec;
        for (auto val : arr) {
            vec.push_back(conversions::string_to_get_certificate_id_use_enum(val));
        }
        k.certificateType.emplace(vec);
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetInstalledCertificateIdsRequest \p k to the given output
/// stream \p os \returns an output stream with the GetInstalledCertificateIdsRequest written to
std::ostream& operator<<(std::ostream& os, const GetInstalledCertificateIdsRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetInstalledCertificateIdsResponse::get_type() const {
    return "GetInstalledCertificateIdsResponse";
}

void to_json(json& j, const GetInstalledCertificateIdsResponse& k) {
    // the required parts of the message
    j = json{
        {"status", conversions::get_installed_certificate_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.statusInfo) {
        j["statusInfo"] = k.statusInfo.value();
    }
    if (k.certificateHashDataChain) {
        j["certificateHashDataChain"] = json::array();
        for (auto val : k.certificateHashDataChain.value()) {
            j["certificateHashDataChain"].push_back(val);
        }
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetInstalledCertificateIdsResponse& k) {
    // the required parts of the message
    k.status = conversions::string_to_get_installed_certificate_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("statusInfo")) {
        k.statusInfo.emplace(j.at("statusInfo"));
    }
    if (j.contains("certificateHashDataChain")) {
        json arr = j.at("certificateHashDataChain");
        std::vector<CertificateHashDataChain> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.certificateHashDataChain.emplace(vec);
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetInstalledCertificateIdsResponse \p k to the given output
/// stream \p os \returns an output stream with the GetInstalledCertificateIdsResponse written to
std::ostream& operator<<(std::ostream& os, const GetInstalledCertificateIdsResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v2
} // namespace ocpp
