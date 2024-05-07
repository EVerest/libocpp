#include "ocpp/v201/charge_point.hpp"
#include "gmock/gmock.h"
#include <gmock/gmock.h>

namespace ocpp::v201 {

TEST(ChargePointFixture, CallbacksAreValidWhenAllRequiredCallbacksProvided) {
    ocpp::v201::Callbacks callbacks;
    EXPECT_FALSE(callbacks.all_callbacks_valid());

    testing::MockFunction<bool(const std::optional<const int32_t> evse_id, const ResetEnum& reset_type)> is_reset_allowed_callback_mock;
    testing::MockFunction<void(const std::optional<const int32_t> evse_id, const ResetEnum& reset_type)> reset_callback_mock;
    testing::MockFunction<void(const int32_t evse_id, const ReasonEnum& stop_reason)> stop_transaction_callback_mock;
    testing::MockFunction<void(const int32_t evse_id)> pause_charging_callback_mock;
    testing::MockFunction<void(const int32_t evse_id, const int32_t connector_id, const OperationalStatusEnum new_status)> connector_effective_operative_status_changed_callback_mock;
    testing::MockFunction<GetLogResponse(const GetLogRequest& request)> get_log_request_callback_mock;
    testing::MockFunction<UnlockConnectorResponse(const int32_t evse_id, const int32_t connecor_id)> unlock_connector_callback_mock;
    testing::MockFunction<void(const RequestStartTransactionRequest& request, const bool authorize_remote_start)> remote_start_transaction_callback_mock;
    testing::MockFunction<bool(const int32_t evse_id, const CiString<36> idToken, const std::optional<CiString<36>> groupIdToken)> is_reservation_for_token_callback_mock;
    testing::MockFunction<UpdateFirmwareResponse(const UpdateFirmwareRequest& request)> update_firmware_request_callback_mock;

    callbacks.is_reset_allowed_callback = is_reset_allowed_callback_mock.AsStdFunction();
    callbacks.reset_callback = reset_callback_mock.AsStdFunction();
    callbacks.stop_transaction_callback = stop_transaction_callback_mock.AsStdFunction();
    callbacks.pause_charging_callback = pause_charging_callback_mock.AsStdFunction();
    callbacks.connector_effective_operative_status_changed_callback = connector_effective_operative_status_changed_callback_mock.AsStdFunction();
    callbacks.get_log_request_callback = get_log_request_callback_mock.AsStdFunction();
    callbacks.unlock_connector_callback = unlock_connector_callback_mock.AsStdFunction();
    callbacks.remote_start_transaction_callback = remote_start_transaction_callback_mock.AsStdFunction();
    callbacks.is_reservation_for_token_callback = is_reservation_for_token_callback_mock.AsStdFunction();
    callbacks.update_firmware_request_callback = update_firmware_request_callback_mock.AsStdFunction();

    EXPECT_TRUE(callbacks.all_callbacks_valid());
}
}