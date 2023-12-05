// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <functional>
#include <mutex>

#include <ocpp/v201/enums.hpp>
#include <optional>

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
    ErrorClearedOnOccupied,
    ErrorClearedOnReserved,
    UnavailableToAvailable,
    UnavailableToOccupied,
    UnavailableToReserved,
    UnavailableFaulted,
    ReturnToOperativeState
};

namespace conversions {
/// \brief Converts the given ConnectorEvent \p e to human readable string
/// \returns a string representation of the ConnectorEvent
std::string connector_event_to_string(ConnectorEvent e);

/// \brief Converts the given std::string \p s to ConnectorEvent
/// \returns a ConnectorEvent from a string representation
ConnectorEvent string_to_connector_event(const std::string& s);
} // namespace conversions

/// \brief Represents a Connector, thus electrical outlet on a Charging Station. Single physical Connector.
class Connector {
private:
    int32_t connector_id;

    /// \brief Protects the state, last_state, and effective_state fields
    std::mutex state_mutex;
    /// \brief The independent availability state of the whole EVSE, set via OCPP or libocpp calls
    /// This status is persisted in the database
    ConnectorStatusEnum state;
    /// \brief State to return to when returning to operative state. Must be Available, Occupied, or Reserved.
    /// used to e.g. keep track of plug-ins and plug-outs while the connector is inoperative
    /// If "state" is operative, this is the same value.
    ConnectorStatusEnum state_if_operative;
    /// \brief The effective availability status, visible to OCPP and used in most protocol logic
    /// This status is not persisted, but computed from the individual status and the effective status of the parent
    ConnectorStatusEnum effective_state;

    ConnectorStatusEnum get_state();
    void set_state(const ConnectorStatusEnum new_state);
    std::function<void(const ConnectorStatusEnum& status)> status_notification_callback;

public:
    /// \brief Construct a new Connector object
    /// \param connector_id id of the connector
    /// \param status_notification_callback callback executed when the state of the connector changes
    Connector(const int32_t connector_id,
              const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback);

    /// \brief Get the effective state of the connector
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum get_effective_state();

    /// \brief Submits the given \p event to the state machine controller
    /// \param event
    void submit_event(ConnectorEvent event, OperationalStatusEnum evse_status);

    /// \brief Changes the availability status of the connector.
    /// \param new_status The new availability status to switch to, empty if it is to remain the same
    /// \param evse_status The effective availability status of the EVSE
    void change_availability(std::optional<OperationalStatusEnum> new_status,
                             OperationalStatusEnum evse_status);
};

} // namespace v201
} // namespace ocpp
