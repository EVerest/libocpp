// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

///
/// @file smart_charging_handler_stub.cpp
/// @brief Stub of the smart charging handler, that can be used in tests.
///
/// This stub implements the methods from the smart charging handler. An extern variable points to the mock and the
/// internal functions call the functions of the mock. This way it is possible to test the class with the 'normal'
/// google mock test functions.
///
/// To be able to use it, include this file in the test and define the mock below the include of this file:
/// `std::unique_ptr<ocpp::v201::SmartChargingHandlerMock> smart_charging_handler_mock;`
///
/// For an example, see ocpp/v201/functional_blocks/test_smart_charging.cpp
/// Also, see the CMakeLists.txt in ocpp/v201/functional_blocks
///

#include "ocpp/v201/smart_charging.hpp"
#include "smart_charging_handler_mock.hpp"

extern std::unique_ptr<ocpp::v201::SmartChargingHandlerMock> smart_charging_handler_mock;

namespace ocpp::v201 {

std::ostream& operator<<(std::ostream& os, const ProfileValidationResultEnum validation_result) {
    os << conversions::profile_validation_result_to_string(validation_result);
    return os;
}

namespace conversions {
/// \brief Converts the given ProfileValidationResultEnum \p e to human readable string
/// \returns a string representation of the ProfileValidationResultEnum
std::string profile_validation_result_to_string(ProfileValidationResultEnum e) {
    return "";
}

/// \brief Converts the given ProfileValidationResultEnum \p e to a OCPP reasonCode.
/// \returns a reasonCode
std::string profile_validation_result_to_reason_code(ProfileValidationResultEnum e) {
    return "";
}
} // namespace conversions

SmartChargingHandler::SmartChargingHandler(EvseManagerInterface& evse_manager, DeviceModel& device_model,
                                           DatabaseHandlerInterface& database_handler) :
    evse_manager(evse_manager), device_model(device_model), database_handler(database_handler) {
}

SetChargingProfileResponse
SmartChargingHandler::conform_validate_and_add_profile(ChargingProfile& profile, int32_t evse_id,
                                                       ChargingLimitSourceEnum charging_limit_source,
                                                       AddChargingProfileSource source_of_request) {
    return smart_charging_handler_mock->conform_validate_and_add_profile(profile, evse_id, charging_limit_source,
                                                                         source_of_request);
}

void SmartChargingHandler::delete_transaction_tx_profiles(const std::string& transaction_id) {
    smart_charging_handler_mock->delete_transaction_tx_profiles(transaction_id);
}

ProfileValidationResultEnum
SmartChargingHandler::conform_and_validate_profile(ChargingProfile& profile, int32_t evse_id,
                                                   AddChargingProfileSource source_of_request) {
    return smart_charging_handler_mock->conform_and_validate_profile(profile, evse_id,
                                                                     source_of_request);
}

SetChargingProfileResponse SmartChargingHandler::add_profile(ChargingProfile& profile, int32_t evse_id,
                                                             ChargingLimitSourceEnum charging_limit_source) {
    return smart_charging_handler_mock->add_profile(profile, evse_id,
                                                    charging_limit_source);
}

ClearChargingProfileResponse SmartChargingHandler::clear_profiles(const ClearChargingProfileRequest& request) {
    return smart_charging_handler_mock->clear_profiles(request);
}

std::vector<ReportedChargingProfile>
SmartChargingHandler::get_reported_profiles(const GetChargingProfilesRequest& request) const {
    return smart_charging_handler_mock->get_reported_profiles(request);
}
std::vector<ChargingProfile>
SmartChargingHandler::get_valid_profiles(int32_t evse_id,
                                         const std::set<ChargingProfilePurposeEnum>& purposes_to_ignore) {
    return smart_charging_handler_mock->get_valid_profiles(evse_id,
                                                           purposes_to_ignore);
}

CompositeSchedule SmartChargingHandler::calculate_composite_schedule(
    std::vector<ChargingProfile>& valid_profiles, const ocpp::DateTime& start_time, const ocpp::DateTime& end_time,
    const int32_t evse_id, std::optional<ChargingRateUnitEnum> charging_rate_unit) {
    return smart_charging_handler_mock->calculate_composite_schedule(
        valid_profiles, start_time, end_time,
        evse_id, charging_rate_unit);
}
} // namespace ocpp::v201
