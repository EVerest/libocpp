// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/functional_blocks/smart_charging.hpp>

#include <ocpp/v201/connectivity_manager.hpp>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>
#include <ocpp/v201/evse_manager.hpp>
#include <ocpp/v201/smart_charging.hpp>

#include <ocpp/v201/utils.hpp>

namespace ocpp::v201 {
SmartCharging::SmartCharging(DeviceModel& device_model, EvseManagerInterface& evse_manager,
                             ConnectivityManagerInterface& connectivity_manager,
                             MessageDispatcherInterface<MessageType>& message_dispatcher,
                             DatabaseHandlerInterface& database_handler,
                             std::function<void()> set_charging_profiles_callback) :
    device_model(device_model),
    evse_manager(evse_manager),
    connectivity_manager(connectivity_manager),
    message_dispatcher(message_dispatcher),
    smart_charging_handler(std::make_unique<SmartChargingHandler>(evse_manager, device_model, database_handler)),
    set_charging_profiles_callback(set_charging_profiles_callback) {
}

void SmartCharging::handle_message(const ocpp::EnhancedMessage<MessageType>& message) {
    const auto& json_message = message.message;
    switch (message.messageType) {
    case MessageType::SetChargingProfile:
        this->handle_set_charging_profile_req(json_message);
        break;
    case MessageType::ClearChargingProfile:
        this->handle_clear_charging_profile_req(json_message);
        break;
    case MessageType::GetChargingProfiles:
        this->handle_get_charging_profiles_req(json_message);
        break;
    case MessageType::GetCompositeSchedule:
        this->handle_get_composite_schedule_req(json_message);
        break;
    default:
        throw MessageTypeNotImplementedException(message.messageType);
    }
}

std::vector<CompositeSchedule> SmartCharging::get_all_composite_schedules(const int32_t duration_s,
                                                                          const ChargingRateUnitEnum& unit) {
    std::vector<CompositeSchedule> composite_schedules;

    std::set<ChargingProfilePurposeEnum> purposes_to_ignore = utils::get_purposes_to_ignore(
        this->device_model.get_optional_value<std::string>(ControllerComponentVariables::IgnoredProfilePurposesOffline)
            .value_or(""),
        !this->connectivity_manager.is_websocket_connected());

    const auto number_of_evses = this->evse_manager.get_number_of_evses();
    // get all composite schedules including the one for evse_id == 0
    for (int32_t evse_id = 0; evse_id <= number_of_evses; evse_id++) {
        GetCompositeScheduleRequest request;
        request.duration = duration_s;
        request.evseId = evse_id;
        request.chargingRateUnit = unit;
        auto composite_schedule_response = this->get_composite_schedule_internal(request, purposes_to_ignore);
        if (composite_schedule_response.status == GenericStatusEnum::Accepted and
            composite_schedule_response.schedule.has_value()) {
            composite_schedules.push_back(composite_schedule_response.schedule.value());
        } else {
            EVLOG_warning << "Could not internally retrieve composite schedule for evse id " << evse_id << ": "
                          << composite_schedule_response;
        }
    }

    return composite_schedules;
}

void SmartCharging::delete_transaction_tx_profiles(const std::string& transaction_id) {
    this->smart_charging_handler->delete_transaction_tx_profiles(transaction_id);
}

SetChargingProfileResponse
SmartCharging::conform_validate_and_add_profile(ChargingProfile& profile, int32_t evse_id,
                                                ChargingLimitSourceEnum charging_limit_source,
                                                AddChargingProfileSource source_of_request) {
    return this->smart_charging_handler->conform_validate_and_add_profile(profile, evse_id, charging_limit_source,
                                                                          source_of_request);
}

ProfileValidationResultEnum SmartCharging::conform_and_validate_profile(ChargingProfile& profile, int32_t evse_id,
                                                                        AddChargingProfileSource source_of_request) {
    return this->smart_charging_handler->conform_and_validate_profile(profile, evse_id, source_of_request);
}

void SmartCharging::report_charging_profile_req(const int32_t request_id, const int32_t evse_id,
                                                const ChargingLimitSourceEnum source,
                                                const std::vector<ChargingProfile>& profiles, const bool tbc) {
    ReportChargingProfilesRequest req;
    req.requestId = request_id;
    req.evseId = evse_id;
    req.chargingLimitSource = source;
    req.chargingProfile = profiles;
    req.tbc = tbc;

    ocpp::Call<ReportChargingProfilesRequest> call(req);
    this->message_dispatcher.dispatch_call(call);
}

void SmartCharging::report_charging_profile_req(const ReportChargingProfilesRequest& req) {
    ocpp::Call<ReportChargingProfilesRequest> call(req);
    this->message_dispatcher.dispatch_call(call);
}

void SmartCharging::handle_set_charging_profile_req(Call<SetChargingProfileRequest> call) {
    EVLOG_debug << "Received SetChargingProfileRequest: " << call.msg << "\nwith messageId: " << call.uniqueId;
    auto msg = call.msg;
    SetChargingProfileResponse response;
    response.status = ChargingProfileStatusEnum::Rejected;

    // K01.FR.29: Respond with a CallError if SmartCharging is not available for this Charging Station
    bool is_smart_charging_available =
        this->device_model.get_optional_value<bool>(ControllerComponentVariables::SmartChargingCtrlrAvailable)
            .value_or(false);

    if (!is_smart_charging_available) {
        EVLOG_warning << "SmartChargingCtrlrAvailable is not set for Charging Station. Returning NotSupported error";

        const auto call_error =
            CallError(call.uniqueId, "NotSupported", "Charging Station does not support smart charging", json({}));
        this->message_dispatcher.dispatch_call_error(call_error);

        return;
    }

    // K01.FR.22: Reject ChargingStationExternalConstraints profiles in SetChargingProfileRequest
    if (msg.chargingProfile.chargingProfilePurpose == ChargingProfilePurposeEnum::ChargingStationExternalConstraints) {
        response.statusInfo = StatusInfo();
        response.statusInfo->reasonCode = "InvalidValue";
        response.statusInfo->additionalInfo = "ChargingStationExternalConstraintsInSetChargingProfileRequest";
        EVLOG_debug << "Rejecting SetChargingProfileRequest:\n reasonCode: " << response.statusInfo->reasonCode.get()
                    << "\nadditionalInfo: " << response.statusInfo->additionalInfo->get();

        ocpp::CallResult<SetChargingProfileResponse> call_result(response, call.uniqueId);
        this->message_dispatcher.dispatch_call_result(call_result);

        return;
    }

    response = this->smart_charging_handler->conform_validate_and_add_profile(msg.chargingProfile, msg.evseId);
    if (response.status == ChargingProfileStatusEnum::Accepted) {
        EVLOG_debug << "Accepting SetChargingProfileRequest";
        this->set_charging_profiles_callback();
    } else {
        EVLOG_debug << "Rejecting SetChargingProfileRequest:\n reasonCode: " << response.statusInfo->reasonCode.get()
                    << "\nadditionalInfo: " << response.statusInfo->additionalInfo->get();
    }

    ocpp::CallResult<SetChargingProfileResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);
}

