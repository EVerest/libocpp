// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/device_model.hpp>
#include <ocpp/v201/evse_manager.hpp>
#include <ocpp/v201/message_dispatcher.hpp>
#include <ocpp/v201/message_handler.hpp>

#include <ocpp/v201/messages/ChangeAvailability.hpp>
#include <ocpp/v201/messages/Heartbeat.hpp>

namespace ocpp::v201 {
class AvailabilityInterface : public MessageHandlerInterface {
public:
    virtual ~AvailabilityInterface() {
    }

    virtual void handle_message(const ocpp::EnhancedMessage<MessageType>& message) = 0;

    // Functional Block G: Availability
    virtual void status_notification_req(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum status,
                                 const bool initiated_by_trigger_message = false) = 0;
    virtual void heartbeat_req(const bool initiated_by_trigger_message = false) = 0;

    /// \brief Checks if all connectors are effectively inoperative.
    /// If this is the case, calls the all_connectors_unavailable_callback
    /// This is used e.g. to allow firmware updates once all transactions have finished
    virtual bool are_all_connectors_effectively_inoperative() = 0;
};

/// \brief Combines ChangeAvailabilityRequest with persist flag for scheduled Availability changes
struct AvailabilityChange {
    ChangeAvailabilityRequest request;
    bool persist;
};

typedef std::function<void(const ocpp::DateTime& currentTime)> TimeSyncCallback;
typedef std::function<void()> AllConnectorsUnavailableCallback;

class Availability : public AvailabilityInterface {
private: // Members
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    DeviceModel& device_model;
    EvseManagerInterface& evse_manager;
    ComponentStateManagerInterface& component_state_manager;

    std::optional<std::function<void(const ocpp::DateTime& currentTime)>> time_sync_callback;
    std::optional<std::function<void()>> all_connectors_unavailable_callback;

    std::chrono::time_point<std::chrono::steady_clock> heartbeat_request_time;

    std::map<int32_t, AvailabilityChange> scheduled_change_availability_requests;
    // TODO mz move heartbeat timer here?

public:
    Availability(MessageDispatcherInterface<MessageType>& message_dispatcher, DeviceModel& device_model,
                 EvseManagerInterface& evse_manager, ComponentStateManagerInterface& component_state_manager,
                 std::optional<TimeSyncCallback> time_sync_callback,
                 std::optional<AllConnectorsUnavailableCallback> all_connectors_unavailable_callback);
    void handle_message(const ocpp::EnhancedMessage<MessageType>& message) override;

    // Functional Block G: Availability
    void status_notification_req(const int32_t evse_id, const int32_t connector_id, const ConnectorStatusEnum status,
                                 const bool initiated_by_trigger_message = false) override;
    void heartbeat_req(const bool initiated_by_trigger_message = false) override;

private: // Functions
    // Functional Block G: Availability
    void handle_change_availability_req(Call<ChangeAvailabilityRequest> call);
    void handle_heartbeat_response(CallResult<HeartbeatResponse> call);

    /// \brief Helper function to determine if the requested change results in a state that the Connector(s) is/are
    /// already in \param request \return
    bool is_already_in_state(const ChangeAvailabilityRequest& request);
    void handle_scheduled_change_availability_requests(const int32_t evse_id);

    /// \brief Checks if all connectors are effectively inoperative.
    /// If this is the case, calls the all_connectors_unavailable_callback
    /// This is used e.g. to allow firmware updates once all transactions have finished
    bool are_all_connectors_effectively_inoperative() override;

           /// \brief Immediately execute the given \param request to change the operational state of a component
           /// If \param persist is set to true, the change will be persisted across a reboot
    void execute_change_availability_request(ChangeAvailabilityRequest request, bool persist);

    void set_cs_operative_status(OperationalStatusEnum new_status, bool persist) override;

    void set_evse_operative_status(int32_t evse_id, OperationalStatusEnum new_status, bool persist) override;

    void set_connector_operative_status(int32_t evse_id, int32_t connector_id, OperationalStatusEnum new_status,
                                        bool persist) override;
};
} // namespace ocpp::v201
