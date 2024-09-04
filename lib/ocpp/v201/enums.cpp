// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <stdexcept>

#include <ocpp/v201/enums.hpp>

namespace ocpp::v201 {

namespace conversions {
/// \brief Converts the given std::string \p s to VariableMonitorType
/// \returns a VariableMonitorType from a string representation
VariableMonitorType string_to_variable_monitor_type(const std::string& s) {
    if (s == "HardWiredMonitor") {
        return VariableMonitorType::HardWiredMonitor;
    }
    if (s == "PreconfiguredMonitor") {
        return VariableMonitorType::PreconfiguredMonitor;
    }
    if (s == "CustomMonitor") {
        return VariableMonitorType::CustomMonitor;
    }

    throw std::out_of_range("Provided string " + s + " could not be converted to enum of type VariableMonitorType");
}

std::string variable_source_enum_to_string(const VariableSource s) {
    switch (s) {
    case VariableSource::OCPP:
        return "OCPP";
    case VariableSource::EVEREST_CORE:
        return "EVEREST_CORE";
    }

    throw std::out_of_range("VariableSource enum is out of range.");
}

VariableSource string_to_variable_source_enum(const std::string& s) {
    if (s == "OCPP") {
        return VariableSource::OCPP;
    }

    if (s == "EVEREST_CORE") {
        return VariableSource::EVEREST_CORE;
    }

    throw std::out_of_range("Provided string " + s + " could not be converted to enum of type VariableSource");
}

} // namespace conversions

} // namespace ocpp::v201
