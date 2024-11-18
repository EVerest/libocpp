// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V21_GETDERCONTROL_HPP
#define OCPP_V21_GETDERCONTROL_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/v201/ocpp_enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>
using namespace ocpp::v201;
#include <ocpp/common/types.hpp>

namespace ocpp {
namespace v21 {

/// \brief Contains a OCPP GetDERControl message
struct GetDERControlRequest : public ocpp::Message {
    bool isDefault;
    std::optional<DERControlEnum> controlType;
    std::optional<CiString<36>> controlId;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this GetDERControl message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given GetDERControlRequest \p k to a given json object \p j
void to_json(json& j, const GetDERControlRequest& k);

/// \brief Conversion from a given json object \p j to a given GetDERControlRequest \p k
void from_json(const json& j, GetDERControlRequest& k);

/// \brief Writes the string representation of the given GetDERControlRequest \p k to the given output stream \p os
/// \returns an output stream with the GetDERControlRequest written to
std::ostream& operator<<(std::ostream& os, const GetDERControlRequest& k);

/// \brief Contains a OCPP GetDERControlResponse message
struct GetDERControlResponse : public ocpp::Message {
    DERControlStatusEnum status;
    std::optional<std::vector<DERCurveGet>> curve;
    std::optional<std::vector<EnterServiceGet>> enterService;
    std::optional<std::vector<FixedPFGet>> fixedPFAbsorb;
    std::optional<std::vector<FixedPFGet>> fixedPFInject;
    std::optional<std::vector<FixedVarGet>> fixedVar;
    std::optional<std::vector<FreqDroopGet>> freqDroop;
    std::optional<std::vector<GradientGet>> gradient;
    std::optional<std::vector<LimitMaxDischargeGet>> limitMaxDischarge;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this GetDERControlResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given GetDERControlResponse \p k to a given json object \p j
void to_json(json& j, const GetDERControlResponse& k);

/// \brief Conversion from a given json object \p j to a given GetDERControlResponse \p k
void from_json(const json& j, GetDERControlResponse& k);

/// \brief Writes the string representation of the given GetDERControlResponse \p k to the given output stream \p os
/// \returns an output stream with the GetDERControlResponse written to
std::ostream& operator<<(std::ostream& os, const GetDERControlResponse& k);

} // namespace v21
} // namespace ocpp

#endif // OCPP_V21_GETDERCONTROL_HPP
