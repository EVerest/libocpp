// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <message_dispatcher_mock.hpp>
#include <ocpp/v201/functional_blocks/data_transfer.hpp>

using namespace ocpp::v201;
using ::testing::_;
using ::testing::Invoke;

// Test when the data_transfer_callback is not set, expecting DataTransferStatusEnum::UnknownVendorId
TEST(DataTransferTest, HandleDataTransferReq_NoCallback) {
    MockMessageDispatcher mock_dispatcher;
    DataTransfer data_transfer(mock_dispatcher, std::nullopt); // No callback

    DataTransferRequest request;
    request.vendorId = "test_vendor";
    ocpp::Call<DataTransferRequest> call(request, "unique_id_123");

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<DataTransferResponse>();
        EXPECT_EQ(response.status, DataTransferStatusEnum::UnknownVendorId);
    }));

    data_transfer.handle_data_transfer_req(call);
}

// Test when the data_transfer_callback is set it returns the specific response
TEST(DataTransferTest, HandleDataTransferReq_WithCallback) {
    MockMessageDispatcher mock_dispatcher;

    auto callback = [](const DataTransferRequest&) {
        DataTransferResponse response;
        response.status = DataTransferStatusEnum::Accepted;
        return response;
    };

    DataTransfer data_transfer(mock_dispatcher, callback);

    DataTransferRequest request;
    request.vendorId = "test_vendor";
    ocpp::Call<DataTransferRequest> call(request, "unique_id_123");

    EXPECT_CALL(mock_dispatcher, dispatch_call_result(_)).WillOnce(Invoke([](const json& call_result) {
        auto response = call_result[ocpp::CALLRESULT_PAYLOAD].get<DataTransferResponse>();
        EXPECT_EQ(response.status, DataTransferStatusEnum::Accepted);
    }));

    data_transfer.handle_data_transfer_req(call);
}
