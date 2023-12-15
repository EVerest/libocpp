// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <functional>
#include <mutex>

#include "component_state_manager.hpp"
#include "database_handler.hpp"
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
    /// \brief ID of the EVSE this connector belongs to (>0)
    int32_t evse_id;
    /// \brief ID of the connector itself (>0)
    int32_t connector_id;

    /// \brief Component responsible for maintaining and persisting the operational status of CS, EVSEs, and connectors.
    std::shared_ptr<ComponentStateManager> component_state_manager;

    /// \brief Sends a status update to the CSMS about a change in effective status of this connector.
    std::function<void(const ConnectorStatusEnum& status)> status_notification_callback;

    /// \brief Signal a changed availability of a single connector
    /// \param evse_id The id of the EVSE
    /// \param connector_id The ID of the connector within the EVSE
    /// \param new_status The operational status to switch to
    std::function<void(const int32_t evse_id, const int32_t connector_id,
                       const OperationalStatusEnum new_status)>
        change_connector_effective_availability_callback;

public:
    /// \brief Construct a new Connector object
    /// \param evse_id id of the EVSE the connector is ap art of
    /// \param connector_id id of the connector
    /// \param database_handler a reference to the internal libocpp database handler
    /// \param status_notification_callback callback executed to send a status notification for the connector
    /// \param change_effective_availability_callback callback to change the effective operative state of the connector
    Connector(const int32_t evse_id, const int32_t connector_id,
              std::shared_ptr<ComponentStateManager> component_state_manager,
              const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback,
              std::function<void(const int32_t evse_id, const int32_t connector_id,
                                 const OperationalStatusEnum new_status)>
                  change_connector_effective_availability_callback);

    /// \brief Gets the effective Operative/Inoperative status of this connector
    OperationalStatusEnum get_effective_operational_status();
    /// \brief Gets the effective Available/Unavailable/Faulted/Reserved/Occupied status of this connector
    ConnectorStatusEnum get_effective_connector_status();

    /// \brief Adjust the state of the connector according to the \p event that was submitted.
    /// \param event
    void submit_event(ConnectorEvent event);

    /// \brief Explicitly causes a StatusNotification callback with the current effective status to be sent.
    void trigger_status_notification_callback();

    /// \brief Switches the operative status of the connector and recomputes its effective status
    /// \param new_status: The operative status to switch to
    /// \param persist: True if the updated operative status setting should be persisted
    void set_connector_operative_status(OperationalStatusEnum new_status, bool persist);

    /// \brief Explicitly trigger the change_effective_availability_callback for each component (done on boot)
    void trigger_change_effective_availability_callback();

    /// \brief Call the change_effective_availability_callback and the status_notification_callback if state changed
    void trigger_callbacks_if_effective_state_changed();
};

} // namespace v201
} // namespace ocpp
