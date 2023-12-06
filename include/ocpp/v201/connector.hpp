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
    ReservationCleared,
    Error,
    ErrorCleared,
};

namespace conversions {
/// \brief Converts the given ConnectorEvent \p e to human readable string
/// \returns a string representation of the ConnectorEvent
std::string connector_event_to_string(ConnectorEvent e);
} // namespace conversions

/// \brief Represents a Connector, thus electrical outlet on a Charging Station. Single physical Connector.
class Connector {
private:
    int32_t connector_id;

    /// \brief Protects all fields describing the state (or effective state) of the connector
    std::recursive_mutex state_mutex;
    bool enabled;
    bool reserved;
    bool plugged_in;
    bool faulted;
    ConnectorStatusEnum effective_status;

    std::function<void(const ConnectorStatusEnum& status)> status_notification_callback;

    /// \brief Determine the new effective state of the connector
    /// \param evse_status: The effective state of the EVSE
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum determine_effective_status(OperationalStatusEnum evse_status);

    /// \brief Updates the connector's effective status and publishes a status update if needed
    void set_effective_status(ConnectorStatusEnum new_effective_status);

public:
    /// \brief Construct a new Connector object
    /// \param connector_id id of the connector
    /// \param status_notification_callback callback executed when the state of the connector changes
    Connector(const int32_t connector_id,
              const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback);

    /// \brief Get the effective state of the connector
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum get_effective_status();

    /// \brief Submits the given \p event to the state machine controller
    /// \param event
    /// \param evse_status: The effective state of the EVSE
    void submit_event(ConnectorEvent event, OperationalStatusEnum evse_status);

    /// \brief Switches the operative status of the connector
    /// \param new_status: The new operative status to switch to
    /// \param evse_status: The effective status of the EVSE
    /// \param persist: Whether the updated state should be persisted in the database or not
    void set_operative_status(OperationalStatusEnum new_status, OperationalStatusEnum evse_status, bool persist);

    /// \brief Updates the effective status of the container without changing the operational status
    /// \param evse_status: The effective status of the EVSE
    void update_effective_status(OperationalStatusEnum evse_status);
};

} // namespace v201
} // namespace ocpp
