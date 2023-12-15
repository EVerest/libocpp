// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include "database_handler.hpp"
#include <ocpp/v201/enums.hpp>

namespace ocpp::v201 {

struct FullConnectorStatus {
    OperationalStatusEnum individual_operational_status;
    bool faulted;
    bool reserved;
    bool occupied;

    ConnectorStatusEnum to_effective_status();
};

/// \brief Stores and monitors operational/effective states of the CS, EVSEs, and connectors
class ComponentStateManager {
private:
    std::shared_ptr<DatabaseHandler> database;

    OperationalStatusEnum cs_individual_status;
    std::vector<std::pair<OperationalStatusEnum, std::vector<FullConnectorStatus>>>
        evse_and_connector_individual_statuses;

    // Last OPERATIVE status (Operational/Inoperational) that was reported to the user of libocpp via callbacks
    OperationalStatusEnum last_cs_effective_operational_status;
    std::vector<std::pair<OperationalStatusEnum, std::vector<OperationalStatusEnum>>>
        last_evse_and_connector_effective_operational_statuses;

    // Last connector status for each connector that was successfully reported to the CSMS
    // We need to track this separately because the connector_status_update_callback can fail (e.g. when offline)
    std::vector<std::vector<ConnectorStatusEnum>> last_connector_reported_statuses;

    /// \brief Callback triggered by the library when the effective status of the charging station changes
    /// \param old_status The previous effective status
    /// \param new_status The effective status after the change
    std::optional<std::function<void(const OperationalStatusEnum old_status, const OperationalStatusEnum new_status)>>
        cs_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback triggered by the library when the effective status of an EVSE changes
    /// \param evse_id The ID of the EVSE whose status changed
    /// \param old_status The previous effective status
    /// \param new_status The effective status after the change
    std::optional<std::function<void(const int32_t evse_id, const OperationalStatusEnum old_status,
                                     const OperationalStatusEnum new_status)>>
        evse_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback triggered by the library when the effective status of a connector changes
    /// \param evse_id The ID of the EVSE whose status changed
    /// \param connector_id The ID of the connector within the EVSE whose status changed
    /// \param old_status The previous effective status
    /// \param new_status The effective status after the change
    std::optional<std::function<void(const int32_t evse_id, const int32_t connector_id,
                                     const OperationalStatusEnum old_status, const OperationalStatusEnum new_status)>>
        connector_effective_availability_changed_callback = std::nullopt;

    /// \brief Callback used by the library to trigger a StatusUpdateRequest for a connector
    /// \param evse_id The ID of the EVSE
    /// \param connector_id The ID of the connector
    /// \param new_status The connector status
    /// \return true if the status notification was successfully sent, false otherwise
    std::function<bool(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum new_status)>
        connector_status_update_callback;

    int32_t num_evses();
    int32_t num_connectors(int32_t evse_id);

    void check_evse_id(int32_t evse_id);
    void check_evse_and_connector_id(int32_t evse_id, int32_t connector_id);

    OperationalStatusEnum& individual_evse_status(int32_t evse_id);
    FullConnectorStatus& individual_connector_status(int32_t evse_id, int32_t connector_id);

    OperationalStatusEnum& last_evse_effective_status(int32_t evse_id);
    OperationalStatusEnum& last_connector_effective_status(int32_t evse_id, int32_t connector_id);
    ConnectorStatusEnum& last_connector_reported_status(int32_t evse_id, int32_t connector_id);

    void trigger_callbacks_cs(bool only_if_state_changed, bool send_status_updates);
    void trigger_callbacks_evse(int32_t evse_id, bool only_if_state_changed, bool send_status_updates);
    void trigger_callbacks_connector(int32_t evse_id, int32_t connector_id, bool only_if_state_changed,
                                     bool send_status_updates);

    void send_status_notification_single_connector_internal(int32_t evse_id, int32_t connector_id,
                                                            bool only_if_changed);

public:
    explicit ComponentStateManager(
        const std::map<int32_t, int32_t>& evse_connector_structure, std::shared_ptr<DatabaseHandler> db_handler,
        std::function<bool(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum new_status)>
            connector_status_update_callback);

    OperationalStatusEnum get_cs_individual_operational_status();
    OperationalStatusEnum get_evse_individual_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_individual_operational_status(int32_t evse_id, int32_t connector_id);

    OperationalStatusEnum get_cs_persisted_operational_status();
    OperationalStatusEnum get_evse_persisted_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_persisted_operational_status(int32_t evse_id, int32_t connector_id);

    void set_cs_effective_availability_changed_callback(
        std::function<void(const OperationalStatusEnum old_status, const OperationalStatusEnum new_status)> callback);
    void set_evse_effective_availability_changed_callback(
        std::function<void(const int32_t evse_id, const OperationalStatusEnum old_status,
                           const OperationalStatusEnum new_status)>
            callback);
    void set_connector_effective_availability_changed_callback(
        std::function<void(const int32_t evse_id, const int32_t connector_id, const OperationalStatusEnum old_status,
                           const OperationalStatusEnum new_status)>
            callback);

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

    void trigger_boot_callbacks();

    void send_status_notification_all_connectors();
    void send_status_notification_changed_connectors();
    void send_status_notification_single_connector(int32_t evse_id, int32_t connector_id);
};

} // namespace ocpp::v201
