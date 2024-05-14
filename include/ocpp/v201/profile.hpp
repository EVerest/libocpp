#include <ocpp/v201/ocpp_types.hpp>

namespace ocpp {
namespace v201 {

bool operator==(const ChargingSchedulePeriod& a, const ChargingSchedulePeriod& b);
bool operator!=(const ChargingSchedulePeriod& a, const ChargingSchedulePeriod& b);

bool operator==(const CompositeSchedule& a, const CompositeSchedule& b);
bool operator!=(const CompositeSchedule& a, const CompositeSchedule& b);

bool operator==(const ChargingSchedule& a, const ChargingSchedule& b);
bool operator!=(const ChargingSchedule& a, const ChargingSchedule& b);

struct period_entry_t {
    void init(const ocpp::DateTime& in_start, int in_duration, const ChargingSchedulePeriod& in_period,
              const ChargingProfile& in_profile);
    bool validate(const ChargingProfile& profile, const ocpp::DateTime& now);

    ocpp::DateTime start;
    ocpp::DateTime end;
    float limit;
    std::optional<std::int32_t> number_phases;
    std::optional<std::int32_t> phase_to_use;
    std::int32_t stack_level;
    ChargingRateUnitEnum charging_rate_unit;
    std::optional<float> min_charging_rate;

    bool equals(const period_entry_t& other) const {
        return (start == other.end) && (end == other.end) && (limit == other.limit) &&
               (number_phases == other.number_phases) && (stack_level == other.stack_level) &&
               (charging_rate_unit == other.charging_rate_unit) && (min_charging_rate == other.min_charging_rate);
    }
};

bool operator==(const period_entry_t& a, const period_entry_t& b);
bool operator!=(const period_entry_t& a, const period_entry_t& b);
bool operator==(const std::vector<period_entry_t>& a, const std::vector<period_entry_t>& b);

std::string to_string(const period_entry_t& entry);

std::ostream& operator<<(std::ostream& os, const period_entry_t& entry);

/// \brief calculate the start times for the profile
/// \param now the current date and time
/// \param end the end of the composite schedule
/// \param session_start optional when the charging session started
/// \param profile the charging profile
/// \return a list of the start times of the profile
std::vector<DateTime> calculate_start(const DateTime& now, const DateTime& end,
                                      const std::optional<DateTime>& session_start, const ChargingProfile& profile);

/// \brief Calculates the period entries based upon the indicated time window for every profile passed in.
/// \param now the current date and time
/// \param end the end of the composite schedule
/// \param session_start optional when the charging session started
/// \param profile the charging profile
/// \param period_index the schedule period index
/// \return the list of start times
std::vector<period_entry_t> calculate_profile_entry(const DateTime& now, const DateTime& end,
                                                    const std::optional<DateTime>& session_start,
                                                    const ChargingProfile& profile, std::uint8_t period_index);

/// \brief generate an ordered list of valid schedule periods for the profile
/// \param now the current date and time
/// \param end ignore entries beyond this date and time (i.e. that start after end)
/// \param session_start optional when the charging session started
/// \param profile the charging profile
/// \return a list of profile periods with calculated date & time start and end times
/// \note it is valid for there to be gaps (for recurring profiles)
std::vector<period_entry_t> calculate_profile(const DateTime& now, const DateTime& end,
                                              const std::optional<DateTime>& session_start,
                                              const ChargingProfile& profile);

/// \brief calculate the composite schedule for the list of periods
/// \param combined_schedules the list of periods to build into the schedule
/// \param now the start of the composite schedule
/// \param end the end (i.e. duration) of the composite schedule
/// \param charging_rate_unit the units to use (defaults to Amps)
/// \return the calculated composite schedule
CompositeSchedule calculate_composite_schedule(std::vector<period_entry_t>& combined_schedules, const DateTime& now,
                                               const DateTime& end,
                                               std::optional<ChargingRateUnitEnum> charging_rate_unit);

/// \brief calculate the composite combined schedule
/// \param charge_point_max the composite schedule for ChargePointMax profiles
/// \param tx_default the composite schedule for TxDefault profiles
/// \param tx the composite schedule for Tx profiles
/// \return the calculated combined composite schedule
/// \note all composite schedules must have the same units configured
CompositeSchedule calculate_composite_schedule(const CompositeSchedule& charging_station_max,
                                               const CompositeSchedule& tx_default, const CompositeSchedule& tx);

} // namespace v201
} // namespace ocpp