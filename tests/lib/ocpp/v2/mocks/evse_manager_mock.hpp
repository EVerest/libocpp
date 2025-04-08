#pragma once

#include "gmock/gmock.h"

#include <ocpp/v2/evse_manager.hpp>

namespace ocpp::v2 {
class EvseManagerMock : public EvseManagerInterface {
public:
    MOCK_METHOD(EvseInterface&, get_evse, (int32_t id));
    MOCK_METHOD(const EvseInterface&, get_evse, (int32_t id), (const));
    MOCK_METHOD(bool, does_connector_exist, (const int32_t evse_id, const ocpp::CiString<20> connector_type), (const));
    MOCK_METHOD(bool, does_evse_exist, (int32_t id), (const));
    MOCK_METHOD(bool, are_all_connectors_effectively_inoperative, (), (const));
    MOCK_METHOD(size_t, get_number_of_evses, (), (const));
    MOCK_METHOD(std::optional<int32_t>, get_transaction_evseid, (const ocpp::CiString<36> &transaction_id), (const));
    MOCK_METHOD(bool, any_transaction_active, (const std::optional<EVSE> &evse), (const));
    MOCK_METHOD(bool, is_valid_evse, (const EVSE &evse), (const));
    MOCK_METHOD(EvseIterator, begin, ());
    MOCK_METHOD(EvseIterator, end, ());
};
}
