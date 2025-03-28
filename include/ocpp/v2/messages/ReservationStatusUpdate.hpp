// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V2_RESERVATIONSTATUSUPDATE_HPP
#define OCPP_V2_RESERVATIONSTATUSUPDATE_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/common/types.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

namespace ocpp {
namespace v2 {

/// \brief Contains a OCPP ReservationStatusUpdate message
struct ReservationStatusUpdateRequest : public ocpp::Message {
    int32_t reservationId;
    ReservationUpdateStatusEnum reservationUpdateStatus;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this ReservationStatusUpdate message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given ReservationStatusUpdateRequest \p k to a given json object \p j
void to_json(json& j, const ReservationStatusUpdateRequest& k);

/// \brief Conversion from a given json object \p j to a given ReservationStatusUpdateRequest \p k
void from_json(const json& j, ReservationStatusUpdateRequest& k);

/// \brief Writes the string representation of the given ReservationStatusUpdateRequest \p k to the given output stream
/// \p os \returns an output stream with the ReservationStatusUpdateRequest written to
std::ostream& operator<<(std::ostream& os, const ReservationStatusUpdateRequest& k);

/// \brief Contains a OCPP ReservationStatusUpdateResponse message
struct ReservationStatusUpdateResponse : public ocpp::Message {
    std::optional<CustomData> customData;

    /// \brief Provides the type of this ReservationStatusUpdateResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given ReservationStatusUpdateResponse \p k to a given json object \p j
void to_json(json& j, const ReservationStatusUpdateResponse& k);

/// \brief Conversion from a given json object \p j to a given ReservationStatusUpdateResponse \p k
void from_json(const json& j, ReservationStatusUpdateResponse& k);

/// \brief Writes the string representation of the given ReservationStatusUpdateResponse \p k to the given output stream
/// \p os \returns an output stream with the ReservationStatusUpdateResponse written to
std::ostream& operator<<(std::ostream& os, const ReservationStatusUpdateResponse& k);

} // namespace v2
} // namespace ocpp

#endif // OCPP_V2_RESERVATIONSTATUSUPDATE_HPP
