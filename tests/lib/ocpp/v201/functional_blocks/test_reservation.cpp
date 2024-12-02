// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "evse_manager_fake.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <message_dispatcher_mock.hpp>

#include <ocpp/v201/functional_blocks/reservation.hpp>

#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model_storage_sqlite.hpp>
#include <ocpp/v201/init_device_model_db.hpp>

const static std::string MIGRATION_FILES_PATH = "./resources/v201/device_model_migration_files";
const static std::string CONFIG_PATH = "./resources/example_config/v201/component_config";
const static std::string DEVICE_MODEL_DB_IN_MEMORY_PATH = "file::memory:?cache=shared";
const static uint32_t NR_OF_EVSES = 2;

using namespace ocpp::v201;
using ::testing::_;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::Return;

class ReservationTest : public ::testing::Test {
public:
protected: // Functions
    ReservationTest() :
        database_connection(std::make_unique<ocpp::common::DatabaseConnection>(DEVICE_MODEL_DB_IN_MEMORY_PATH)) {
        database_connection->open_connection();
        this->device_model = create_device_model();
        this->reservation = std::make_unique<Reservation>(
            mock_dispatcher, this->device_model, evse_manager, reserve_now_callback_mock.AsStdFunction(),
            cancel_reservation_callback_mock.AsStdFunction(), is_reservation_for_token_callback_mock.AsStdFunction());
    }

    void create_device_model_db(const std::string& path) {
        InitDeviceModelDb db(path, MIGRATION_FILES_PATH);
        db.initialize_database(CONFIG_PATH, true);
    }

    std::shared_ptr<DeviceModel> create_device_model(const bool is_reservation_available = true,
                                                     const bool non_evse_specific_enabled = true) {
        create_device_model_db(DEVICE_MODEL_DB_IN_MEMORY_PATH);
        auto device_model_storage = std::make_unique<DeviceModelStorageSqlite>(DEVICE_MODEL_DB_IN_MEMORY_PATH);
        auto device_model = std::make_shared<DeviceModel>(std::move(device_model_storage));
        // Defaults
        set_reservation_available(device_model, is_reservation_available);
        set_reservation_enabled(device_model, true);
        set_non_evse_specific(device_model, non_evse_specific_enabled);

        // Check values
        const bool reservation_available_in_device_model =
            device_model->get_optional_value<bool>(ControllerComponentVariables::ReservationCtrlrAvailable)
                .value_or(false);
        EXPECT_EQ(reservation_available_in_device_model, is_reservation_available);

        const bool reservation_enabled_in_device_model =
            device_model->get_optional_value<bool>(ControllerComponentVariables::ReservationCtrlrEnabled)
                .value_or(false);
        EXPECT_EQ(reservation_enabled_in_device_model, true);

        const bool non_evse_specific_enabled_device_model =
            device_model->get_optional_value<bool>(ControllerComponentVariables::ReservationCtrlrNonEvseSpecific)
                .value_or(false);
        EXPECT_EQ(non_evse_specific_enabled_device_model, non_evse_specific_enabled);

        return device_model;
    }

