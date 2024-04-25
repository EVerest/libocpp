// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <ocpp/v16/smart_charging.hpp>

namespace libocpp::test {
std::optional<ocpp::DateTime> get_profile_start_time(ocpp::v16::SmartChargingHandler& handler,
                                                     const ocpp::v16::ChargingProfile& profile,
                                                     const ocpp::DateTime& time, const int connector_id);
ocpp::DateTime get_next_temp_time(ocpp::v16::SmartChargingHandler& handler, const ocpp::DateTime temp_time,
                                  const std::vector<ocpp::v16::ChargingProfile>& valid_profiles,
                                  const int connector_id);
ocpp::v16::PeriodDateTimePair find_period_at(ocpp::v16::SmartChargingHandler& handler, const ocpp::DateTime& time,
                                             const ocpp::v16::ChargingProfile& profile, const int connector_id);

} // namespace libocpp::test

#endif // INTERNAL_H_