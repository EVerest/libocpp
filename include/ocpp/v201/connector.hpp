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
    Enable,
    Disable,
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
    bool evse_is_operative;

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
