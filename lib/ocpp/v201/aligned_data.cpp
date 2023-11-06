// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/aligned_data.hpp>
#include <everest/logging.hpp>
#include <everest/timer.hpp>

namespace ocpp {
namespace v201 {
AlignedData::AlignedData() {
    EVLOG_info << "constructor";
    // reset the values
    for (auto element : this->averaged_meter_values.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            // calculate and store the sum of all the values
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].sum = 0;
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].num_elements = 0;
        }
    }
}
void AlignedData::clear_values() {
    EVLOG_info << " Clearing the aligned values";
    // reset the values
    for (auto element : this->averaged_meter_values.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            // calculate and store the sum of all the values
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].sum =0;
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].num_elements = 0;
        }
    }
}

void AlignedData::set_values(const MeterValue& meter_value) {
    EVLOG_info << " Setting the aligned values";

    //store all the meter values in the struct
    this->averaged_meter_values = meter_value;

    //avg all the possible measurerands
    for (auto element : meter_value.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            // calculate and store the sum of all the values
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].sum += element.value;
            this->aligned_meter_values[static_cast<int>(element.measurand.value())].num_elements ++;
            EVLOG_info << "sum: " << this->aligned_meter_values[static_cast<int>(element.measurand.value())].sum;
            EVLOG_info << "ele: "
                       << this->aligned_meter_values[static_cast<int>(element.measurand.value())].num_elements;
        }
    }
}

MeterValue AlignedData::get_values() {
    EVLOG_info << "Getting the aligned values";
    this->average_meter_value();
    return this->averaged_meter_values;
}

void AlignedData::average_meter_value() {
    for (auto& element : this->averaged_meter_values.sampledValue) {
        if (is_avg_meas(element.measurand)) {
            element.value = this->aligned_meter_values[static_cast<int>(element.measurand.value())].sum /
                            this->aligned_meter_values[static_cast<int>(element.measurand.value())].num_elements;
            EVLOG_info << "AVG: " << element.value;
        }
    }
}
bool AlignedData::is_avg_meas(const std::optional<ocpp::v201::MeasurandEnum>& meas) {

    if (meas == MeasurandEnum::Voltage) {
        // EVLOG_info << "Meas: " << static_cast<int>(meas.value());
        return true;
    } else {
        return false;
    }
}
} // namespace v201
} // namespace ocpp