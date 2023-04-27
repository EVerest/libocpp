// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <functional>
#include <mutex>

#include <ocpp/v201/enums.hpp>

namespace ocpp {
namespace v201 {

/// \brief Enum for ConnectorEvents
enum class ConnectorEvent {
    PlugIn,
    PlugOut,
    Reserve,
    Error,
    Unavailable,
    ReservationFinished,
    PlugInAndTokenValid,
    ErrorCleared,
    ErrorCleardOnOccupied,
    ErrorCleardOnReserved,
    UnavailableToAvailable,
    UnavailableToOccupied,
    UnavailableToReserved,
    UnavailableFaulted,
    ReturnToOperativeState
};

/// \brief Represents a Connector, thus electrical outlet on a Charging Station. Single physical Connector.
class Connector {
private:
    int32_t connector_id;
    ConnectorStatusEnum state;
    ConnectorStatusEnum last_state;
    OperationalStatusEnum operational_state;
    std::mutex state_mutex;

    void set_state(const ConnectorStatusEnum new_state);
    std::function<void(const ConnectorStatusEnum& status)> status_notification_callback;

public:
    /// \brief Construct a new Connector object
    /// \param connector_id id of the connector
    /// \param status_notification_callback callback executed when the state of the connector changes
    Connector(const int32_t connector_id,
              const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback);

    /// \brief Get the state object
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum get_state();

    /// \brief Get the operational state
    /// \return OperationalStatusEnum
    OperationalStatusEnum get_operational_state();

    void set_operational_state(const OperationalStatusEnum &operational_state);

    /// \brief Submits the given \p event to the state machine controller
    /// \param event
    void submit_event(ConnectorEvent event);
};

} // namespace v201
} // namespace ocpp
