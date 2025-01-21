// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/message_handler.hpp>

#include <ocpp/v201/message_dispatcher.hpp>

#include <ocpp/v201/messages/ClearChargingProfile.hpp>
#include <ocpp/v201/messages/GetChargingProfiles.hpp>
#include <ocpp/v201/messages/GetCompositeSchedule.hpp>
#include <ocpp/v201/messages/ReportChargingProfiles.hpp>
#include <ocpp/v201/messages/SetChargingProfile.hpp>

namespace ocpp::v201 {

class DeviceModel;
class EvseManagerInterface;
class ConnectivityManagerInterface;
class SmartChargingHandlerInterface;

class SmartChargingInterface : public MessageHandlerInterface {
public:
    virtual ~SmartChargingInterface() {
    }
    /// \brief Gets composite schedules for all evse_ids (including 0) for the given \p duration and \p unit . If no
    /// valid profiles are given for an evse for the specified period, the composite schedule will be empty for this
    /// evse.
    /// \param duration of the request from. Composite schedules will be retrieved from now to (now + duration)
    /// \param unit of the period entries of the composite schedules
    /// \return vector of composite schedules, one for each evse_id including 0.
    virtual std::vector<CompositeSchedule> get_all_composite_schedules(const int32_t duration,
                                                               const ChargingRateUnitEnum& unit) = 0;
};

class SmartCharging : public SmartChargingInterface {
private: // Members
    DeviceModel& device_model;
    EvseManagerInterface& evse_manager;
    ConnectivityManagerInterface& connectivity_manager;
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    SmartChargingHandlerInterface& smart_charging_handler;

    std::function<void()> set_charging_profiles_callback;

public:
    SmartCharging(DeviceModel& device_model, EvseManagerInterface& evse_manager, ConnectivityManagerInterface& connectivity_manager,
                  MessageDispatcherInterface<MessageType>& message_dispatcher, SmartChargingHandlerInterface& smart_charging_handler,
                  std::function<void()> set_charging_profiles_callback);
    void handle_message(const ocpp::EnhancedMessage<MessageType>& message) override;
    std::vector<CompositeSchedule> get_all_composite_schedules(const int32_t duration,
                                                               const ChargingRateUnitEnum& unit) override;

private: // Functions
    /* OCPP message requests */
    void report_charging_profile_req(const int32_t request_id, const int32_t evse_id,
                                     const ChargingLimitSourceEnum source, const std::vector<ChargingProfile>& profiles,
                                     const bool tbc);
    void report_charging_profile_req(const ReportChargingProfilesRequest& req);

    /* OCPP message handlers */
    void handle_set_charging_profile_req(Call<SetChargingProfileRequest> call);
    void handle_clear_charging_profile_req(Call<ClearChargingProfileRequest> call);
    void handle_get_charging_profiles_req(Call<GetChargingProfilesRequest> call);
    void handle_get_composite_schedule_req(Call<GetCompositeScheduleRequest> call);

    /// \brief Gets a composite schedule based on the given \p request
    /// \param request specifies different options for the request
    /// \return GetCompositeScheduleResponse containing the status of the operation and the composite schedule if the
    /// operation was successful
    GetCompositeScheduleResponse get_composite_schedule(const GetCompositeScheduleRequest& request);
    GetCompositeScheduleResponse
    get_composite_schedule_internal(const GetCompositeScheduleRequest& request,
                                    const std::set<ChargingProfilePurposeEnum>& profiles_to_ignore = {});
};
} // namespace ocpp::v201
