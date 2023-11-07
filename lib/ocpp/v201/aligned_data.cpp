// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <everest/timer.hpp>
#include <ocpp/v201/aligned_data.hpp>

namespace ocpp {
namespace v201 {
AlignedData::AlignedData() {
    EVLOG_info << "constructor";
    // reset the values
}
void AlignedData::clear_values() {
    EVLOG_info << " Clearing the aligned values";
    // reset the values
    this->aligned_meter_values.clear();
    this->averaged_meter_values.sampledValue.clear();
}

void AlignedData::set_values(const MeterValue& meter_value) {
    EVLOG_info << " Setting the aligned values";

    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);

    // store all the meter values in the struct
    this->averaged_meter_values = meter_value;

    // avg all the possible measurerands
    for (auto element : meter_value.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            AlignedDataValues temp = this->aligned_meter_values[{element.measurand.value(), element.phase.value()}];
            temp.sum += element.value;
            temp.num_elements++;
            this->aligned_meter_values[{element.measurand.value(), element.phase.value()}] = temp;
        }
    }
}

MeterValue AlignedData::get_values() {
    EVLOG_info << "Getting the aligned values";
    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);
    this->average_meter_value();
    return this->averaged_meter_values;
}

void AlignedData::average_meter_value() {
    for (auto& element : this->averaged_meter_values.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            element.value = this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].sum /
                            this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].num_elements;
            EVLOG_info << "Meas: " << element.measurand.value() << " phase: " << element.phase.value()
                       << " sum: " << this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].sum
                       << " num_ele"
                       << this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].num_elements;
            EVLOG_info << "AVG: " << element.value;
        }
    }
}
bool AlignedData::is_avg_meas(const std::optional<ocpp::v201::MeasurandEnum>& meas) {

    if (meas.has_value() && (meas == MeasurandEnum::Voltage)) {
        return true;
    } else {
        return false;
    }
}
} // namespace v201
} // namespace ocpp