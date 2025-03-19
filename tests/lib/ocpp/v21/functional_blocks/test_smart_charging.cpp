// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v2/functional_blocks/smart_charging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>

#include <date/tz.h>
#include <sqlite3.h>

#include <comparators.hpp>
#include <ocpp/common/call_types.hpp>
#include <ocpp/common/types.hpp>
#include <ocpp/v2/ctrlr_component_variables.hpp>
#include <ocpp/v2/database_handler.hpp>
#include <ocpp/v2/device_model.hpp>
#include <ocpp/v2/evse.hpp>
#include <ocpp/v2/functional_blocks/functional_block_context.hpp>
#include <ocpp/v2/ocpp_enums.hpp>
#include <ocpp/v2/ocpp_types.hpp>

#include "component_state_manager_mock.hpp"
#include "connectivity_manager_mock.hpp"
#include "device_model_test_helper.hpp"
#include "evse_manager_fake.hpp"
#include "evse_mock.hpp"
#include "evse_security_mock.hpp"
#include "lib/ocpp/common/database_testing_utils.hpp"
#include "message_dispatcher_mock.hpp"
#include "smart_charging_test_utils.hpp"

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::Return;
using ::testing::ReturnRef;

namespace ocpp::v2 {
class SmartChargingTestV21 : public DatabaseTestingUtils {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        // TODO: use in-memory db so we don't need to reset the db between tests
        this->database_handler->clear_charging_profiles();
    }

    TestSmartCharging create_smart_charging() {
        std::unique_ptr<common::DatabaseConnection> database_connection =
            std::make_unique<common::DatabaseConnection>(fs::path("/tmp/ocpp201") / "cp.db");
        database_handler =
            std::make_shared<DatabaseHandler>(std::move(database_connection), MIGRATION_FILES_LOCATION_V2);
        database_handler->open_connection();
        device_model = device_model_test_helper.get_device_model();
        this->functional_block_context = std::make_unique<FunctionalBlockContext>(
            this->mock_dispatcher, *this->device_model, this->connectivity_manager, *this->evse_manager,
            *this->database_handler, this->evse_security, this->component_state_manager, this->ocpp_version);
        return TestSmartCharging(*functional_block_context, set_charging_profiles_callback_mock.AsStdFunction());
    }

    // Default values used within the tests
    DeviceModelTestHelper device_model_test_helper;
    MockMessageDispatcher mock_dispatcher;
    std::unique_ptr<EvseManagerFake> evse_manager = std::make_unique<EvseManagerFake>(NR_OF_TWO_EVSES);
    std::shared_ptr<DatabaseHandler> database_handler;
    DeviceModel* device_model;
    ::testing::NiceMock<ConnectivityManagerMock> connectivity_manager;
    ocpp::EvseSecurityMock evse_security;
    ComponentStateManagerMock component_state_manager;
    MockFunction<void()> set_charging_profiles_callback_mock;
    std::unique_ptr<FunctionalBlockContext> functional_block_context;
    TestSmartCharging smart_charging = create_smart_charging();
    std::atomic<OcppProtocolVersion> ocpp_version = OcppProtocolVersion::v21;
};

TEST_F(SmartChargingTestV21,
       K01FR44_IfPhaseToUseProvidedForDCChargingStationAndDCInputPhaseControlFalse_ThenProfileIsInvalid) {
    auto mock_evse = testing::NiceMock<EvseMock>();
    ON_CALL(mock_evse, get_current_phase_type).WillByDefault(testing::Return(CurrentPhaseType::DC));
    ON_CALL(mock_evse, get_id).WillByDefault(testing::Return(1));
    ComponentVariable c =
        EvseComponentVariables::get_component_variable(1, EvseComponentVariables::DCInputPhaseControl);
    device_model->set_value(c.component, c.variable.value(), AttributeEnum::Actual, "false", "test", true);

    auto periods = create_charging_schedule_periods(0, 1, 1, 5.0f);
    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    auto sut = smart_charging.validate_profile_schedules(profile, &mock_evse);

    EXPECT_THAT(sut, testing::Eq(ProfileValidationResultEnum::ChargingSchedulePeriodNoPhaseForDC));
}

TEST_F(SmartChargingTestV21,
       K01FR44_IfPhaseToUseProvidedForDCChargingStationAndDCInputPhaseControlTrue_ThenProfileIsValid) {
    auto mock_evse = testing::NiceMock<EvseMock>();
    ON_CALL(mock_evse, get_current_phase_type).WillByDefault(testing::Return(CurrentPhaseType::DC));
    ON_CALL(mock_evse, get_id).WillByDefault(testing::Return(1));
    ComponentVariable c =
        EvseComponentVariables::get_component_variable(1, EvseComponentVariables::DCInputPhaseControl);
    device_model->set_value(c.component, c.variable.value(), AttributeEnum::Actual, "true", "test", true);

    auto periods = create_charging_schedule_periods(0, 1, 1, 5.0f);
    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    auto sut = smart_charging.validate_profile_schedules(profile, &mock_evse);

    EXPECT_THAT(sut, testing::Eq(ProfileValidationResultEnum::Valid));
}

