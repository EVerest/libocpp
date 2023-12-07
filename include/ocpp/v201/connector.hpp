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
    /// \brief Matches the connector's operative status setting, as set by the CSMS or libocpp commands.
    /// Note: this might not match the actual state of the connector, e.g. because the EVSE or CS is inoperative.
    bool enabled;
    /// \brief True if a reservation is active on this connector
    bool reserved;
    /// \brief True if the connector is occupied (an EV is plugged in)
    bool plugged_in;
    /// \brief True if an error is active on this connector (e.g. an electrical fault)
    bool faulted;
    /// \brief The actual status of the connector, depending on the enabled, reserved, plugged_in, and faulted booleans
    ///  as well as the effective status (Operative/Inoperative) of the EVSE and CS.
    ConnectorStatusEnum effective_status;

    /// \brief Sends a status update to the CSMS about a change in effective status of this connector.
    std::function<void(const ConnectorStatusEnum& status)> status_notification_callback;

    /// \brief Callback to execute an effective status change (e.g. actually enabling/disabling connectors)
    std::function<void(const OperationalStatusEnum new_status)> change_effective_availability_callback;

    /// \brief Callback to persist an operational state change in the database
    std::function<void(const OperationalStatusEnum new_status)> persist_availability_callback;

    /// \brief Determines the current effective state of the connector
    /// \param evse_status: The effective state of the EVSE
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum determine_effective_status(OperationalStatusEnum evse_status);

    /// \brief Updates the connector's effective status and publishes a status update if it changed
    void set_effective_status(ConnectorStatusEnum new_effective_status);

public:
    /// \brief Construct a new Connector object
    /// \param connector_id id of the connector
    /// \param status_notification_callback callback executed when the state of the connector changes
    Connector(const int32_t connector_id,
              const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback,
              const std::function<void(const OperationalStatusEnum new_status)> change_effective_availability_callback,
              const std::function<void(const OperationalStatusEnum new_status)> persist_availability_callback);

    /// \brief Get the effective state of the connector
    /// \return ConnectorStatusEnum
    ConnectorStatusEnum get_effective_status();

    /// \brief Get the operative status of the connector (NOT the same as the effective status!)
    OperationalStatusEnum get_operative_status();

    /// \brief Adjust the state of the connector according to the \p event that was submitted.
    /// \param event
    /// \param evse_status: The effective state of the EVSE
    void submit_event(ConnectorEvent event, OperationalStatusEnum evse_status);

    /// \brief Switches the operative status of the connector and recomputes its effective status
    /// \param new_status: The operative status to switch to, empty if we only want to recompute the effective status
    /// \param evse_status: The effective status of the EVSE
    /// \param persist: True if the updated operative status setting should be persisted
    void set_operative_status(std::optional<OperationalStatusEnum> new_status, OperationalStatusEnum evse_status,
                              bool persist);
};

} // namespace v201
} // namespace ocpp
