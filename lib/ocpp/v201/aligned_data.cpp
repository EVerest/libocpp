// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <everest/timer.hpp>
#include <ocpp/v201/aligned_data.hpp>

namespace ocpp {
namespace v201 {
AlignedData::AlignedData() {
    EVLOG_debug << "constructor";
    // reset the values
}
void AlignedData::clear_values() {
    EVLOG_debug << " Clearing the aligned values";
    // reset the values
    this->aligned_meter_values.clear();
    this->averaged_meter_values.sampledValue.clear();
}

void AlignedData::set_values(const MeterValue& meter_value) {
    EVLOG_debug << " Setting the aligned values";

    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);

    // store all the meter values in the struct
    this->averaged_meter_values = meter_value;

    // avg all the possible measurerands
    for (auto element : meter_value.sampledValue) {
        if (is_avg_meas(element)) {
            AlignedDataValues temp = this->aligned_meter_values[{element.measurand.value(), element.phase.value()}];
            temp.sum += element.value;
            temp.num_elements++;
            this->aligned_meter_values[{element.measurand.value(), element.phase.value()}] = temp;
        }
    }
}

MeterValue AlignedData::get_values() {
    EVLOG_debug << "Getting the aligned values";
    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);
    this->average_meter_value();
    return this->averaged_meter_values;
}

void AlignedData::average_meter_value() {
    for (auto& element : this->averaged_meter_values.sampledValue) {
        if (is_avg_meas(element)) {
            element.value = this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].sum /
                            this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].num_elements;
            EVLOG_debug << "Meas: " << element.measurand.value() << " phase: " << element.phase.value() << " sum: "
                        << this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].sum
                        << " num_ele"
                        << this->aligned_meter_values[{element.measurand.value(), element.phase.value()}].num_elements;
            EVLOG_debug << "AVG: " << element.value;
        }
    }
}
bool AlignedData::is_avg_meas(const SampledValue& sample) {

    if (sample.measurand.has_value() && sample.phase.has_value()) {
        if ((sample.measurand == MeasurandEnum::Current_Import) || (sample.measurand == MeasurandEnum::Voltage) ||
            (sample.measurand == MeasurandEnum::Power_Active_Import))
            return true;
    } else {
        return false;
    }
}
} // namespace v201
} // namespace ocpp