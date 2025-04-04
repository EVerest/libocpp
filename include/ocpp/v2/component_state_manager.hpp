// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include "database_handler.hpp"
#include <ocpp/v2/ocpp_enums.hpp>

namespace ocpp::v2 {

/// \brief Exception used when an evse that does not exist is accessed.
class EvseOutOfRangeException : public std::exception {
public:
    explicit EvseOutOfRangeException(int32_t id) : msg{"Evse with id " + std::to_string(id) + " does not exist"} {
    }

    ~EvseOutOfRangeException() noexcept override = default;

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

/// \brief Exception used when an connector_id that does not exist is accessed.
class ConnectorOutOfRangeException : public std::exception {
public:
    explicit ConnectorOutOfRangeException(int32_t connector_id, int32_t evse_id) :
        msg{"Connector with id " + std::to_string(connector_id) + " does not exist for evse with id " +
            std::to_string(evse_id)} {
    }

    ~ConnectorOutOfRangeException() noexcept override = default;

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

/// \brief Describes the individual state of a single connector
struct FullConnectorStatus {
    /// \brief Operative/Inoperative status, usually set by the CSMS
    OperationalStatusEnum individual_operational_status;
    /// \brief True if the connector has an active (uncleared) error, assumed false on boot
    bool faulted;
    /// \brief True if the connector has an active reservation, assumed false on boot
    bool reserved;
    /// \brief True if the connector has a cable plugged in, assumed false on boot
    bool occupied;
    /// \brief True if the connector is explicitly set to unavailable
    bool unavailable;

    /// \brief Translates the individual state to an Available/Unavailable/Occupied/Reserved/Faulted state
    /// This does NOT take into account the state of the EVSE or CS,
    /// and is intended to be used internally by the ComponentStateManagerInterface.
    ConnectorStatusEnum to_connector_status();
};

class ComponentStateManagerInterface {
public:
    virtual ~ComponentStateManagerInterface();

    /// \brief Set a callback to be called when the effective Operative/Inoperative state of the CS changes.
    virtual void set_cs_effective_availability_changed_callback(
        const std::function<void(const OperationalStatusEnum new_status)>& callback) = 0;

    /// \brief Set a callback to be called when the effective Operative/Inoperative state of an EVSE changes.
    virtual void set_evse_effective_availability_changed_callback(
        const std::function<void(const int32_t evse_id, const OperationalStatusEnum new_status)>& callback) = 0;

    /// \brief Set a callback to be called when the effective Operative/Inoperative state of a connector changes.
    virtual void set_connector_effective_availability_changed_callback(
        const std::function<void(const int32_t evse_id, const int32_t connector_id,
                                 const OperationalStatusEnum new_status)>& callback) = 0;

    /// \brief Get the individual status (Operative/Inoperative) of the CS, as set by the CSMS
    virtual OperationalStatusEnum get_cs_individual_operational_status() = 0;

    /// \brief Get the individual status (Operative/Inoperative) of an EVSE, as set by the CSMS
    /// Note: This is not the same as the effective status.
    /// The EVSE might be effectively Inoperative if the CS is Inoperative.
    virtual OperationalStatusEnum get_evse_individual_operational_status(int32_t evse_id) = 0;

    /// \brief Get the individual status (Operative/Inoperative) of a connector, as set by the CSMS
    /// Note: This is not the same as the effective status.
    /// The connector might be effectively Inoperative if its EVSE or the CS is Inoperative.
    virtual OperationalStatusEnum get_connector_individual_operational_status(int32_t evse_id,
                                                                              int32_t connector_id) = 0;

    /// \brief Get the individual status (Operative/Inoperative) of the CS, as persisted in the database
    /// This status is restored after reboot, and differs from the individual status if non-persistent
    /// status changes were made.
    virtual OperationalStatusEnum get_cs_persisted_operational_status() = 0;

    /// \brief Get the individual status (Operative/Inoperative) of an EVSE, as persisted in the database
    /// This status is restored after reboot, and differs from the individual status if non-persistent
    /// status changes were made.
    virtual OperationalStatusEnum get_evse_persisted_operational_status(int32_t evse_id) = 0;

    /// \brief Get the individual status (Operative/Inoperative) of a connector, as persisted in the database
    /// This status is restored after reboot, and differs from the individual status if non-persistent
    /// status changes were made.
    virtual OperationalStatusEnum get_connector_persisted_operational_status(int32_t evse_id, int32_t connector_id) = 0;

