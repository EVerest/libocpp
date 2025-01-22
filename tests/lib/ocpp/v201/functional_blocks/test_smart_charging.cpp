// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>

#include <ocpp/v201/functional_blocks/smart_charging.hpp>

#include "connectivity_manager_mock.hpp"
#include "database_handler_mock.hpp"
#include "device_model_test_helper.hpp"
#include "evse_manager_fake.hpp"
#include "evse_security_mock.hpp"
#include "message_dispatcher_mock.hpp"

#include "comparators.hpp"

// Since the smart charging handler is a private member of the smart charging functional block, a stub is created to
// be able to mock the internal functions of the smart charging handler and use them in the test.

// Add the sub implementation of the smart charging handler, giving access to the smart charging handler mock.
#include "smart_charging_handler_stub.cpp"

// Define the extern smart charging handler mock (extern is defined in the `smart_charging_handler_stub.cpp`) to be
// able to use the mock is this file.
std::unique_ptr<ocpp::v201::SmartChargingHandlerMock> smart_charging_handler_mock;


const static uint32_t NR_OF_EVSES = 2;
static const int DEFAULT_EVSE_ID = 1;
static const int DEFAULT_PROFILE_ID = 1;
static const int DEFAULT_STACK_LEVEL = 1;
static const ocpp::v201::AddChargingProfileSource DEFAULT_REQUEST_TO_ADD_PROFILE_SOURCE =
    ocpp::v201::AddChargingProfileSource::SetChargingProfile;
static const std::string DEFAULT_TX_ID = "10c75ff7-74f5-44f5-9d01-f649f3ac7b78";


using namespace ocpp::v201;
using ::testing::MockFunction;



class SmartChargingTest : public ::testing::Test {
public:
protected: // Members
    boost::uuids::random_generator uuid_generator;
    DeviceModelTestHelper device_model_test_helper;
    MockMessageDispatcher mock_dispatcher;
    EvseManagerFake evse_manager{NR_OF_EVSES};
    DeviceModel* device_model;
    ::testing::NiceMock<ConnectivityManagerMock> connectivity_manager;
    ::testing::NiceMock<ocpp::v201::DatabaseHandlerMock> database_handler_mock;
    ocpp::EvseSecurityMock evse_security;
    std::unique_ptr<SmartCharging> smart_charging;
    MockFunction<void()> set_charging_profiles_callback_mock;

protected: // Functions
    SmartChargingTest() :
        uuid_generator(boost::uuids::random_generator()),
        device_model_test_helper(),
        mock_dispatcher(),
        device_model(device_model_test_helper.get_device_model()),
        connectivity_manager(),
        database_handler_mock(),
        evse_security(),
        smart_charging(std::make_unique<SmartCharging>(*device_model, evse_manager, connectivity_manager,
                                                       mock_dispatcher, database_handler_mock,
                                                       set_charging_profiles_callback_mock.AsStdFunction())) {
    }

    std::vector<ChargingSchedulePeriod> create_charging_schedule_periods(std::vector<int32_t> start_periods) {
        auto charging_schedule_periods = std::vector<ChargingSchedulePeriod>();
        for (auto start_period : start_periods) {
            ChargingSchedulePeriod charging_schedule_period;
            charging_schedule_period.startPeriod = start_period;
            charging_schedule_periods.push_back(charging_schedule_period);
        }

        return charging_schedule_periods;
    }

    ChargingSchedule create_charge_schedule(ChargingRateUnitEnum charging_rate_unit,
                                            std::vector<ChargingSchedulePeriod> charging_schedule_period,
                                            std::optional<ocpp::DateTime> start_schedule = std::nullopt) {
        int32_t id = 0;
        std::optional<CustomData> custom_data;
        std::optional<int32_t> duration;
        std::optional<float> min_charging_rate;
        std::optional<SalesTariff> sales_tariff;

        return ChargingSchedule{
            id,
            charging_rate_unit,
            charging_schedule_period,
            custom_data,
            start_schedule,
            duration,
            min_charging_rate,
            sales_tariff,
        };
    }

    std::map<int32_t, int32_t> create_evse_connector_structure() {
        std::map<int32_t, int32_t> evse_connector_structure;
        evse_connector_structure.insert_or_assign(1, 1);
        evse_connector_structure.insert_or_assign(2, 1);
        return evse_connector_structure;
    }

