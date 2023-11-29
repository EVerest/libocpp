// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#ifndef DEVICE_MODEL_HPP
#define DEVICE_MODEL_HPP

#include <type_traits>

#include <everest/logging.hpp>

#include <ocpp/v201/device_model_storage.hpp>

namespace ocpp {
namespace v201 {

/// \brief Response to requesting a value from the device model
/// \tparam T
template <typename T> struct RequestDeviceModelResponse {
    GetVariableStatusEnum status;
    std::optional<T> value;
};

/// \brief Converts the given \p value to the specific type based on the template parameter
/// \tparam T
/// \param value
/// \return
template <typename T> T to_specific_type(const std::string& value) {
    if constexpr (std::is_same<T, std::string>::value) {
        return value;
    } else if constexpr (std::is_same<T, int>::value) {
        return std::stoi(value);
    } else if constexpr (std::is_same<T, double>::value) {
        return std::stod(value);
    } else if constexpr (std::is_same<T, DateTime>::value) {
        return DateTime(value);
    } else if constexpr (std::is_same<T, bool>::value) {
        return ocpp::conversions::string_to_bool(value);
    } else {
        EVLOG_AND_THROW(std::runtime_error("Requested unknown datatype"));
    }
}

/// \brief This class manages access to the device model representation and to the device model storage and provides
/// functionality to support the use cases defined in the functional block Provisioning
class DeviceModel {

private:
    DeviceModelMap device_model;
    std::unique_ptr<DeviceModelStorage> storage;

    /// \brief Private helper method that does some checks with the device model representation in memory to evaluate if
    /// a value for the given parameters can be requested. If it can be requested it will be retrieved from the device
    /// model storage and the given \p value will be set to the value that was retrieved
    /// \param component_id
    /// \param variable_id
    /// \param attribute_enum
    /// \param value string reference to value: will be set to requested value if value is present
    /// \param allow_write_only true to allow a writeOnly value to be read.
    /// \return GetVariableStatusEnum that indicates the result of the request
    GetVariableStatusEnum request_value_internal(const Component& component_id, const Variable& variable_id,
                                                 const AttributeEnum& attribute_enum, std::string& value,
                                                 bool allow_write_only);

    /// \brief Iterates over the given \p component_criteria and converts this to the variable names
    /// (Active,Available,Enabled,Problem). If any of the variables can not be find as part of a component this function
    /// returns true. If any of those variable's value is true, this function returns true. If all variable's value are
    /// false, this function returns false
    ///  \param component_id
    ///  \param /// component_criteria
    ///  \return
    bool component_criteria_match(const Component& component_id,
                                  const std::vector<ComponentCriterionEnum>& component_criteria);

    /// \brief Sets the variable_id attribute \p value specified by \p component_id , \p variable_id and \p
    /// attribute_enum \param component_id \param variable_id \param attribute_enum \param value
    /// \param force_read_only If this is true, only read-only variables can be changed,
    ///                        otherwise only non read-only variables can be changed
    /// \return Result of the requested operation
    SetVariableStatusEnum set_value_internal(const Component& component_id, const Variable& variable_id,
                                             const AttributeEnum& attribute_enum, const std::string& value,
                                             bool force_read_only);

public:
    /// \brief Constructor for the device model
    /// \param device_model_storage pointer to a device model storage class
    explicit DeviceModel(std::unique_ptr<DeviceModelStorage> device_model_storage);

    /// \brief Direct access to value of a VariableAttribute for the given component, variable and attribute_enum. This
    /// should only be called for variables that have a role standardized in the OCPP2.0.1 specification.
    /// \tparam T datatype of the value that is requested
    /// \param component_variable Combination of Component and Variable that identifies the Variable
    /// \param attribute_enum defaults to AttributeEnum::Actual
    /// \return the requested value from the device model storage
    template <typename T>
    T get_value(const RequiredComponentVariable& component_variable,
                const AttributeEnum& attribute_enum = AttributeEnum::Actual) {
        std::string value;
        auto response = GetVariableStatusEnum::UnknownVariable;
        if (component_variable.variable.has_value()) {
            response = this->request_value_internal(component_variable.component, component_variable.variable.value(),
                                                    attribute_enum, value, true);
        }
        if (response == GetVariableStatusEnum::Accepted) {
            return to_specific_type<T>(value);
        } else {
            EVLOG_critical
                << "Directly requested value for ComponentVariable that doesn't exist in the device model storage: "
                << component_variable;
            EVLOG_AND_THROW(std::runtime_error(
                "Directly requested value for ComponentVariable that doesn't exist in the device model storage."));
        }
    }

