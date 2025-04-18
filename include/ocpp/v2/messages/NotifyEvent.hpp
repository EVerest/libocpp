// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V2_NOTIFYEVENT_HPP
#define OCPP_V2_NOTIFYEVENT_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/common/types.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

namespace ocpp {
namespace v2 {

/// \brief Contains a OCPP NotifyEvent message
struct NotifyEventRequest : public ocpp::Message {
    ocpp::DateTime generatedAt;
    int32_t seqNo;
    std::vector<EventData> eventData;
    std::optional<bool> tbc;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this NotifyEvent message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given NotifyEventRequest \p k to a given json object \p j
void to_json(json& j, const NotifyEventRequest& k);

/// \brief Conversion from a given json object \p j to a given NotifyEventRequest \p k
void from_json(const json& j, NotifyEventRequest& k);

/// \brief Writes the string representation of the given NotifyEventRequest \p k to the given output stream \p os
/// \returns an output stream with the NotifyEventRequest written to
std::ostream& operator<<(std::ostream& os, const NotifyEventRequest& k);

/// \brief Contains a OCPP NotifyEventResponse message
struct NotifyEventResponse : public ocpp::Message {
    std::optional<CustomData> customData;

    /// \brief Provides the type of this NotifyEventResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given NotifyEventResponse \p k to a given json object \p j
void to_json(json& j, const NotifyEventResponse& k);

/// \brief Conversion from a given json object \p j to a given NotifyEventResponse \p k
void from_json(const json& j, NotifyEventResponse& k);

/// \brief Writes the string representation of the given NotifyEventResponse \p k to the given output stream \p os
/// \returns an output stream with the NotifyEventResponse written to
std::ostream& operator<<(std::ostream& os, const NotifyEventResponse& k);

} // namespace v2
} // namespace ocpp

#endif // OCPP_V2_NOTIFYEVENT_HPP