TEST_F(SmartChargingTestV21, K01FR55_AllChargingProfilesAreStored) {
    // In the default device model, all charging profiles must be set to persistent true, because we don't have a local
    // storage or in-memory storage for the charging profiles (everything is currently stored in the database).
    EXPECT_TRUE(device_model
                    ->get_optional_value<bool>(
                        ControllerComponentVariables::ChargingProfilePersistenceChargingStationExternalConstraints)
                    .value_or(false));
    EXPECT_TRUE(
        device_model->get_optional_value<bool>(ControllerComponentVariables::ChargingProfilePersistenceLocalGeneration)
            .value_or(false));
    EXPECT_TRUE(
        device_model->get_optional_value<bool>(ControllerComponentVariables::ChargingProfilePersistenceTxProfile)
            .value_or(false));
}

TEST_F(SmartChargingTestV21, K01FR56_NewChargingProfileStoredTooQuicklyAfterThePrevious) {
    const ComponentVariable update_rate_limit = ControllerComponentVariables::ChargingProfileUpdateRateLimit;
    device_model->set_value(update_rate_limit.component, update_rate_limit.variable.value(), AttributeEnum::Actual,
                            "5000", "test", true);

    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Accepted));

    response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileRateLimitExceeded");

    device_model->set_value(update_rate_limit.component, update_rate_limit.variable.value(), AttributeEnum::Actual, "0",
                            "test", true);

    response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Accepted));
}

TEST_F(SmartChargingTestV21, K01FR70_PriorityChargingCanNotHaveValueForDuration) {
    const ComponentVariable supported_additional_purpose = ControllerComponentVariables::SupportedAdditionalPurposes;
    device_model->set_value(supported_additional_purpose.component, supported_additional_purpose.variable.value(),
                            AttributeEnum::Actual, "PriorityCharging", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::PriorityCharging,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00"), 5000), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingSchedulePriorityExtranousDuration");
}

TEST_F(SmartChargingTestV21, K01FR71_PriorityChargingMustHaveOperationModeChargingOnly_Rejected) {
    const ComponentVariable supported_additional_purpose = ControllerComponentVariables::SupportedAdditionalPurposes;
    device_model->set_value(supported_additional_purpose.component, supported_additional_purpose.variable.value(),
                            AttributeEnum::Actual, "PriorityCharging", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    periods.at(0).operationMode = OperationModeEnum::ExternalLimits;
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::PriorityCharging,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingSchedulePeriodPriorityChargingNotChargingOnly");
}

TEST_F(SmartChargingTestV21, K01FR71_PriorityChargingMustHaveOperationModeChargingOnly_Accepted) {
    const ComponentVariable supported_additional_purpose = ControllerComponentVariables::SupportedAdditionalPurposes;
    device_model->set_value(supported_additional_purpose.component, supported_additional_purpose.variable.value(),
                            AttributeEnum::Actual, "PriorityCharging", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    periods.at(0).operationMode = OperationModeEnum::ChargingOnly;
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::PriorityCharging,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Accepted));
}

TEST_F(SmartChargingTestV21, K01FR81_ChargingProfileIdIsBiggerThanMaxExternalConstraintsId_Rejected) {
    const ComponentVariable max_external_constraints_id = ControllerComponentVariables::MaxExternalConstraintsId;
    device_model->set_value(max_external_constraints_id.component, max_external_constraints_id.variable.value(),
                            AttributeEnum::Actual, "25", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidProfileId");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileIdSmallerThanMaxExternalConstraintsId");
}

TEST_F(SmartChargingTestV21, K01FR81_ChargingProfileIdIsBiggerThanMaxExternalConstraintsId_Accepted) {
    const ComponentVariable max_external_constraints_id = ControllerComponentVariables::MaxExternalConstraintsId;
    device_model->set_value(max_external_constraints_id.component, max_external_constraints_id.variable.value(),
                            AttributeEnum::Actual, "25", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    auto existing_profile = create_charging_profile(
        26, ChargingProfilePurposeEnum::TxDefaultProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Accepted));
}

