// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <gmock/gmock.h>

#include "ocpp/v201/device_model_storage.hpp"

namespace ocpp::v201 {
class DeviceModelStorageMock : public DeviceModelStorage {
public:
    // MOCK_METHOD(DeviceModelMap, get_device_model, (), (override));
    MOCK_METHOD(std::optional<VariableAttribute>, get_variable_attribute,
                (const Component&, const Variable&, const AttributeEnum&), (override));
    MOCK_METHOD(std::vector<VariableAttribute>, get_variable_attributes,
                (const Component&, const Variable&, const std::optional<AttributeEnum>&), (override));
    MOCK_METHOD(bool, set_variable_attribute_value,
                (const Component&, const Variable&, const AttributeEnum&, const std::string&, const std::string&),
                (override));
    MOCK_METHOD(std::optional<VariableMonitoringMeta>, set_monitoring_data,
                (const SetMonitoringData&, const VariableMonitorType), (override));
    MOCK_METHOD(bool, update_monitoring_reference, (const int32_t monitor_id, const std::string& reference_value),
                (override));
    MOCK_METHOD(std::vector<VariableMonitoringMeta>, get_monitoring_data,
                (const std::vector<MonitoringCriterionEnum>&, const Component&, const Variable&), (override));
    MOCK_METHOD(ClearMonitoringStatusEnum, clear_variable_monitor, (int, bool), (override));
    MOCK_METHOD(int32_t, clear_custom_variable_monitors, (), (override));
    MOCK_METHOD(void, check_integrity, (), (override));

    DeviceModelMap device_model_map;

    DeviceModelMap get_device_model() override {
        return device_model_map;
    }

    void set_mock_variable(std::string component_name, std::string variable_name, std::string value,
                           DataEnum data_type) {
        Component component{component_name, std::nullopt, std::nullopt, std::nullopt};

        Variable variable{variable_name, std::nullopt, std::nullopt};

        VariableMetaData metadata{
            {data_type, false, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt}};

        this->device_model_map.try_emplace(component, VariableMap{});
        this->device_model_map.at(component).try_emplace(variable, metadata);

        VariableAttribute attribute;
        attribute.type = AttributeEnum::Actual;
        attribute.value = value;

        ON_CALL(*this, get_variable_attribute(component, variable, testing::_))
            .WillByDefault(testing::Return(attribute));
    }
};
} // namespace ocpp::v201