    void set_reservation_enabled(std::shared_ptr<DeviceModel> device_model, const bool enabled) {

        const auto& reservation_enabled = ControllerComponentVariables::ReservationCtrlrEnabled;
        EXPECT_EQ(device_model->set_value(reservation_enabled.component, reservation_enabled.variable.value(),
                                          AttributeEnum::Actual, enabled ? "true" : "false", "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    void set_reservation_available(std::shared_ptr<DeviceModel> device_model, const bool available) {
        const auto& reservation_available = ControllerComponentVariables::ReservationCtrlrAvailable;
        EXPECT_EQ(device_model->set_value(reservation_available.component, reservation_available.variable.value(),
                                          AttributeEnum::Actual, (available ? "true" : "false"), "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    void set_non_evse_specific(std::shared_ptr<DeviceModel> device_model, const bool non_evse_specific_enabled) {
        const auto& non_evse_specific = ControllerComponentVariables::ReservationCtrlrNonEvseSpecific;
        EXPECT_EQ(device_model->set_value(non_evse_specific.component, non_evse_specific.variable.value(),
                                          AttributeEnum::Actual, (non_evse_specific_enabled ? "true" : "false"),
                                          "default", true),
                  SetVariableStatusEnum::Accepted);
    }

    ocpp::EnhancedMessage<MessageType>
    create_example_reserve_now_request(const std::optional<int32_t> evse_id = std::nullopt,
                                       const std::optional<ConnectorEnum> connector_type = std::nullopt) {
        ReserveNowRequest request;
        request.connectorType = connector_type;
        request.evseId = evse_id;
        request.id = 1;
        IdToken id_token;
        id_token.idToken = "SOME_TOKEN";
        id_token.type = IdTokenEnum::ISO14443;
        request.idToken = id_token;
        ocpp::Call<ReserveNowRequest> call(request);
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.messageType = MessageType::ReserveNow;
        enhanced_message.message = call;
        return enhanced_message;
    }

    ocpp::EnhancedMessage<MessageType> create_example_cancel_reservation_request(const int32_t reservation_id) {
        CancelReservationRequest request;
        request.reservationId = reservation_id;
        ocpp::Call<CancelReservationRequest> call(request);
        ocpp::EnhancedMessage<MessageType> enhanced_message;
        enhanced_message.messageType = MessageType::CancelReservation;
        enhanced_message.message = call;
        return enhanced_message;
    }

protected: // Members
    // DatabaseConnection as member so the database keeps open and is not destroyed (because this is an in memory
    // database).
    std::unique_ptr<ocpp::common::DatabaseConnection> database_connection;
    MockMessageDispatcher mock_dispatcher;
    EvseManagerFake evse_manager{NR_OF_EVSES};
    std::shared_ptr<DeviceModel> device_model;
    MockFunction<ReserveNowStatusEnum(const ReserveNowRequest& request)> reserve_now_callback_mock;
    MockFunction<bool(const int32_t reservationId)> cancel_reservation_callback_mock;
    MockFunction<ocpp::ReservationCheckStatus(const int32_t evse_id, const ocpp::CiString<36> idToken,
                                              const std::optional<ocpp::CiString<36>> groupIdToken)>
        is_reservation_for_token_callback_mock;
    // Make reservation a unique ptr so we can create it after creating the device model.
    std::unique_ptr<Reservation> reservation;
};

TEST_F(ReservationTest, handle_reserve_now_reservation_not_available) {
    set_reservation_available(this->device_model, false);

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Reservation is not available");
    }));

    EvseMock& m1 = evse_manager.get_mock(1);
    EvseMock& m2 = evse_manager.get_mock(2);

    EXPECT_CALL(m1, get_connector_status(_)).Times(0);
    EXPECT_CALL(m2, get_connector_status(_)).Times(0);

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request();
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_callback_nullptr) {
    Reservation r{mock_dispatcher,
                  this->device_model,
                  evse_manager,
                  nullptr,
                  cancel_reservation_callback_mock.AsStdFunction(),
                  is_reservation_for_token_callback_mock.AsStdFunction()};

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Reservation is not implemented");
    }));

    EvseMock& m1 = evse_manager.get_mock(1);
    EvseMock& m2 = evse_manager.get_mock(2);

    EXPECT_CALL(m1, get_connector_status(_)).Times(0);
    EXPECT_CALL(m2, get_connector_status(_)).Times(0);

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request();
    r.handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_reservation_disabled) {
    set_reservation_enabled(this->device_model, false);

    const bool reservation_enabled_in_device_model =
        this->device_model->get_optional_value<bool>(ControllerComponentVariables::ReservationCtrlrEnabled)
            .value_or(false);
    EXPECT_EQ(reservation_enabled_in_device_model, false);

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Reservation is not enabled");
    }));

    EvseMock& m1 = evse_manager.get_mock(1);
    EvseMock& m2 = evse_manager.get_mock(2);

    EXPECT_CALL(m1, get_connector_status(_)).Times(0);
    EXPECT_CALL(m2, get_connector_status(_)).Times(0);

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request();
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_non_evse_specific_disabled) {
    set_non_evse_specific(this->device_model, false);

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(),
                  "No evse id was given while it should be sent in the request when NonEvseSpecific is disabled");
    }));

    EvseMock& m1 = evse_manager.get_mock(1);
    EvseMock& m2 = evse_manager.get_mock(2);

    EXPECT_CALL(m1, get_connector_status(_)).Times(0);
    EXPECT_CALL(m2, get_connector_status(_)).Times(0);

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request();
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_evse_not_existing) {
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Evse id does not exist");
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request(5);
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_connector_not_existing) {
    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::Pan)).WillOnce(Return(false));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Connector type does not exist");
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request(1, ConnectorEnum::Pan);
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_connectors_not_existing) {
    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cG105)).WillOnce(Return(false));

    EvseMock& m2 = evse_manager.get_mock(2);
    EXPECT_CALL(m2, does_connector_exist(ConnectorEnum::cG105)).WillOnce(Return(false));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Rejected);
        ASSERT_TRUE(response.statusInfo.has_value());
        ASSERT_TRUE(response.statusInfo.value().additionalInfo.has_value());
        EXPECT_EQ(response.statusInfo.value().additionalInfo.value(), "Could not get status info from connector");
    }));

    const ocpp::EnhancedMessage<MessageType> request =
        create_example_reserve_now_request(std::nullopt, ConnectorEnum::cG105);
    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_one_connector_not_existing) {
    std::optional<ConnectorEnum> tesla_connector_type = ConnectorEnum::cTesla;

    const ocpp::EnhancedMessage<MessageType> request =
        create_example_reserve_now_request(std::nullopt, ConnectorEnum::cTesla);

    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(false));

    EvseMock& m2 = evse_manager.get_mock(2);
    EXPECT_CALL(m2, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));
    EXPECT_CALL(m2, get_connector_status(tesla_connector_type)).WillOnce(Return(ConnectorStatusEnum::Unavailable));

    EXPECT_CALL(reserve_now_callback_mock, Call(_)).WillOnce(Return(ReserveNowStatusEnum::Accepted));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Accepted);
    }));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_all_connectors_not_available) {
    std::optional<ConnectorEnum> tesla_connector_type = ConnectorEnum::cTesla;

    const ocpp::EnhancedMessage<MessageType> request =
        create_example_reserve_now_request(std::nullopt, ConnectorEnum::cTesla);

    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));
    EXPECT_CALL(m1, get_connector_status(tesla_connector_type)).WillOnce(Return(ConnectorStatusEnum::Unavailable));

    EvseMock& m2 = evse_manager.get_mock(2);
    EXPECT_CALL(m2, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));
    EXPECT_CALL(m2, get_connector_status(tesla_connector_type)).WillOnce(Return(ConnectorStatusEnum::Unavailable));

    EXPECT_CALL(reserve_now_callback_mock, Call(_)).WillOnce(Return(ReserveNowStatusEnum::Accepted));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Accepted);
    }));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_non_specific_evse_successful) {
    std::optional<ConnectorEnum> tesla_connector_type = ConnectorEnum::cTesla;

    const ocpp::EnhancedMessage<MessageType> request =
        create_example_reserve_now_request(std::nullopt, ConnectorEnum::cTesla);

    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));
    EXPECT_CALL(m1, get_connector_status(tesla_connector_type)).WillOnce(Return(ConnectorStatusEnum::Available));

    EXPECT_CALL(reserve_now_callback_mock, Call(_)).WillOnce(Return(ReserveNowStatusEnum::Accepted));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Accepted);
    }));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_specific_evse_successful) {
    std::optional<ConnectorEnum> tesla_connector_type = ConnectorEnum::cTesla;

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request(2, ConnectorEnum::cTesla);

    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cTesla)).Times(0);
    EXPECT_CALL(m1, get_connector_status(tesla_connector_type)).Times(0);

    EvseMock& m2 = evse_manager.get_mock(2);
    EXPECT_CALL(m2, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));

    EXPECT_CALL(reserve_now_callback_mock, Call(_)).WillOnce(Return(ReserveNowStatusEnum::Accepted));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Accepted);
    }));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_reserve_now_specific_evse_occupied) {
    std::optional<ConnectorEnum> tesla_connector_type = ConnectorEnum::cTesla;

    const ocpp::EnhancedMessage<MessageType> request = create_example_reserve_now_request(2, ConnectorEnum::cTesla);

    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, does_connector_exist(ConnectorEnum::cTesla)).Times(0);
    EXPECT_CALL(m1, get_connector_status(tesla_connector_type)).Times(0);

    EvseMock& m2 = evse_manager.get_mock(2);
    EXPECT_CALL(m2, does_connector_exist(ConnectorEnum::cTesla)).WillOnce(Return(true));

    EXPECT_CALL(reserve_now_callback_mock, Call(_)).WillOnce(Return(ReserveNowStatusEnum::Occupied));

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<ReserveNowResponse>();
        EXPECT_EQ(response.status, ReserveNowStatusEnum::Occupied);
    }));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_cancel_reservation_reservation_not_available) {
    set_reservation_available(this->device_model, false);

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CancelReservationResponse>();
        EXPECT_EQ(response.status, CancelReservationStatusEnum::Rejected);
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_cancel_reservation_request(2);

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_cancel_reservation_reservation_not_enabled) {
    set_reservation_enabled(this->device_model, false);

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CancelReservationResponse>();
        EXPECT_EQ(response.status, CancelReservationStatusEnum::Rejected);
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_cancel_reservation_request(2);

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_cancel_reservation_callback_nullptr) {
    Reservation r{mock_dispatcher, this->device_model,
                  evse_manager,    reserve_now_callback_mock.AsStdFunction(),
                  nullptr,         is_reservation_for_token_callback_mock.AsStdFunction()};

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CancelReservationResponse>();
        EXPECT_EQ(response.status, CancelReservationStatusEnum::Rejected);
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_cancel_reservation_request(2);

    r.handle_message(request);
}

