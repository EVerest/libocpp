// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include "internal.hpp"

namespace libocpp::test {

std::optional<ocpp::DateTime> get_profile_start_time(ocpp::v16::SmartChargingHandler& handler,
                                                     const ocpp::v16::ChargingProfile& profile,
                                                     const ocpp::DateTime& time, const int connector_id) {
    return handler.get_profile_start_time(profile, time, connector_id);
}

ocpp::DateTime get_next_temp_time(ocpp::v16::SmartChargingHandler& handler, const ocpp::DateTime temp_time,
                                  const std::vector<ocpp::v16::ChargingProfile>& valid_profiles,
                                  const int connector_id) {
    return handler.get_next_temp_time(temp_time, valid_profiles, connector_id);
}

ocpp::v16::PeriodDateTimePair find_period_at(ocpp::v16::SmartChargingHandler& handler, const ocpp::DateTime& time,
                                             const ocpp::v16::ChargingProfile& profile, const int connector_id) {
    return handler.find_period_at(time, profile, connector_id);
}

} // namespace libocpp::test
