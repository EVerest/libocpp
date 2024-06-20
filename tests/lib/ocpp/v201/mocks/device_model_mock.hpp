#include <gmock/gmock.h>

#include "ocpp/v201/device_model.hpp"

namespace ocpp::v201 {
class DeviceModelMock : public DeviceModelInterface {
public:
    MOCK_METHOD(SetVariableStatusEnum, set_value,
                (const Component& component_id, const Variable& variable_id, const AttributeEnum& attribute_enum,
                 const std::string& value, const bool allow_read_only),
                (override));

    MOCK_METHOD(SetVariableStatusEnum, set_read_only_value,
                (const Component& component_id, const Variable& variable_id, const AttributeEnum& attribute_enum,
                 const std::string& value),
                (override));

    MOCK_METHOD(std::optional<VariableMetaData>, get_variable_meta_data,
                (const Component& component_id, const Variable& variable_id), (override));

    MOCK_METHOD(std::vector<ReportData>, get_base_report_data, (const ReportBaseEnum& report_base), (override));

    MOCK_METHOD(std::vector<ReportData>, get_custom_report_data,
                (const std::optional<std::vector<ComponentVariable>>& component_variables,
                 const std::optional<std::vector<ComponentCriterionEnum>>& component_criteria),
                (override));

    MOCK_METHOD(void, check_integrity, ((const std::map<int32_t, int32_t>& evse_connector_structure)), (override));

    MOCK_METHOD(GetVariableStatusEnum, request_value_internal,
                (const Component& component_id, const Variable& variable_id, const AttributeEnum& attribute_enum,
                 std::string& value, bool allow_write_only),
                (override));

    void expect_request_value(const ComponentVariable& component_variable, std::string value,
                              const AttributeEnum& attribute_enum = AttributeEnum::Actual) {
        using testing::_;
        EXPECT_CALL(*this,
                    request_value_internal(component_variable.component, component_variable.variable.value(), _, _, _))
            .WillOnce(
                testing::DoAll(testing::SetArgReferee<3>(value), testing::Return(GetVariableStatusEnum::Accepted)));
    }
};
} // namespace ocpp::v201