TEST_F(SmartChargingTestV21, K01FR120_PriorityChargingNotSupported) {
    const ComponentVariable supported_additional_purpose = ControllerComponentVariables::SupportedAdditionalPurposes;
    device_model->set_value(supported_additional_purpose.component, supported_additional_purpose.variable.value(),
                            AttributeEnum::Actual, "", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    periods.at(0).operationMode = OperationModeEnum::ChargingOnly;
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::PriorityCharging,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "UnsupportedPurpose");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileUnsupportedPurpose");
}

TEST_F(SmartChargingTestV21, K01FR120_LocalGenerationNotSupported) {
    const ComponentVariable supported_additional_purpose = ControllerComponentVariables::SupportedAdditionalPurposes;
    device_model->set_value(supported_additional_purpose.component, supported_additional_purpose.variable.value(),
                            AttributeEnum::Actual, "", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::LocalGeneration,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "UnsupportedPurpose");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileUnsupportedPurpose");
}

TEST_F(SmartChargingTestV21, K01FR121_DynamicProfilesNotSupported) {
    const ComponentVariable supports_dynamic_profiles = ControllerComponentVariables::SupportsDynamicProfiles;
    device_model->set_value(supports_dynamic_profiles.component, supports_dynamic_profiles.variable.value(),
                            AttributeEnum::Actual, "false", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Dynamic, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "UnsupportedKind");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileUnsupportedKind");
}

TEST_F(SmartChargingTestV21, K01FR122_DynUpdateIntervalSetWhileProfileIsNotDynamic) {
    const ComponentVariable supports_dynamic_profiles = ControllerComponentVariables::SupportsDynamicProfiles;
    device_model->set_value(supports_dynamic_profiles.component, supports_dynamic_profiles.variable.value(),
                            AttributeEnum::Actual, "true", "test", true);
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto existing_profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), {},
        ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL, ocpp::DateTime("2024-01-01T13:00:00"),
        ocpp::DateTime("2024-02-01T13:00:00"));
    existing_profile.dynUpdateInterval = 20;

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidProfile");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingProfileNotDynamic");
}

TEST_F(SmartChargingTestV21, K01FR123_LocalTimeNotSupported) {
    // Local time is default not supported, so we don't set the value in the device model here.
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto charging_schedule =
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00"));
    charging_schedule.useLocalTime = true;
    auto existing_profile =
        create_charging_profile(DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile, charging_schedule, {},
                                ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL,
                                ocpp::DateTime("2024-01-01T13:00:00"), ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingScheduleUnsupportedLocalTime");
}

TEST_F(SmartChargingTestV21, K01FR124_RandomizedDelayNotSupported) {
    // Randomized Delay is default not supported, so we don't set the value in the device model here.
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto charging_schedule =
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00"));
    charging_schedule.randomizedDelay = 10;
    auto existing_profile =
        create_charging_profile(DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile, charging_schedule, {},
                                ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL,
                                ocpp::DateTime("2024-01-01T13:00:00"), ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingScheduleUnsupportedRandomizedDelay");
}

TEST_F(SmartChargingTestV21, K01FR125_LimitAtSoCNotSupported) {
    // LimitAtSoc is default not supported, so we don't set the value in the device model here.
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    auto charging_schedule =
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00"));
    charging_schedule.limitAtSoC = {1, 22.0f, std::nullopt};
    auto existing_profile =
        create_charging_profile(DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile, charging_schedule, {},
                                ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL,
                                ocpp::DateTime("2024-01-01T13:00:00"), ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingScheduleUnsupportedLimitAtSoC");
}

TEST_F(SmartChargingTestV21, K01FR126_EvseSleepNotSupported) {
    // EvseSleep is default not supported, so we don't set the value in the device model here.
    auto periods = create_charging_schedule_periods(0, 1, 1, 0.5f);
    ASSERT_GE(periods.size(), 1);
    periods.at(0).evseSleep = true;
    auto charging_schedule =
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00"));
    auto existing_profile =
        create_charging_profile(DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxDefaultProfile, charging_schedule, {},
                                ChargingProfileKindEnum::Absolute, DEFAULT_STACK_LEVEL,
                                ocpp::DateTime("2024-01-01T13:00:00"), ocpp::DateTime("2024-02-01T13:00:00"));

    auto response = smart_charging.conform_validate_and_add_profile(existing_profile, DEFAULT_EVSE_ID);
    EXPECT_THAT(response.status, testing::Eq(ChargingProfileStatusEnum::Rejected));
    EXPECT_EQ(response.statusInfo.value().reasonCode, "InvalidSchedule");
    EXPECT_EQ(response.statusInfo.value().additionalInfo, "ChargingScheduleUnsupportedEvseSleep");
}
} // namespace ocpp::v2
