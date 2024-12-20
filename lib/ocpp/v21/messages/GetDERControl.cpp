// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#include <ocpp/v21/messages/GetDERControl.hpp>

#include <optional>
#include <ostream>
#include <string>

using json = nlohmann::json;

namespace ocpp {
namespace v21 {

std::string GetDERControlRequest::get_type() const {
    return "GetDERControl";
}

void to_json(json& j, const GetDERControlRequest& k) {
    // the required parts of the message
    j = json{
        {"isDefault", k.isDefault},
    };
    // the optional parts of the message
    if (k.controlType) {
        j["controlType"] = ocpp::v201::conversions::dercontrol_enum_to_string(k.controlType.value());
    }
    if (k.controlId) {
        j["controlId"] = k.controlId.value();
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetDERControlRequest& k) {
    // the required parts of the message
    k.isDefault = j.at("isDefault");

    // the optional parts of the message
    if (j.contains("controlType")) {
        k.controlType.emplace(ocpp::v201::conversions::string_to_dercontrol_enum(j.at("controlType")));
    }
    if (j.contains("controlId")) {
        k.controlId.emplace(j.at("controlId"));
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetDERControlRequest \p k to the given output stream \p os
/// \returns an output stream with the GetDERControlRequest written to
std::ostream& operator<<(std::ostream& os, const GetDERControlRequest& k) {
    os << json(k).dump(4);
    return os;
}

std::string GetDERControlResponse::get_type() const {
    return "GetDERControlResponse";
}

void to_json(json& j, const GetDERControlResponse& k) {
    // the required parts of the message
    j = json{
        {"status", ocpp::v201::conversions::dercontrol_status_enum_to_string(k.status)},
    };
    // the optional parts of the message
    if (k.curve) {
        j["curve"] = json::array();
        for (auto val : k.curve.value()) {
            j["curve"].push_back(val);
        }
    }
    if (k.enterService) {
        j["enterService"] = json::array();
        for (auto val : k.enterService.value()) {
            j["enterService"].push_back(val);
        }
    }
    if (k.fixedPFAbsorb) {
        j["fixedPFAbsorb"] = json::array();
        for (auto val : k.fixedPFAbsorb.value()) {
            j["fixedPFAbsorb"].push_back(val);
        }
    }
    if (k.fixedPFInject) {
        j["fixedPFInject"] = json::array();
        for (auto val : k.fixedPFInject.value()) {
            j["fixedPFInject"].push_back(val);
        }
    }
    if (k.fixedVar) {
        j["fixedVar"] = json::array();
        for (auto val : k.fixedVar.value()) {
            j["fixedVar"].push_back(val);
        }
    }
    if (k.freqDroop) {
        j["freqDroop"] = json::array();
        for (auto val : k.freqDroop.value()) {
            j["freqDroop"].push_back(val);
        }
    }
    if (k.gradient) {
        j["gradient"] = json::array();
        for (auto val : k.gradient.value()) {
            j["gradient"].push_back(val);
        }
    }
    if (k.limitMaxDischarge) {
        j["limitMaxDischarge"] = json::array();
        for (auto val : k.limitMaxDischarge.value()) {
            j["limitMaxDischarge"].push_back(val);
        }
    }
    if (k.customData) {
        j["customData"] = k.customData.value();
    }
}

void from_json(const json& j, GetDERControlResponse& k) {
    // the required parts of the message
    k.status = ocpp::v201::conversions::string_to_dercontrol_status_enum(j.at("status"));

    // the optional parts of the message
    if (j.contains("curve")) {
        json arr = j.at("curve");
        std::vector<DERCurveGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.curve.emplace(vec);
    }
    if (j.contains("enterService")) {
        json arr = j.at("enterService");
        std::vector<EnterServiceGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.enterService.emplace(vec);
    }
    if (j.contains("fixedPFAbsorb")) {
        json arr = j.at("fixedPFAbsorb");
        std::vector<FixedPFGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.fixedPFAbsorb.emplace(vec);
    }
    if (j.contains("fixedPFInject")) {
        json arr = j.at("fixedPFInject");
        std::vector<FixedPFGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.fixedPFInject.emplace(vec);
    }
    if (j.contains("fixedVar")) {
        json arr = j.at("fixedVar");
        std::vector<FixedVarGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.fixedVar.emplace(vec);
    }
    if (j.contains("freqDroop")) {
        json arr = j.at("freqDroop");
        std::vector<FreqDroopGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.freqDroop.emplace(vec);
    }
    if (j.contains("gradient")) {
        json arr = j.at("gradient");
        std::vector<GradientGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.gradient.emplace(vec);
    }
    if (j.contains("limitMaxDischarge")) {
        json arr = j.at("limitMaxDischarge");
        std::vector<LimitMaxDischargeGet> vec;
        for (auto val : arr) {
            vec.push_back(val);
        }
        k.limitMaxDischarge.emplace(vec);
    }
    if (j.contains("customData")) {
        k.customData.emplace(j.at("customData"));
    }
}

/// \brief Writes the string representation of the given GetDERControlResponse \p k to the given output stream \p os
/// \returns an output stream with the GetDERControlResponse written to
std::ostream& operator<<(std::ostream& os, const GetDERControlResponse& k) {
    os << json(k).dump(4);
    return os;
}

} // namespace v21
} // namespace ocpp