    ocpp::v201::ChargingProfile
    create_charging_profile(int32_t charging_profile_id, ChargingProfilePurposeEnum charging_profile_purpose,
                            ChargingSchedule charging_schedule, std::optional<std::string> transaction_id = {},
                            ChargingProfileKindEnum charging_profile_kind = ChargingProfileKindEnum::Absolute,
                            int stack_level = DEFAULT_STACK_LEVEL, std::optional<ocpp::DateTime> validFrom = {},
                            std::optional<ocpp::DateTime> validTo = {}) {
        auto recurrency_kind = RecurrencyKindEnum::Daily;
        std::vector<ChargingSchedule> charging_schedules = {charging_schedule};
        ocpp::v201::ChargingProfile charging_profile;
        charging_profile.id = charging_profile_id;
        charging_profile.stackLevel = stack_level;
        charging_profile.chargingProfilePurpose = charging_profile_purpose;
        charging_profile.chargingProfileKind = charging_profile_kind;
        charging_profile.chargingSchedule = charging_schedules;
        charging_profile.customData = {};
        charging_profile.recurrencyKind = recurrency_kind;
        charging_profile.validFrom = validFrom;
        charging_profile.validTo = validTo;
        charging_profile.transactionId = transaction_id;
        return charging_profile;
    }

    template <class T> void call_to_json(json& j, const ocpp::Call<T>& call) {
        j = json::array();
        j.push_back(ocpp::MessageTypeId::CALL);
        j.push_back(call.uniqueId.get());
        j.push_back(call.msg.get_type());
        j.push_back(json(call.msg));
    }

    std::string uuid() {
        std::stringstream s;
        s << uuid_generator();
        return s.str();
    }

    template <class T, MessageType M> ocpp::EnhancedMessage<MessageType> request_to_enhanced_message(const T& req) {
        auto message_id = uuid();
        ocpp::Call<T> call(req);
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.uniqueId = message_id;
        enhanced_message.messageType = M;
        enhanced_message.messageTypeId = ocpp::MessageTypeId::CALL;

        call_to_json(enhanced_message.message, call);

        return enhanced_message;
    }

    // Test interface
protected:
    void SetUp() override {
        smart_charging_handler_mock = std::make_unique<SmartChargingHandlerMock>();
    }
    void TearDown() override {
        smart_charging_handler_mock = nullptr;
    }
};

TEST_F(SmartChargingTest, K01_SetChargingProfileRequest_ValidatesAndAddsProfile) {
    auto periods = create_charging_schedule_periods({0, 1, 2});

    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    EXPECT_CALL(*smart_charging_handler_mock,
                conform_validate_and_add_profile(profile, DEFAULT_EVSE_ID, ChargingLimitSourceEnum::CSO,
                                                 DEFAULT_REQUEST_TO_ADD_PROFILE_SOURCE));

    smart_charging->handle_message(set_charging_profile_req);
}

TEST_F(SmartChargingTest, K01FR07_SetChargingProfileRequest_TriggersCallbackWhenValid) {
    auto periods = create_charging_schedule_periods({0, 1, 2});

    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    SetChargingProfileResponse accept_response;
    accept_response.status = ChargingProfileStatusEnum::Accepted;

    ON_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).WillByDefault(testing::Return(accept_response));
    EXPECT_CALL(set_charging_profiles_callback_mock, Call);

    smart_charging->handle_message(set_charging_profile_req);
}

TEST_F(SmartChargingTest, K01FR07_SetChargingProfileRequest_DoesNotTriggerCallbackWhenInvalid) {
    auto periods = create_charging_schedule_periods({0, 1, 2});

    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    SetChargingProfileResponse reject_response;
    reject_response.status = ChargingProfileStatusEnum::Rejected;
    reject_response.statusInfo = StatusInfo();
    reject_response.statusInfo->reasonCode = conversions::profile_validation_result_to_reason_code(
        ProfileValidationResultEnum::TxProfileEvseHasNoActiveTransaction);
    reject_response.statusInfo->additionalInfo = conversions::profile_validation_result_to_string(
        ProfileValidationResultEnum::TxProfileEvseHasNoActiveTransaction);

    ON_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).WillByDefault(testing::Return(reject_response));
    EXPECT_CALL(set_charging_profiles_callback_mock, Call).Times(0);

    smart_charging->handle_message(set_charging_profile_req);
}

