// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V2_RESET_HPP
#define OCPP_V2_RESET_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/common/types.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

namespace ocpp {
namespace v2 {

/// \brief Contains a OCPP Reset message
struct ResetRequest : public ocpp::Message {
    ResetEnum type;
    std::optional<int32_t> evseId;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this Reset message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given ResetRequest \p k to a given json object \p j
void to_json(json& j, const ResetRequest& k);

/// \brief Conversion from a given json object \p j to a given ResetRequest \p k
void from_json(const json& j, ResetRequest& k);

/// \brief Writes the string representation of the given ResetRequest \p k to the given output stream \p os
/// \returns an output stream with the ResetRequest written to
std::ostream& operator<<(std::ostream& os, const ResetRequest& k);

/// \brief Contains a OCPP ResetResponse message
struct ResetResponse : public ocpp::Message {
    ResetStatusEnum status;
    std::optional<StatusInfo> statusInfo;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this ResetResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given ResetResponse \p k to a given json object \p j
void to_json(json& j, const ResetResponse& k);

/// \brief Conversion from a given json object \p j to a given ResetResponse \p k
void from_json(const json& j, ResetResponse& k);

/// \brief Writes the string representation of the given ResetResponse \p k to the given output stream \p os
/// \returns an output stream with the ResetResponse written to
std::ostream& operator<<(std::ostream& os, const ResetResponse& k);

} // namespace v2
} // namespace ocpp

#endif // OCPP_V2_RESET_HPP
