// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <gtest/gtest.h>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>
#include <ocpp/v201/device_model_storage_sqlite.hpp>

namespace ocpp {
namespace v201 {

class DeviceModelTest : public ::testing::Test {
protected:
    const std::string DEVICE_MODEL_DATABASE = "./resources/unittest_device_model.db";
    std::unique_ptr<DeviceModel> dm;
    const RequiredComponentVariable cv = ControllerComponentVariables::AlignedDataInterval;

    void SetUp() override {
        dm =
            std::make_unique<DeviceModel>(std::move(std::make_unique<DeviceModelStorageSqlite>(DEVICE_MODEL_DATABASE)));
    }

    void TearDown() override {
        // reset the value to default
        dm->set_value(cv.component, cv.variable.value(), ocpp::v201::AttributeEnum::Actual, "10");
    }
};

/// \brief Test if value of 0 is allowed for ControllerComponentVariables::AlignedDataInterval although the minLimit is
/// set to 5
TEST_F(DeviceModelTest, test_allow_zero) {
    // default value is 10
    auto r = dm->get_value<int>(cv, ocpp::v201::AttributeEnum::Actual);
    ASSERT_EQ(r, 10);

    // try to set to value of 2, which is not allowed because minLimit of
    auto sv_result = dm->set_value(cv.component, cv.variable.value(), ocpp::v201::AttributeEnum::Actual, "2");
    ASSERT_EQ(sv_result, SetVariableStatusEnum::Rejected);

    // try to set to 0, which is allowed because 0 is an exception
    sv_result = dm->set_value(cv.component, cv.variable.value(), ocpp::v201::AttributeEnum::Actual, "0");
    ASSERT_EQ(sv_result, SetVariableStatusEnum::Accepted);

    r = dm->get_value<int>(cv, ocpp::v201::AttributeEnum::Actual);
    ASSERT_EQ(r, 0);
}

TEST_F(DeviceModelTest, test_component_as_key_in_map) {
    std::map<Component, int32_t> components_to_ints;

    const Component base_comp = {.name = "Foo"};
    components_to_ints[base_comp] = 1;

    const Component different_instance_comp = {
        .name = "Foo",
        .instance = "bar",
    };
    const Component different_evse_comp = {
        .name = "Foo",
        .evse = EVSE{.id = 0},
    };
    const Component different_evse_and_instance_comp = {
        .name = "Foo",
        .evse = EVSE{.id = 0},
        .instance = "bar",
    };
    const Component comp_with_custom_data = {
        .name = "Foo",
        .customData = json::object({{"vendorId", "Baz"}}),
    };
    const Component different_name_comp = {
        .name = "Bar",
    };

    EXPECT_EQ(components_to_ints.find(base_comp)->second, 1);
    EXPECT_EQ(components_to_ints.find(different_instance_comp), components_to_ints.end());
    EXPECT_EQ(components_to_ints.find(different_evse_comp), components_to_ints.end());
    EXPECT_EQ(components_to_ints.find(different_evse_and_instance_comp), components_to_ints.end());
    EXPECT_EQ(components_to_ints.find(comp_with_custom_data)->second, 1);
    EXPECT_EQ(components_to_ints.find(different_name_comp), components_to_ints.end());
}

TEST_F(DeviceModelTest, test_set_monitors) {
    std::vector<SetMonitoringData> requests;

    const EVSE evse = {.id = 2, .connectorId = 3};

    const Component component1 = {
        .name = "UnitTestCtrlr",
        .evse = evse,
    };
    const Component component2 = {.name = "AlignedDataCtrlr"};

    const Variable variable_comp1 = {.name = "UnitTestPropertyAName"};
    const Variable variable_comp2 = {.name = "Interval"};
    
    const SetMonitoringData req_one{.value = 0.0,
                                    .type = MonitorEnum::PeriodicClockAligned,
                                    .severity = 7,
                                    .component = component1,
                                    .variable = variable_comp1};
    const SetMonitoringData req_two{.value = 4.579,
                                    .type = MonitorEnum::UpperThreshold,
                                    .severity = 3,
                                    .component = component2,
                                    .variable = variable_comp2};

    requests.push_back(req_one);
    requests.push_back(req_two);

    auto results = dm->set_monitors(requests);
    ASSERT_EQ(results.size(), requests.size());

    for (auto& result : results) {
        ASSERT_EQ(result.status, SetMonitoringStatusEnum::Accepted);
    }
}

TEST_F(DeviceModelTest, test_get_monitors) {
    std::vector<MonitoringCriterionEnum> criteria = {
        MonitoringCriterionEnum::DeltaMonitoring,
        MonitoringCriterionEnum::PeriodicMonitoring,
        MonitoringCriterionEnum::ThresholdMonitoring,
    };

    const EVSE evse = {.id = 2, .connectorId = 3};

    const Component component1 = {
        .name = "UnitTestCtrlr",
        .evse = evse,
    };
    const Component component2 = {.name = "AlignedDataCtrlr"};

    const Variable variable_comp1 = {.name = "UnitTestPropertyAName"};
    const Variable variable_comp2 = {.name = "Interval"};

    std::vector<ComponentVariable> components = {
        {component1, std::nullopt, variable_comp1},
        {component2, std::nullopt, variable_comp2},
    };

    auto results = dm->get_monitors(criteria, components);
    ASSERT_EQ(results.size(), 2);

    ASSERT_EQ(results[0].variableMonitoring.size(), 1);
    ASSERT_EQ(results[1].variableMonitoring.size(), 1);

    auto monitor1 = results[0].variableMonitoring[0];
    auto monitor2 = results[1].variableMonitoring[0];

    // Valued used above
    const SetMonitoringData req_one{.value = 0.0,
                                    .type = MonitorEnum::PeriodicClockAligned,
                                    .severity = 7,
                                    .component = component1,
                                    .variable = variable_comp1};
    const SetMonitoringData req_two{.value = 4.579,
                                    .type = MonitorEnum::UpperThreshold,
                                    .severity = 3,
                                    .component = component2,
                                    .variable = variable_comp2};

    ASSERT_EQ(monitor1.severity, 7);
    ASSERT_EQ(monitor1.type, MonitorEnum::PeriodicClockAligned);
    ASSERT_EQ(monitor2.severity, 3);
    ASSERT_TRUE(abs(monitor2.value - 4.579) < 0.001); // Nearly equal since it's floating point
}

TEST_F(DeviceModelTest, test_clear_monitors) {
    std::vector<MonitoringCriterionEnum> criteria = {
        MonitoringCriterionEnum::DeltaMonitoring,
        MonitoringCriterionEnum::PeriodicMonitoring,
        MonitoringCriterionEnum::ThresholdMonitoring,
    };

    const EVSE evse = {.id = 2, .connectorId = 3};

    const Component component1 = {
        .name = "UnitTestCtrlr",
        .evse = evse,
    };
    const Component component2 = {.name = "AlignedDataCtrlr"};

    const Variable variable_comp1 = {.name = "UnitTestPropertyAName"};
    const Variable variable_comp2 = {.name = "Interval"};

    std::vector<ComponentVariable> components = {
        {component1, std::nullopt, variable_comp1},
        {component2, std::nullopt, variable_comp2},
    };

    // We just assume ID 1/2
    std::vector<int> to_delete = {1, 2};
    dm->clear_monitors(to_delete);

    auto results = dm->get_monitors(criteria, components);
    ASSERT_EQ(results.size(), 0);
}

} // namespace v201
} // namespace ocpp
