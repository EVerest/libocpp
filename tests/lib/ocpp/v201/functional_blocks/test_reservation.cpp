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
protected:  // Functions
    void create_device_model_db(const std::string& path) {
        InitDeviceModelDb db(path, MIGRATION_FILES_PATH);
        db.initialize_database(CONFIG_PATH, true);
    }

    std::shared_ptr<DeviceModel> create_device_model(const bool is_reservation_available) {
        create_device_model_db(DEVICE_MODEL_DB_IN_MEMORY_PATH);
        auto device_model_storage = std::make_unique<DeviceModelStorageSqlite>(DEVICE_MODEL_DB_IN_MEMORY_PATH);
        auto device_model = std::make_shared<DeviceModel>(std::move(device_model_storage));
        // Defaults
        const auto& reservation_available = ControllerComponentVariables::ReservationCtrlrAvailable;
        const auto& reservation_enabled = ControllerComponentVariables::ReservationCtrlrAvailable;
        device_model->set_value(reservation_available.component, reservation_available.variable.value(),
                                AttributeEnum::Actual, (is_reservation_available ? "true" : "false"), "test", true);
        device_model->set_value(reservation_enabled.component, reservation_enabled.variable.value(), AttributeEnum::Actual,
                                "true", "test", true);
        return device_model;
    }

protected:    // Members
    MockMessageDispatcher mock_dispatcher;
    EvseManagerFake evse_manager{NR_OF_EVSES};
    std::shared_ptr<DeviceModel> device_model = create_device_model(true);
    MockFunction<ReserveNowStatusEnum(const ReserveNowRequest& request)> reserve_now_callback_mock;
    MockFunction<bool(const int32_t reservationId)> cancel_reservation_callback_mock;
    MockFunction<ocpp::ReservationCheckStatus(const int32_t evse_id, const ocpp::CiString<36> idToken,
                                              const std::optional<ocpp::CiString<36>> groupIdToken)>
        is_reservation_for_token_callback_mock;
};

TEST_F(ReservationTest, handle_reserve_now) {
    Reservation reservation(mock_dispatcher, device_model, evse_manager, reserve_now_callback_mock.AsStdFunction(),
                            cancel_reservation_callback_mock.AsStdFunction(),
                            is_reservation_for_token_callback_mock.AsStdFunction());
    auto bla = NR_OF_EVSES;
}
