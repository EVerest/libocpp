// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include "gmock/gmock.h"

#include "ocpp/v201/evse.hpp"

namespace ocpp::v201 {
class EvseMock : public EvseInterface {
public:
    MOCK_METHOD(int32_t, get_id, (), (const));
    MOCK_METHOD(uint32_t, get_number_of_connectors, (), (const));
    MOCK_METHOD(void, open_transaction,
                (const std::string& transaction_id, const int32_t connector_id, const DateTime& timestamp,
                 const MeterValue& meter_start, const std::optional<IdToken>& id_token,
                 const std::optional<IdToken>& group_id_token, const std::optional<int32_t> reservation_id,
                 ChargingStateEnum charging_state));
    MOCK_METHOD(void, close_transaction,
                (const DateTime& timestamp, const MeterValue& meter_stop, const ReasonEnum& reason));
    MOCK_METHOD(void, start_checking_max_energy_on_invalid_id, ());
    MOCK_METHOD(bool, has_active_transaction, (), (const));
    MOCK_METHOD(bool, has_active_transaction, (int32_t connector_id), (const));
    MOCK_METHOD(void, release_transaction, ());
    MOCK_METHOD(std::unique_ptr<EnhancedTransaction>&, get_transaction, ());
    MOCK_METHOD(void, submit_event, (const int32_t connector_id, ConnectorEvent event));
    MOCK_METHOD(void, on_meter_value, (const MeterValue& meter_value));
    MOCK_METHOD(MeterValue, get_meter_value, ());
    MOCK_METHOD(MeterValue, get_idle_meter_value, ());
    MOCK_METHOD(void, clear_idle_meter_values, ());
    MOCK_METHOD(Connector*, get_connector, (int32_t connector_id));
    MOCK_METHOD(OperationalStatusEnum, get_effective_operational_status, ());
    MOCK_METHOD(void, set_evse_operative_status, (OperationalStatusEnum new_status, bool persist));
    MOCK_METHOD(void, set_connector_operative_status,
                (int32_t connector_id, OperationalStatusEnum new_status, bool persist));
    MOCK_METHOD(void, restore_connector_operative_status, (int32_t connector_id));
    MOCK_METHOD(CurrentPhaseType, get_current_phase_type, ());
};
} // namespace ocpp::v201
