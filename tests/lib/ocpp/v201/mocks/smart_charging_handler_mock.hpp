// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "gmock/gmock.h"
#include <cstdint>
#include <vector>

#include "ocpp/v201/messages/SetChargingProfile.hpp"
#include "ocpp/v201/ocpp_enums.hpp"
#include "ocpp/v201/smart_charging.hpp"

namespace ocpp::v201 {
class SmartChargingHandlerMock : public SmartChargingHandlerInterface {
public:
    MOCK_METHOD(SetChargingProfileResponse, conform_validate_and_add_profile,
                (ChargingProfile & profile, int32_t evse_id, ChargingLimitSourceEnum charging_limit_source,
                 AddChargingProfileSource source_of_request));
    MOCK_METHOD(ProfileValidationResultEnum, conform_and_validate_profile,
                (ChargingProfile & profile, int32_t evse_id, AddChargingProfileSource source_of_request));
    MOCK_METHOD(void, delete_transaction_tx_profiles, (const std::string& transaction_id));
    MOCK_METHOD(SetChargingProfileResponse, add_profile,
                (ChargingProfile & profile, int32_t evse_id, ChargingLimitSourceEnum charging_limit_source));
    MOCK_METHOD(ClearChargingProfileResponse, clear_profiles, (const ClearChargingProfileRequest& request), (override));
    MOCK_METHOD(std::vector<ReportedChargingProfile>, get_reported_profiles,
                (const GetChargingProfilesRequest& request), (const, override));
    MOCK_METHOD(CompositeSchedule, calculate_composite_schedule,
                (const ocpp::DateTime& start_time, const ocpp::DateTime& end_time, const int32_t evse_id,
                 std::optional<ChargingRateUnitEnum> charging_rate_unit, bool is_offline, bool simulate_transaction_active),
                (override));
};
} // namespace ocpp::v201
