// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest
// This code is generated using the generator in 'src/code_generator/common`, please do not edit manually

#ifndef OCPP_V21_NOTIFYPERIODICEVENTSTREAM_HPP
#define OCPP_V21_NOTIFYPERIODICEVENTSTREAM_HPP

#include <nlohmann/json_fwd.hpp>
#include <optional>

#include <ocpp/v201/ocpp_enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>
using namespace ocpp::v201;
#include <ocpp/common/types.hpp>

namespace ocpp {
namespace v21 {

/// \brief Contains a OCPP NotifyPeriodicEventStream message
struct NotifyPeriodicEventStreamRequest : public ocpp::Message {
    std::vector<StreamDataElement> data;
    int32_t id;
    int32_t pending;
    std::optional<CustomData> customData;

    /// \brief Provides the type of this NotifyPeriodicEventStream message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given NotifyPeriodicEventStreamRequest \p k to a given json object \p j
void to_json(json& j, const NotifyPeriodicEventStreamRequest& k);

/// \brief Conversion from a given json object \p j to a given NotifyPeriodicEventStreamRequest \p k
void from_json(const json& j, NotifyPeriodicEventStreamRequest& k);

/// \brief Writes the string representation of the given NotifyPeriodicEventStreamRequest \p k to the given output
/// stream \p os \returns an output stream with the NotifyPeriodicEventStreamRequest written to
std::ostream& operator<<(std::ostream& os, const NotifyPeriodicEventStreamRequest& k);

/// \brief Contains a OCPP NotifyPeriodicEventStreamResponse message
struct NotifyPeriodicEventStreamResponse : public ocpp::Message {
    GenericStatusEnum status;
    std::optional<CustomData> customData;
    std::optional<StatusInfo> statusInfo;

    /// \brief Provides the type of this NotifyPeriodicEventStreamResponse message as a human readable string
    /// \returns the message type as a human readable string
    std::string get_type() const override;
};

/// \brief Conversion from a given NotifyPeriodicEventStreamResponse \p k to a given json object \p j
void to_json(json& j, const NotifyPeriodicEventStreamResponse& k);

/// \brief Conversion from a given json object \p j to a given NotifyPeriodicEventStreamResponse \p k
void from_json(const json& j, NotifyPeriodicEventStreamResponse& k);

/// \brief Writes the string representation of the given NotifyPeriodicEventStreamResponse \p k to the given output
/// stream \p os \returns an output stream with the NotifyPeriodicEventStreamResponse written to
std::ostream& operator<<(std::ostream& os, const NotifyPeriodicEventStreamResponse& k);

} // namespace v21
} // namespace ocpp

#endif // OCPP_V21_NOTIFYPERIODICEVENTSTREAM_HPP
