// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <unordered_map>

#include <everest/timer.hpp>

#include <ocpp/v201/ocpp_enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>

#include <ocpp/v201/device_model_storage.hpp>

namespace ocpp::v201 {

class DeviceModel;

enum UpdateMonitorMetaType {
    TRIGGER,
    PERIODIC
};

/// \brief Meta data required for our internal keeping needs
struct UpdaterMonitorMeta {
    UpdateMonitorMetaType type;

    VariableMonitoringMeta monitor_meta;
    Component component;
    Variable variable;

    /// \brief database ID for quick instant retrieval if required
    std::int32_t monitor_id;

    std::string value_previous;
    std::string value_current;

    /// \brief Last time this monitor was triggered
    std::chrono::time_point<std::chrono::steady_clock> last_trigger_steady;

    /// \brief Next time when we require to trigger a clock aligned value. Has meaning
    /// only for periodic monitors
    std::chrono::time_point<std::chrono::system_clock> next_trigger_clock_aligned;

    /// \brief Generated monitor events, that can be related to this meta
    std::vector<EventData> generated_monitor_events;

    /// \brief Write-only values will not have the value reported
    std::uint32_t is_writeonly : 1;

    /// \brief If it was sent to the CSMS, has no meaning when this
    /// is a periodic monitor
    std::uint32_t is_csms_sent : 1;

    /// \brief The trigger has been cleared, that is it returned to normal after a problem
    /// was detected. Can be removed from the map when it was cleared. Has no meaning if
    /// this is a periodic monitor
    std::uint32_t is_cleared : 1;
};

typedef std::function<void(const std::vector<EventData>&)> notify_events;
typedef std::function<bool()> is_offline;

class MonitoringUpdater {

public:
    MonitoringUpdater() = delete;

    /// \brief Constructs a new variable monitor updater
    /// \param device_model Currently used variable device model
    /// \param notify_csms_events Function that can be invoked with a number of alert events
    /// \param is_chargepoint_offline Function that can be invoked in order to retrieve the
    /// status of the charging station connection to the CSMS
    MonitoringUpdater(std::shared_ptr<DeviceModel> device_model, notify_events notify_csms_events,
                      is_offline is_chargepoint_offline);
    ~MonitoringUpdater();

public:
    /// \brief Starts monitoring the variables, kicking the timer
    void start_monitoring();
    /// \brief Stops monitoring the variables, canceling the timer
    void stop_monitoring();

    /// \brief Processes the variable triggered monitors. Will be called
    /// after relevant variable modification operations or will be called
    /// periodically in case that processing can not be done at the current
    /// moment, for example in the case of an internal variable modification
    void process_triggered_monitors();

private:
    /// \brief Callback that is registered to the 'device_model' that determines if any of
    /// the monitors are triggered for a certain variable when the internal value is used. Will
    /// delay the sending of the monitors to the CSMS until the charging station has
    /// finished any current operation. The reason is that a variable can change during an
    /// operation where the CSMS does NOT expect a message of type 'EventData' therefore
    /// the processing is delayed either until a manual call to 'process_triggered_monitors'
    /// or when the periodic monitoring timer is hit
    void on_variable_changed(const std::unordered_map<int64_t, VariableMonitoringMeta>& monitors,
                             const Component& component, const Variable& variable,
                             const VariableCharacteristics& characteristics, const VariableAttribute& attribute,
                             const std::string& value_old, const std::string& value_current);

    /// \brief Processes the periodic monitors. Since this can be somewhat of a costly
    /// operation (DB query of each triggered monitor's actual value) the processing time
    /// can be configured using the 'VariableMonitoringProcessTime' internal variable. If
    // there are also any pending alert triggered monitors, those will be processed too
    void process_monitors_internal(bool allow_periodics, bool allow_trigger);

    /// \brief Processes the monitor meta, generating in it's internal list all the
    /// required events. It will generate the EventData for a notify regardless
    /// of the offline state
    /// \return true if the monitor should be removed from the map, false otherwise
    bool process_monitor_meta_internal(UpdaterMonitorMeta& updater_meta_data);

    /// \brief Query the database (from in-memory data for fast retrieval)
    /// and updates our internal monitors with the new database data
    void update_periodic_monitors_internal();

    void get_monitoring_info(bool& out_is_offline, int& out_offline_severity, int& out_active_monitoring_level,
                             MonitoringBaseEnum& out_active_monitoring_base);

    bool is_monitoring_enabled();

private:
    std::shared_ptr<DeviceModel> device_model;
    Everest::SteadyTimer monitors_timer;

    // Charger to CSMS message unique ID for EventData
    std::int32_t unique_id;

    notify_events notify_csms_events;
    is_offline is_chargepoint_offline;

    std::unordered_map<std::int32_t, UpdaterMonitorMeta> updater_monitors_meta;
};

} // namespace ocpp::v201