// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <unordered_map>

#include <everest/timer.hpp>

#include <ocpp/v201/enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>

#include <ocpp/v201/device_model_storage.hpp>

namespace ocpp::v201 {

class DeviceModel;

struct TriggeredMonitorData {
    VariableMonitoringMeta monitor_meta;
    Component component;
    Variable variable;

    std::string value_previous;
    std::string value_current;

    // \brief Write-only values will not have the value reported
    bool is_writeonly;

    /// \brief If the trigger was sent to the CSMS. We'll keep a copy since we'll also detect
    /// when the monitor returns back to normal
    bool csms_sent;

    /// \brief The trigger has been cleared, that is it returned to normal after a problem
    /// was detected. Can be removed from the map when it was cleared
    bool cleared;
};

struct PeriodicMonitorData {
    VariableMonitoringMeta monitor_meta;
    Component component;
    Variable variable;

    /// \brief Last time we've triggered a periodic delta monitor
    std::chrono::time_point<std::chrono::steady_clock> last_trigger_steady;

    /// \brief Next time when we require to trigger a clock aligned value
    std::chrono::time_point<std::chrono::system_clock> next_trigger_clock_aligned;
};

typedef std::function<void(const std::vector<EventData>&)> notify_events;
typedef std::function<bool()> is_offline;

class MonitoringUpdater {

public:
    MonitoringUpdater() = delete;
    MonitoringUpdater(std::shared_ptr<DeviceModel> device_model, notify_events notify_csms_events,
                      is_offline is_chargepoint_offline);
    ~MonitoringUpdater();

public:
    void start_monitoring();
    void stop_monitoring();

    void process_triggered_monitors();

private:
    void on_variable_changed(const std::unordered_map<int64_t, VariableMonitoringMeta>& monitors,
                             const Component& component, const Variable& variable,
                             const VariableCharacteristics& characteristics, const VariableAttribute& attribute,
                             const std::string& value_old, const std::string& value_current);

    void process_periodic_monitors();

private:
    std::shared_ptr<DeviceModel> device_model;
    Everest::SteadyTimer monitors_timer;

    // Charger to CSMS message unique ID for EventData
    int32_t unique_id;

    notify_events notify_csms_events;
    is_offline is_chargepoint_offline;

    std::unordered_map<int32_t, TriggeredMonitorData> triggered_monitors;
    std::unordered_map<int32_t, PeriodicMonitorData> periodic_monitors;
};

} // namespace ocpp::v201