TEST_F(ReservationTest, handle_cancel_reservation_accepted) {
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CancelReservationResponse>();
        EXPECT_EQ(response.status, CancelReservationStatusEnum::Accepted);
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_cancel_reservation_request(2);

    EXPECT_CALL(cancel_reservation_callback_mock, Call(_)).WillOnce(Return(true));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, handle_cancel_reservation_rejected) {
    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<CancelReservationResponse>();
        EXPECT_EQ(response.status, CancelReservationStatusEnum::Rejected);
    }));

    const ocpp::EnhancedMessage<MessageType> request = create_example_cancel_reservation_request(2);

    EXPECT_CALL(cancel_reservation_callback_mock, Call(_)).WillOnce(Return(false));

    this->reservation->handle_message(request);
}

TEST_F(ReservationTest, on_reservation_status) {
    ReservationStatusUpdateRequest request;
    request.reservationId = 3;
    request.reservationUpdateStatus = ReservationUpdateStatusEnum::Removed;
    ocpp::Call<ReservationStatusUpdateRequest> call(request);
    ocpp::EnhancedMessage<MessageType> enhanced_message;
    enhanced_message.messageType = MessageType::CancelReservation;
    enhanced_message.message = call;

    EXPECT_CALL(mock_dispatcher, dispatch_call(_, _)).WillOnce(Invoke([](const json& call, bool triggered) {
        auto response = call[ocpp::CALL_PAYLOAD].get<ReservationStatusUpdateRequest>();
        EXPECT_EQ(response.reservationUpdateStatus, ReservationUpdateStatusEnum::Removed);
        EXPECT_EQ(response.reservationId, 3);
        EXPECT_FALSE(triggered);
    }));

    reservation->on_reservation_status(3, ReservationUpdateStatusEnum::Removed);
}

TEST_F(ReservationTest, is_evse_reserved_for_other) {
    EXPECT_CALL(is_reservation_for_token_callback_mock, Call(42, _, _))
        .WillOnce(Return(ocpp::ReservationCheckStatus::NotReserved));

    EvseMock& m1 = evse_manager.get_mock(1);
    IdToken id_token;
    id_token.idToken = "ID_TOKEN_THINGIE";

    EXPECT_CALL(m1, get_id).WillOnce(Return(42));

    EXPECT_EQ(reservation->is_evse_reserved_for_other(m1, id_token, std::nullopt),
              ocpp::ReservationCheckStatus::NotReserved);
}

TEST_F(ReservationTest, on_reserved) {
    EvseMock& m1 = evse_manager.get_mock(2);
    EXPECT_CALL(m1, submit_event(1, ConnectorEvent::Reserve)).Times(1);

    reservation->on_reserved(2, 1);
}

TEST_F(ReservationTest, on_reservation_cleared) {
    EvseMock& m1 = evse_manager.get_mock(1);
    EXPECT_CALL(m1, submit_event(1, ConnectorEvent::ReservationCleared)).Times(1);

    reservation->on_reserved(1, 1);
}