    /// \brief Set the individual status (Operative/Inoperative) of the CS
    virtual void set_cs_individual_operational_status(OperationalStatusEnum new_status, bool persist) = 0;

    /// \brief Set the individual status (Operative/Inoperative) of an EVSE
    /// Note: This is not the same as the effective status.
    /// The EVSE might be effectively Inoperative if the CS is Inoperative.
    virtual void set_evse_individual_operational_status(int32_t evse_id, OperationalStatusEnum new_status,
                                                        bool persist) = 0;

    /// \brief Set the individual status (Operative/Inoperative) of a connector
    /// Note: This is not the same as the effective status.
    /// The connector might be effectively Inoperative if its EVSE or the CS is Inoperative.
    virtual void set_connector_individual_operational_status(int32_t evse_id, int32_t connector_id,
                                                             OperationalStatusEnum new_status, bool persist) = 0;

    /// \brief Get the effective Operative/Inoperative status of an EVSE
    /// This is computed from the EVSE's and the CS's individual statuses.
    virtual OperationalStatusEnum get_evse_effective_operational_status(int32_t evse_id) = 0;

    /// \brief Get the effective Operative/Inoperative status of a connector.
    /// This is computed from the connector's, the EVSE's, and the CS's individual statuses.
    virtual OperationalStatusEnum get_connector_effective_operational_status(int32_t evse_id, int32_t connector_id) = 0;

    /// \brief Get the effective Available/Unavailable/Occupied/Faulted/Reserved status of a connector.
    /// If the EVSE or the CS is Inoperative, the connector will be effectively Unavailable.
    virtual ConnectorStatusEnum get_connector_effective_status(int32_t evse_id, int32_t connector_id) = 0;

    /// \brief Update the state of the connector when plugged in or out
    virtual void set_connector_occupied(int32_t evse_id, int32_t connector_id, bool is_occupied) = 0;

    /// \brief Update the state of the connector when reservations are made or expire
    virtual void set_connector_reserved(int32_t evse_id, int32_t connector_id, bool is_reserved) = 0;

    /// \brief Update the state of the connector when errors are raised and cleared
    virtual void set_connector_faulted(int32_t evse_id, int32_t connector_id, bool is_faulted) = 0;

    /// \brief Update the state of the connector when unavailable or enabled
    virtual void set_connector_unavailable(int32_t evse_id, int32_t connector_id, bool is_unavailable) = 0;

    /// \brief Call the {cs, evse, connector}_effective_availability_changed_callback callback once for every component.
    /// This is usually only done once on boot to notify the rest of the system what the state manager expects the
    /// operative state (Operative/Inoperative) of the CS, EVSEs, and connectors to be.
    virtual void trigger_all_effective_availability_changed_callbacks() = 0;

    /// \brief Call the send_connector_status_notification_callback once for every connector.
    /// This is usually done on boot, and on reconnect after the station has been offline for a long time.
    virtual void send_status_notification_all_connectors() = 0;

    /// \brief Call the send_connector_status_notification_callback once for every connector whose state has changed
    /// since it was last reported with a successful send_connector_status_notification_callback. This is usually done
    /// when the station has been offline for short time and comes back online.
    virtual void send_status_notification_changed_connectors() = 0;

    /// \brief Call the send_connector_status_notification_callback for a single connector.
    /// This is usually done when the CSMS explicitly sends a TriggerMessage to send a StatusNotification.
    virtual void send_status_notification_single_connector(int32_t evse_id, int32_t connector_id) = 0;
};

/// \brief Stores and monitors operational/effective states of the CS, EVSEs, and connectors
class ComponentStateManager : public ComponentStateManagerInterface {
private:
    std::shared_ptr<DatabaseHandler> database;

    /// Current individual Operative/Inoperative state of the CS
    OperationalStatusEnum cs_individual_status;
    /// Current individual Operative/Inoperative state of EVSEs, plus individual state of connectors.
    std::vector<std::pair<OperationalStatusEnum, std::vector<FullConnectorStatus>>>
        evse_and_connector_individual_statuses;