TEST_F(SmartChargingTest,
       K01FR22_SetChargingProfileRequest_RejectsChargingStationExternalConstraints) {
    auto periods = create_charging_schedule_periods({0, 1, 2});

    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::ChargingStationExternalConstraints,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    EXPECT_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).Times(0);
    EXPECT_CALL(set_charging_profiles_callback_mock, Call).Times(0);

    smart_charging->handle_message(set_charging_profile_req);
}

TEST_F(SmartChargingTest, K01FR29_SmartChargingCtrlrAvailableIsFalse_RespondsCallError) {
    auto evse_connector_structure = create_evse_connector_structure();
    // auto database_handler = create_database_handler();
    // auto evse_security = std::make_shared<EvseSecurityMock>();
    // configure_callbacks_with_mocks();

    const auto cv = ControllerComponentVariables::SmartChargingCtrlrAvailable;
    this->device_model->set_value(cv.component, cv.variable.value(), AttributeEnum::Actual, "false", "TEST", true);

    auto periods = create_charging_schedule_periods({0, 1, 2});
    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    EXPECT_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).Times(0);

    smart_charging->handle_message(set_charging_profile_req);
}

// TEST_F(SmartChargingTest,
//        K05FR05_RequestStartTransactionRequest_SmartChargingCtrlrEnabledTrue_ValidatesTxProfiles) {
//     const auto cv = ControllerComponentVariables::SmartChargingCtrlrEnabled;
//     this->device_model->set_value(cv.component, cv.variable.value(), AttributeEnum::Actual, "true", "TEST", true);

//     auto periods = create_charging_schedule_periods({0, 1, 2});

//     auto profile = create_charging_profile(
//         DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
//         create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

//     RequestStartTransactionRequest req;
//     req.evseId = DEFAULT_EVSE_ID;
//     req.idToken.idToken = "Local";
//     req.idToken.type = IdTokenEnum::Local;
//     req.chargingProfile = profile;

//     auto start_transaction_req =
//         request_to_enhanced_message<RequestStartTransactionRequest, MessageType::RequestStartTransaction>(req);

//     EXPECT_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).Times(1);
//     // TODO mz this should be called on the charge point
//     smart_charging->handle_message(start_transaction_req);
// }

TEST_F(SmartChargingTest, K01FR29_SmartChargingCtrlrAvailableIsTrue_CallsValidateAndAddProfile) {
    auto evse_connector_structure = create_evse_connector_structure();
    // auto database_handler = create_database_handler();
    // auto evse_security = std::make_shared<EvseSecurityMock>();
    // configure_callbacks_with_mocks();

    const auto cv = ControllerComponentVariables::SmartChargingCtrlrAvailable;
    this->device_model->set_value(cv.component, cv.variable.value(), AttributeEnum::Actual, "true", "TEST", true);

    auto periods = create_charging_schedule_periods({0, 1, 2});
    auto profile = create_charging_profile(
        DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
        create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

    SetChargingProfileRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingProfile = profile;

    auto set_charging_profile_req =
        request_to_enhanced_message<SetChargingProfileRequest, MessageType::SetChargingProfile>(req);

    EXPECT_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).Times(1);

    smart_charging->handle_message(set_charging_profile_req);
}

TEST_F(SmartChargingTest, K08_GetCompositeSchedule_CallsCalculateGetCompositeSchedule) {
    GetCompositeScheduleRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingRateUnit = ChargingRateUnitEnum::W;

    auto get_composite_schedule_req =
        request_to_enhanced_message<GetCompositeScheduleRequest, MessageType::GetCompositeSchedule>(req);

    EXPECT_CALL(*smart_charging_handler_mock, calculate_composite_schedule(testing::_, testing::_, testing::_,
                                                                      DEFAULT_EVSE_ID, req.chargingRateUnit));

    smart_charging->handle_message(get_composite_schedule_req);
}