void SmartCharging::handle_clear_charging_profile_req(Call<ClearChargingProfileRequest> call) {
    EVLOG_debug << "Received ClearChargingProfileRequest: " << call.msg << "\nwith messageId: " << call.uniqueId;
    const auto msg = call.msg;
    ClearChargingProfileResponse response;
    response.status = ClearChargingProfileStatusEnum::Unknown;

    // K10.FR.06
    if (msg.chargingProfileCriteria.has_value() and
        msg.chargingProfileCriteria.value().chargingProfilePurpose.has_value() and
        msg.chargingProfileCriteria.value().chargingProfilePurpose.value() ==
            ChargingProfilePurposeEnum::ChargingStationExternalConstraints) {
        response.statusInfo = StatusInfo();
        response.statusInfo->reasonCode = "InvalidValue";
        response.statusInfo->additionalInfo = "ChargingStationExternalConstraintsInClearChargingProfileRequest";
        EVLOG_debug << "Rejecting SetChargingProfileRequest:\n reasonCode: " << response.statusInfo->reasonCode.get()
                    << "\nadditionalInfo: " << response.statusInfo->additionalInfo->get();
    } else {
        response = this->smart_charging_handler->clear_profiles(msg);
    }

    ocpp::CallResult<ClearChargingProfileResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);
}

