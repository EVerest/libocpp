// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "smart_charging_test_utils.hpp"

TEST_F(CompositeScheduleTestFixtureV201, setpoint_tx_profile) {
    this->load_charging_profiles_for_evse(BASE_JSON_PATH_V21 + "/setpoints/", DEFAULT_EVSE_ID); 

    evse_manager->open_transaction(DEFAULT_EVSE_ID, "f1522902-1170-416f-8e43-9e3bce28fde7");

    const DateTime start_time = ocpp::DateTime("2024-01-17T18:00:00");
    const DateTime end_time = ocpp::DateTime("2024-01-18T06:00:00");

    ChargingSchedulePeriod period1;
    period1.startPeriod = 0;
    period1.limit = 2000.0;
    period1.numberPhases = 1;
    // period1.setpoint = 1800.0;
    ChargingSchedulePeriod period2;
    period2.startPeriod = 1080;
    period2.limit = 11000.0;
    period2.numberPhases = 1;
    // period2.setpoint = -8000.0;
    // period2.dischargeLimit = -10000.0;
    // period2.operationMode = OperationModeEnum::CentralSetpoint;
    ChargingSchedulePeriod period3;
    period3.startPeriod = 25200;
    period3.limit = 6000.0;
    period3.numberPhases = 1;
    // period3.setpoint = 12000.0;
    // period3.operationMode = OperationModeEnum::CentralSetpoint;
    CompositeSchedule expected;
    expected.chargingSchedulePeriod = {period1, period2, period3};
    expected.evseId = DEFAULT_EVSE_ID;
    expected.duration = 43200;
    expected.scheduleStart = start_time;
    expected.chargingRateUnit = ChargingRateUnitEnum::W;

    CompositeSchedule actual = handler->calculate_composite_schedule(start_time, end_time, DEFAULT_EVSE_ID,
                                                                     ChargingRateUnitEnum::W, false, false);

    ASSERT_EQ(actual, expected);
}
