// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/connectivity_manager.hpp>
#include <ocpp/v201/device_model.hpp>

#include "mocks/device_model_storage_mock.hpp"

namespace ocpp {
namespace v201 {

class ConnectivityManagerTest : public ::testing::Test {
public:
    std::unique_ptr<DeviceModelStorageMock> dm_storage_mock_unique{std::make_unique<DeviceModelStorageMock>()};
    DeviceModelStorageMock& dm_storage_mock{*dm_storage_mock_unique.get()};

    std::unique_ptr<DeviceModel> device_model{nullptr};
    std::unique_ptr<ConnectivityManager> connectivity_manager{nullptr};

    ConnectivityManagerTest() {
        dm_storage_mock.set_mock_variable("InternalCtrlr", "ChargePointId", "id1234", DataEnum::string);
        dm_storage_mock.set_mock_variable("SecurityCtrlr", "Identity", "id1234", DataEnum::string);

        dm_storage_mock.set_mock_variable(
            "InternalCtrlr", "NetworkConnectionProfiles",
            R"([{"configurationSlot": 1, "connectionData": {"messageTimeout": 30, "ocppCsmsUrl": "ws://localhost:9000/cp001", "ocppInterface": "Wired0", "ocppTransport": "JSON", "ocppVersion": "OCPP20", "securityProfile": 1}}])",
            DataEnum::string);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "NetworkConfigurationPriority", "1", DataEnum::string);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "RetryBackOffRandomRange", "10", DataEnum::integer);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "RetryBackOffRepeatTimes", "10", DataEnum::integer);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "RetryBackOffWaitMinimum", "10", DataEnum::integer);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "NetworkProfileConnectionAttempts", "10", DataEnum::integer);
        dm_storage_mock.set_mock_variable("InternalCtrlr", "SupportedCiphers12", "", DataEnum::string);
        dm_storage_mock.set_mock_variable("InternalCtrlr", "SupportedCiphers13", "", DataEnum::string);
        dm_storage_mock.set_mock_variable("OCPPCommCtrlr", "WebSocketPingInterval", "10", DataEnum::integer);
    }

    void init_connectivity_manager() {
        this->device_model = std::make_unique<DeviceModel>(std::move(dm_storage_mock_unique));
        this->connectivity_manager = std::make_unique<ConnectivityManager>(*device_model, nullptr, nullptr, nullptr);
    }
};

TEST_F(ConnectivityManagerTest, test_get_network_profile) {
    dm_storage_mock.set_mock_variable("InternalCtrlr", "AllowSecurityLevelZeroConnections", "false", DataEnum::boolean);

    init_connectivity_manager();

    connectivity_manager->start();

    connectivity_manager->get_network_connection_profile(1);
}

} // namespace v201
} // namespace ocpp
