// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <everest/timer.hpp>
#include <ocpp/v201/average_meter_values.hpp>

namespace ocpp {
namespace v201 {
AverageMeterValues::AverageMeterValues() {
}
void AverageMeterValues::clear_values() {
    this->aligned_meter_values.clear();
    this->averaged_meter_values.sampledValue.clear();
}

void AverageMeterValues::set_values(const MeterValue& meter_value) {
    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);
    // store all the meter values in the struct
    this->averaged_meter_values = meter_value;

    // avg all the possible measurerands
    for (auto element : meter_value.sampledValue) {
        if (is_avg_meas(element)) {
            MeterValueCalc temp = this->aligned_meter_values[{element.measurand.value(), element.phase.value_or(7)}];
            temp.sum += element.value;
            temp.num_elements++;
            this->aligned_meter_values[{element.measurand.value(), element.phase.value()}] = temp;
        }
    }
}

MeterValue AverageMeterValues::retrieve_processed_values() {
    std::lock_guard<std::mutex> lk(this->avg_meter_value_mutex);
    this->average_meter_value();
    return this->averaged_meter_values;
}

void AverageMeterValues::average_meter_value() {
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
bool AverageMeterValues::is_avg_meas(const SampledValue& sample) {

    // TODO: check up on location values and how they impact the averaging
    if (sample.measurand.has_value()) {
        if ((sample.measurand == MeasurandEnum::Current_Import) || (sample.measurand == MeasurandEnum::Voltage) ||
            (sample.measurand == MeasurandEnum::Power_Active_Import) || (sample.measurand == MeasurandEnum::Frequency))
            return true;
    } else {
        return false;
    }
}
} // namespace v201
} // namespace ocpp