// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V2_CERTIFICATESIGNED_HPP
#define OCPP_V2_CERTIFICATESIGNED_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/common/types.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

namespace ocpp {
namespace v2 {

/// \brief Contains a OCPP CertificateSigned message
struct CertificateSignedRequest : public ocpp::Message {
    CiString<10000> certificateChain;
    std::optional<CertificateSigningUseEnum> certificateType;
    std::optional<int32_t> requestId;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this CertificateSigned message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given CertificateSignedRequest \p k to a given json object \p j
void to_json(json& j, const CertificateSignedRequest& k);

/// \brief Conversion from a given json object \p j to a given CertificateSignedRequest \p k
void from_json(const json& j, CertificateSignedRequest& k);

/// \brief Writes the string representation of the given CertificateSignedRequest \p k to the given output stream \p os
/// \returns an output stream with the CertificateSignedRequest written to
std::ostream& operator<<(std::ostream& os, const CertificateSignedRequest& k);

/// \brief Contains a OCPP CertificateSignedResponse message
struct CertificateSignedResponse : public ocpp::Message {
    CertificateSignedStatusEnum status;
    std::optional<StatusInfo> statusInfo;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this CertificateSignedResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given CertificateSignedResponse \p k to a given json object \p j
void to_json(json& j, const CertificateSignedResponse& k);

/// \brief Conversion from a given json object \p j to a given CertificateSignedResponse \p k
void from_json(const json& j, CertificateSignedResponse& k);

/// \brief Writes the string representation of the given CertificateSignedResponse \p k to the given output stream \p os
/// \returns an output stream with the CertificateSignedResponse written to
std::ostream& operator<<(std::ostream& os, const CertificateSignedResponse& k);

} // namespace v2
} // namespace ocpp

#endif // OCPP_V2_CERTIFICATESIGNED_HPP