    /// Last Operative/Inoperative status of the CS that was reported to the user of libocpp via callbacks
    OperationalStatusEnum last_cs_effective_operational_status;
    /// Last Operative/Inoperative status of EVSEs and connectors that was reported to the user of libocpp via callbacks
    std::vector<std::pair<OperationalStatusEnum, std::vector<OperationalStatusEnum>>>
        last_evse_and_connector_effective_operational_statuses;

    /// Last connector status for each connector that was reported with a successful
    /// send_connector_status_notification_callback
    // We need to track this separately because the send_connector_status_notification_callback can fail
    std::vector<std::vector<ConnectorStatusEnum>> last_connector_reported_statuses;

    /// \brief Callback triggered by the library when the effective status of the charging station changes
    /// \param new_status The effective status after the change
    std::optional<std::function<void(const OperationalStatusEnum new_status)>>
        cs_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback triggered by the library when the effective status of an EVSE changes
    /// \param evse_id The ID of the EVSE whose status changed
    /// \param new_status The effective status after the change
    std::optional<std::function<void(const int32_t evse_id, const OperationalStatusEnum new_status)>>
        evse_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback triggered by the library when the effective status of a connector changes
    /// \param evse_id The ID of the EVSE whose status changed
    /// \param connector_id The ID of the connector within the EVSE whose status changed
    /// \param new_status The effective status after the change
    std::optional<
        std::function<void(const int32_t evse_id, const int32_t connector_id, const OperationalStatusEnum new_status)>>
        connector_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback used by the library to trigger a StatusUpdateRequest for a connector
    /// \param evse_id The ID of the EVSE
    /// \param connector_id The ID of the connector
    /// \param new_status The connector status
    /// \param initiated_by_trigger_message Indicates if the StatusNotification was initiated by a TriggerMessage.req
    /// \return true if the status notification was successfully sent, false otherwise (usually it fails when offline)
    std::function<bool(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum new_status,
                       bool initiated_by_trigger_message)>
        send_connector_status_notification_callback;

    /// \brief Internal convenience function - returns the number of EVSEs
    int32_t num_evses();
    /// \brief Internal convenience function - returns the number of connectors in an EVSE
    int32_t num_connectors(int32_t evse_id);

    /// \brief Throws a std::out_of_range if \param evse_id is out of bounds
    void check_evse_id(int32_t evse_id);
    /// \brief Throws a std::out_of_range if \param evse_id or \param connector_id is out of bounds
    void check_evse_and_connector_id(int32_t evse_id, int32_t connector_id);

    /// \brief Convenience function, returns a (writeable) reference to the individual status of an EVSE
    OperationalStatusEnum& individual_evse_status(int32_t evse_id);
    /// \brief Convenience function, returns a (writeable) reference to the individual status of a connector
    FullConnectorStatus& individual_connector_status(int32_t evse_id, int32_t connector_id);

    /// \brief Convenience function, returns a (writeable) reference to last Operative/Inoperative status of an EVSE
    /// that was reported to the user of libocpp via callbacks.
    OperationalStatusEnum& last_evse_effective_status(int32_t evse_id);
    /// \brief Convenience function, returns a (writeable) reference to last Operative/Inoperative status of a connector
    /// that was reported to the user of libocpp via callbacks.
    OperationalStatusEnum& last_connector_effective_status(int32_t evse_id, int32_t connector_id);
    /// \brief Convenience function, returns a (writeable) reference to last connector status that was successfully
    /// reported via send_connector_status_notification_callback
    ConnectorStatusEnum& last_connector_reported_status(int32_t evse_id, int32_t connector_id);

    /// \brief Internal helper function, triggers {cs, evse, connector}_effective_availability_changed_callback calls
    /// for the CS and all sub-components.
    /// \param only_if_state_changed If set to true, callbacks are only triggered for components whose state
    ///     has changed since it was last reported via callbacks
    void trigger_callbacks_cs(bool only_if_state_changed);
    /// \brief Internal helper function, triggers {evse, connector}_effective_availability_changed_callback calls
    /// for an EVSE and its connectors
    /// \param only_if_state_changed If set to true, callbacks are only triggered for components whose state
    ///     has changed since it was last reported via callbacks
    void trigger_callbacks_evse(int32_t evse_id, bool only_if_state_changed);
    /// \brief Internal helper function, triggers connector_effective_availability_changed_callback calls
    /// for a connector
    /// \param only_if_state_changed If set to true, callbacks are only triggered for components whose state
    ///     has changed since it was last reported via callbacks
    void trigger_callbacks_connector(int32_t evse_id, int32_t connector_id, bool only_if_state_changed);

