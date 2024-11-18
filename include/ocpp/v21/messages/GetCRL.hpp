// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V21_GETCRL_HPP
#define OCPP_V21_GETCRL_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/v201/ocpp_enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>
using namespace ocpp::v201;
#include <ocpp/common/types.hpp>

namespace ocpp {
namespace v21 {

/// \brief Contains a OCPP GetCRL message
struct GetCRLRequest : public ocpp::Message {
    CertificateHashDataType certificateHashData;
    int32_t requestId;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this GetCRL message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given GetCRLRequest \p k to a given json object \p j
void to_json(json& j, const GetCRLRequest& k);

/// \brief Conversion from a given json object \p j to a given GetCRLRequest \p k
void from_json(const json& j, GetCRLRequest& k);

/// \brief Writes the string representation of the given GetCRLRequest \p k to the given output stream \p os
/// \returns an output stream with the GetCRLRequest written to
std::ostream& operator<<(std::ostream& os, const GetCRLRequest& k);

/// \brief Contains a OCPP GetCRLResponse message
struct GetCRLResponse : public ocpp::Message {
    int32_t requestId;
    GenericStatusEnum status;
    std::optional<StatusInfo> statusInfo;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this GetCRLResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given GetCRLResponse \p k to a given json object \p j
void to_json(json& j, const GetCRLResponse& k);

/// \brief Conversion from a given json object \p j to a given GetCRLResponse \p k
void from_json(const json& j, GetCRLResponse& k);

/// \brief Writes the string representation of the given GetCRLResponse \p k to the given output stream \p os
/// \returns an output stream with the GetCRLResponse written to
std::ostream& operator<<(std::ostream& os, const GetCRLResponse& k);

} // namespace v21
} // namespace ocpp

#endif // OCPP_V21_GETCRL_HPP