TEST_F(SmartChargingTest,
       K08_GetCompositeSchedule_CallsCalculateGetCompositeScheduleWithValidProfiles) {
    GetCompositeScheduleRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingRateUnit = ChargingRateUnitEnum::W;

    auto get_composite_schedule_req =
        request_to_enhanced_message<GetCompositeScheduleRequest, MessageType::GetCompositeSchedule>(req);

    std::vector<ChargingProfile> profiles = {
                                             create_charging_profile(DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
                                                                     create_charge_schedule(ChargingRateUnitEnum::A,
                                                                                            create_charging_schedule_periods({0, 1, 2}),
                                                                                            ocpp::DateTime("2024-01-17T17:00:00")),
                                                                     DEFAULT_TX_ID),
                                             };

    ON_CALL(*smart_charging_handler_mock, get_valid_profiles(DEFAULT_EVSE_ID, testing::_))
        .WillByDefault(testing::Return(profiles));
    EXPECT_CALL(*smart_charging_handler_mock,
                calculate_composite_schedule(profiles, testing::_, testing::_, DEFAULT_EVSE_ID, req.chargingRateUnit));

    smart_charging->handle_message(get_composite_schedule_req);
}

TEST_F(SmartChargingTest,
       K08FR05_GetCompositeSchedule_DoesNotCalculateCompositeScheduleForNonexistentEVSE) {
    GetCompositeScheduleRequest req;
    req.evseId = DEFAULT_EVSE_ID + 3;
    req.chargingRateUnit = ChargingRateUnitEnum::W;

    auto get_composite_schedule_req =
        request_to_enhanced_message<GetCompositeScheduleRequest, MessageType::GetCompositeSchedule>(req);

    EXPECT_CALL(*smart_charging_handler_mock, get_valid_profiles(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*smart_charging_handler_mock,
                calculate_composite_schedule(testing::_, testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    smart_charging->handle_message(get_composite_schedule_req);
}

TEST_F(SmartChargingTest,
       K08FR07_GetCompositeSchedule_DoesNotCalculateCompositeScheduleForIncorrectChargingRateUnit) {
    GetCompositeScheduleRequest req;
    req.evseId = DEFAULT_EVSE_ID;
    req.chargingRateUnit = ChargingRateUnitEnum::W;

    auto get_composite_schedule_req =
        request_to_enhanced_message<GetCompositeScheduleRequest, MessageType::GetCompositeSchedule>(req);

    const auto& charging_rate_unit_cv = ControllerComponentVariables::ChargingScheduleChargingRateUnit;
    device_model->set_value(charging_rate_unit_cv.component, charging_rate_unit_cv.variable.value(),
                            AttributeEnum::Actual, "A", "test", true);

    EXPECT_CALL(*smart_charging_handler_mock, get_valid_profiles(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*smart_charging_handler_mock,
                calculate_composite_schedule(testing::_, testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    smart_charging->handle_message(get_composite_schedule_req);
}

// TEST_F(SmartChargingTest,
//        K05FR04_RequestStartTransactionRequest_SmartChargingCtrlrEnabledFalse_DoesNotValidateTxProfiles) {
//     const auto cv = ControllerComponentVariables::SmartChargingCtrlrEnabled;
//     this->device_model->set_value(cv.component, cv.variable.value(), AttributeEnum::Actual, "false", "TEST", true);

//     auto periods = create_charging_schedule_periods({0, 1, 2});

//     ocpp::v201::ChargingProfile profile = create_charging_profile(
//         DEFAULT_PROFILE_ID, ChargingProfilePurposeEnum::TxProfile,
//         create_charge_schedule(ChargingRateUnitEnum::A, periods, ocpp::DateTime("2024-01-17T17:00:00")), DEFAULT_TX_ID);

//     RequestStartTransactionRequest req;
//     req.evseId = DEFAULT_EVSE_ID;
//     req.idToken.idToken = "Local";
//     req.idToken.type = IdTokenEnum::Local;
//     req.chargingProfile = profile;

//     auto start_transaction_req =
//         request_to_enhanced_message<RequestStartTransactionRequest, MessageType::RequestStartTransaction>(req);

//     EXPECT_CALL(*smart_charging_handler_mock, conform_validate_and_add_profile).Times(0);
//        // TODO mz this should be called on the charge point.
//     smart_charging->handle_message(start_transaction_req);
// }
