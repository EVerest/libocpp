// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <everest/logging.hpp>
#include <everest/timer.hpp>
#include <ocpp/common/types.hpp>
#include <ocpp/v201/enums.hpp>
#include <ocpp/v201/ocpp_types.hpp>
#include <ocpp/v201/types.hpp>
#include <ocpp/v201/utils.hpp>
#include <vector>
namespace ocpp {
namespace v201 {
class AlignedData {

public:
    AlignedData();
    void clear_values();
    void set_values(const MeterValue& meter_value);
    MeterValue get_values();

private:
    struct AlignedDataValues {
        double sum;
        int num_elements;
    };
    struct DataMeasurands {
        MeasurandEnum measurand;
        PhaseEnum phase;

        // Define a comparison operator for the struct
        bool operator<(const DataMeasurands& other) const {
            // Compare based on name, then age
            if (measurand != other.measurand) {
                return measurand < other.measurand;
            }
            return phase < other.phase;
        }
    };

    MeterValue averaged_meter_values;
    std::mutex avg_meter_value_mutex;
    std::map<DataMeasurands, AlignedDataValues> aligned_meter_values;
    bool is_avg_meas(const std::optional<ocpp::v201::MeasurandEnum>& meas);
    void average_meter_value();
};
} // namespace v201

} // namespace ocpp