    /// \brief Internal helper function, calls send_connector_status_notification_callback for a single connector
    /// \param only_if_changed If set to true, the callback will only be triggered if the connector state has changed
    ///  since it was last reported with a successful send_connector_status_notification_callback
    /// \param intiated_by_trigger_message Indicates if the StatusNotification was initiated by a TriggerMessage.req
    void send_status_notification_single_connector_internal(int32_t evse_id, int32_t connector_id, bool only_if_changed,
                                                            bool intiated_by_trigger_message = false);

    /// \brief Initializes *_individual_status(es) from the values stored in the DB.
    /// Inserts Operative if values are missing.
    void read_all_states_from_database_or_set_defaults(const std::map<int32_t, int32_t>& evse_connector_structure);

    /// \brief Initializes last_*_operational_status(es) and last_connector_reported_statuses
    /// with the current effective statuses of all components
    void initialize_reported_state_cache();

public:
    /// \brief At construction time, the state of each component (CS, EVSEs, and connectors) is retrieved from the
    /// database. No callbacks are triggered at this stage.
    /// When the status of components is updated, corresponding callbacks are triggered to notify the user of libocpp.
    /// Additionally, the ComponentStateManager sends StatusNotifications to the CSMS when connector statuses change.
    /// Note: It is expected that ComponentStateManagerInterface::trigger_all_effective_availability_changed_callbacks
    /// is called on boot, and ComponentStateManagerInterface::send_status_notification_all_connectors is called when
    /// first connected to the CSMS.
    /// \param evse_connector_structure Maps each EVSE ID to the number of connectors the EVSE has
    /// \param db_handler A shared reference to the persistent database
    /// \param send_connector_status_notification_callback The callback through which to send StatusNotifications to the
    /// CSMS \param initiated_by_trigger_message Indicates if the StatusNotification was initiated by a
    /// TriggerMessage.req
    explicit ComponentStateManager(
        const std::map<int32_t, int32_t>& evse_connector_structure, std::shared_ptr<DatabaseHandler> db_handler,
        std::function<bool(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum new_status,
                           const bool initiated_by_trigger_message)>
            send_connector_status_notification_callback);

    void set_cs_effective_availability_changed_callback(
        const std::function<void(const OperationalStatusEnum new_status)>& callback);

    void set_evse_effective_availability_changed_callback(
        const std::function<void(const int32_t evse_id, const OperationalStatusEnum new_status)>& callback);

    void set_connector_effective_availability_changed_callback(
        const std::function<void(const int32_t evse_id, const int32_t connector_id,
                                 const OperationalStatusEnum new_status)>& callback);

    OperationalStatusEnum get_cs_individual_operational_status();
    OperationalStatusEnum get_evse_individual_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_individual_operational_status(int32_t evse_id, int32_t connector_id);

    OperationalStatusEnum get_cs_persisted_operational_status();
    OperationalStatusEnum get_evse_persisted_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_persisted_operational_status(int32_t evse_id, int32_t connector_id);

    void set_cs_individual_operational_status(OperationalStatusEnum new_status, bool persist);
    void set_evse_individual_operational_status(int32_t evse_id, OperationalStatusEnum new_status, bool persist);
    void set_connector_individual_operational_status(int32_t evse_id, int32_t connector_id,
                                                     OperationalStatusEnum new_status, bool persist);

    OperationalStatusEnum get_evse_effective_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_effective_operational_status(int32_t evse_id, int32_t connector_id);
    ConnectorStatusEnum get_connector_effective_status(int32_t evse_id, int32_t connector_id);

    void set_connector_occupied(int32_t evse_id, int32_t connector_id, bool is_occupied);
    void set_connector_reserved(int32_t evse_id, int32_t connector_id, bool is_reserved);
    void set_connector_faulted(int32_t evse_id, int32_t connector_id, bool is_faulted);
    void set_connector_unavailable(int32_t evse_id, int32_t connector_id, bool is_unavailable);

    void trigger_all_effective_availability_changed_callbacks();

    void send_status_notification_all_connectors();
    void send_status_notification_changed_connectors();
    void send_status_notification_single_connector(int32_t evse_id, int32_t connector_id);
};

} // namespace ocpp::v2