    /// \brief  Access to std::optional of a VariableAttribute for the given component, variable and attribute_enum.
    /// \tparam T Type of the value that is requested
    /// \param component_variable Combination of Component and Variable that identifies the Variable
    /// \param attribute_enum
    /// \return std::optional<T> if the combination of \p component_variable and \p attribute_enum could successfully
    /// requested from the storage and a value is present for this combination, else std::nullopt .
    template <typename T>
    std::optional<T> get_optional_value(const ComponentVariable& component_variable,
                                        const AttributeEnum& attribute_enum = AttributeEnum::Actual) {
        std::string value;
        auto response = GetVariableStatusEnum::UnknownVariable;
        if (component_variable.variable.has_value()) {
            response = this->request_value_internal(component_variable.component, component_variable.variable.value(),
                                                    attribute_enum, value, true);
        }
        if (response == GetVariableStatusEnum::Accepted) {
            return to_specific_type<T>(value);
        } else {
            return std::nullopt;
        }
    }

    /// \brief Requests a value of a VariableAttribute specified by combination of \p component_id and \p variable_id
    /// from the device model storage
    /// \tparam T datatype of the value that is requested
    /// \param component_id
    /// \param variable_id
    /// \param attribute_enum
    /// \return Response to request that contains status of the request and the requested value as std::optional<T> .
    /// The value is present if the status is GetVariableStatusEnum::Accepted
    template <typename T>
    RequestDeviceModelResponse<T> request_value(const Component& component_id, const Variable& variable_id,
                                                const AttributeEnum& attribute_enum) {
        std::string value;
        const auto req_status = this->request_value_internal(component_id, variable_id, attribute_enum, value, false);

        if (req_status == GetVariableStatusEnum::Accepted) {
            return {GetVariableStatusEnum::Accepted, to_specific_type<T>(value)};
        } else {
            return {req_status};
        }
    }

    /// \brief Sets the variable_id attribute \p value specified by \p component_id , \p variable_id and \p
    /// attribute_enum
    /// \param component_id
    /// \param variable_id
    /// \param attribute_enum
    /// \param value
    /// \return Result of the requested operation
    SetVariableStatusEnum set_value(const Component& component_id, const Variable& variable_id,
                                    const AttributeEnum& attribute_enum, const std::string& value);

    /// \brief Sets the variable_id attribute \p value specified by \p component_id , \p variable_id and \p
    /// attribute_enum for read only variables only. Only works on certain allowed components.
    /// \param component_id
    /// \param variable_id
    /// \param attribute_enum
    /// \param value
    /// \return Result of the requested operation
    SetVariableStatusEnum set_read_only_value(const Component& component_id, const Variable& variable_id,
                                              const AttributeEnum& attribute_enum, const std::string& value);

    /// \brief Gets the VariableMetaData for the given \p component_id and \p variable_id
    /// \param component_id
    /// \param variable_id
    /// \return VariableMetaData or std::nullopt if \p component_id or \p variable_id not present
    std::optional<VariableMetaData> get_variable_meta_data(const Component& component_id, const Variable& variable_id);

    /// \brief Gets the ReportData for the specifed filter \p report_base \p component_variables and \p
    /// component_criteria
    /// \param report_base
    /// \param component_variables
    /// \param component_criteria
    /// \return
    std::vector<ReportData>
    get_report_data(const std::optional<ReportBaseEnum>& report_base = std::nullopt,
                    const std::optional<std::vector<ComponentVariable>>& component_variables = std::nullopt,
                    const std::optional<std::vector<ComponentCriterionEnum>>& component_criteria = std::nullopt);

    /// \brief Check data integrity of the device model provided by the device model data storage:
    /// For "required" variables, assert values exist. Checks might be extended in the future.
    void check_integrity();
};

} // namespace v201
} // namespace ocpp

#endif // DEVICE_MODEL_HPP