void SmartCharging::handle_get_charging_profiles_req(Call<GetChargingProfilesRequest> call) {
    EVLOG_debug << "Received GetChargingProfilesRequest: " << call.msg << "\nwith messageId: " << call.uniqueId;
    const auto msg = call.msg;
    GetChargingProfilesResponse response;

    const auto profiles_to_report = this->smart_charging_handler->get_reported_profiles(msg);

    response.status =
        profiles_to_report.empty() ? GetChargingProfileStatusEnum::NoProfiles : GetChargingProfileStatusEnum::Accepted;

    ocpp::CallResult<GetChargingProfilesResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);

    if (response.status == GetChargingProfileStatusEnum::NoProfiles) {
        return;
    }

    // There are profiles to report.
    // Prepare ReportChargingProfileRequest(s). The message defines the properties evseId and
    // chargingLimitSource as required, so we can not report all profiles in a single
    // ReportChargingProfilesRequest. We need to prepare a single ReportChargingProfilesRequest for each
    // combination of evseId and chargingLimitSource
    std::set<int32_t> evse_ids;                // will contain all evse_ids of the profiles
    std::set<ChargingLimitSourceEnum> sources; // will contain all sources of the profiles

    // fill evse_ids and sources sets
    for (const auto& profile : profiles_to_report) {
        evse_ids.insert(profile.evse_id);
        sources.insert(profile.source);
    }

    std::vector<ReportChargingProfilesRequest> requests_to_send;

    for (const auto evse_id : evse_ids) {
        for (const auto source : sources) {
            std::vector<ChargingProfile> original_profiles;
            for (const auto& reported_profile : profiles_to_report) {
                if (reported_profile.evse_id == evse_id and reported_profile.source == source) {
                    original_profiles.push_back(reported_profile.profile);
                }
            }
            if (not original_profiles.empty()) {
                // prepare a ReportChargingProfilesRequest
                ReportChargingProfilesRequest req;
                req.requestId = msg.requestId; // K09.FR.01
                req.evseId = evse_id;
                req.chargingLimitSource = source;
                req.chargingProfile = original_profiles;
                req.tbc = true;
                requests_to_send.push_back(req);
            }
        }
    }

    requests_to_send.back().tbc = false;

    // requests_to_send are ready, send them and define tbc property
    for (const auto& request_to_send : requests_to_send) {
        this->report_charging_profile_req(request_to_send);
    }
}

void SmartCharging::handle_get_composite_schedule_req(Call<GetCompositeScheduleRequest> call) {
    EVLOG_debug << "Received GetCompositeScheduleRequest: " << call.msg << "\nwith messageId: " << call.uniqueId;
    const auto response = this->get_composite_schedule_internal(call.msg);

    ocpp::CallResult<GetCompositeScheduleResponse> call_result(response, call.uniqueId);
    this->message_dispatcher.dispatch_call_result(call_result);
}

GetCompositeScheduleResponse SmartCharging::get_composite_schedule(const GetCompositeScheduleRequest& request) {
    std::set<ChargingProfilePurposeEnum> purposes_to_ignore = utils::get_purposes_to_ignore(
        this->device_model.get_optional_value<std::string>(ControllerComponentVariables::IgnoredProfilePurposesOffline)
            .value_or(""),
        !this->connectivity_manager.is_websocket_connected());
    return this->get_composite_schedule_internal(request, purposes_to_ignore);
}

GetCompositeScheduleResponse
SmartCharging::get_composite_schedule_internal(const GetCompositeScheduleRequest& request,
                                               const std::set<ChargingProfilePurposeEnum>& profiles_to_ignore) {
    GetCompositeScheduleResponse response;
    response.status = GenericStatusEnum::Rejected;

    std::vector<std::string> supported_charging_rate_units = ocpp::split_string(
        this->device_model.get_value<std::string>(ControllerComponentVariables::ChargingScheduleChargingRateUnit), ',',
        true);

    std::optional<ChargingRateUnitEnum> charging_rate_unit = std::nullopt;
    if (request.chargingRateUnit.has_value()) {
        bool unit_supported = std::any_of(
            supported_charging_rate_units.begin(), supported_charging_rate_units.end(), [&request](std::string item) {
                return conversions::string_to_charging_rate_unit_enum(item) == request.chargingRateUnit.value();
            });

        if (unit_supported) {
            charging_rate_unit = request.chargingRateUnit.value();
        }
    } else if (supported_charging_rate_units.size() > 0) {
        charging_rate_unit = conversions::string_to_charging_rate_unit_enum(supported_charging_rate_units.at(0));
    }

    // K01.FR.05 & K01.FR.07
    if (this->evse_manager.does_evse_exist(request.evseId) and charging_rate_unit.has_value()) {
        auto start_time = ocpp::DateTime();
        auto end_time = ocpp::DateTime(start_time.to_time_point() + std::chrono::seconds(request.duration));

        std::vector<ChargingProfile> valid_profiles =
            this->smart_charging_handler->get_valid_profiles(request.evseId, profiles_to_ignore);

        auto schedule = this->smart_charging_handler->calculate_composite_schedule(
            valid_profiles, start_time, end_time, request.evseId, charging_rate_unit.value());

        response.schedule = schedule;
        response.status = GenericStatusEnum::Accepted;
    } else {
        auto reason = charging_rate_unit.has_value()
                          ? ProfileValidationResultEnum::EvseDoesNotExist
                          : ProfileValidationResultEnum::ChargingScheduleChargingRateUnitUnsupported;
        response.statusInfo = StatusInfo();
        response.statusInfo->reasonCode = conversions::profile_validation_result_to_reason_code(reason);
        response.statusInfo->additionalInfo = conversions::profile_validation_result_to_string(reason);
        EVLOG_debug << "Rejecting SetChargingProfileRequest:\n reasonCode: " << response.statusInfo->reasonCode.get()
                    << "\nadditionalInfo: " << response.statusInfo->additionalInfo->get();
    }
    return response;
}
} // namespace ocpp::v201
