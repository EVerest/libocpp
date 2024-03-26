// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_V201_CHANGEAVAILABILITY_HPP
#define OCPP_V201_CHANGEAVAILABILITY_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/common/types.hpp>
#include <ocpp/v201/enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>

namespace ocpp {
namespace v201 {

/// \brief Contains a OCPP ChangeAvailability message
struct ChangeAvailabilityRequest : public ocpp::Message {
    OperationalStatusEnum operationalStatus;
    std::optional<CustomData> customData;
    std::optional<EVSE> evse;

    /// \brief Provides the type of this ChangeAvailability message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const;
};

/// \brief Conversion from a given ChangeAvailabilityRequest \p k to a given json object \p j
void to_json(json& j, const ChangeAvailabilityRequest& k);

/// \brief Conversion from a given json object \p j to a given ChangeAvailabilityRequest \p k
void from_json(const json& j, ChangeAvailabilityRequest& k);

/// \brief Writes the string representation of the given ChangeAvailabilityRequest \p k to the given output stream \p os
/// \returns an output stream with the ChangeAvailabilityRequest written to
std::ostream& operator<<(std::ostream& os, const ChangeAvailabilityRequest& k);

/// \brief Contains a OCPP ChangeAvailabilityResponse message
struct ChangeAvailabilityResponse : public ocpp::Message {
    ChangeAvailabilityStatusEnum status;
    std::optional<CustomData> customData;
    std::optional<StatusInfo> statusInfo;

    /// \brief Provides the type of this ChangeAvailabilityResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const;
};

/// \brief Conversion from a given ChangeAvailabilityResponse \p k to a given json object \p j
void to_json(json& j, const ChangeAvailabilityResponse& k);

/// \brief Conversion from a given json object \p j to a given ChangeAvailabilityResponse \p k
void from_json(const json& j, ChangeAvailabilityResponse& k);

/// \brief Writes the string representation of the given ChangeAvailabilityResponse \p k to the given output stream \p
/// os \returns an output stream with the ChangeAvailabilityResponse written to
std::ostream& operator<<(std::ostream& os, const ChangeAvailabilityResponse& k);

} // namespace v201
} // namespace ocpp

#endif // OCPP_V201_CHANGEAVAILABILITY_HPP
