// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <chrono>
#include <limits>
#include <optional>

#include <ocpp/common/constants.hpp>
#include <ocpp/common/types.hpp>
#include <ocpp/v16/ocpp_enums.hpp>
#include <ocpp/v16/ocpp_types.hpp>
#include <ocpp/v16/profile.hpp>
#include <ocpp/v16/smart_charging.hpp>

#include <everest/logging.hpp>

using std::chrono::duration_cast;
using std::chrono::seconds;

namespace {

using ocpp::v16::EnhancedChargingSchedulePeriod;

/// \brief update the iterator when the current period has elapsed
/// \param[in] schedule_duration the time in seconds from the start of the composite schedule
/// \param[inout] itt the iterator for the periods in the schedule
/// \param[in] end the item beyond the last period in the schedule
/// \param[out] period the details of the current period in the schedule
/// \param[out] period_duration how long this period lasts
///
/// \note period_duration is defined by the startPeriod of the next period or forever when
///       there is no next period.
void update_itt(const int schedule_duration, std::vector<EnhancedChargingSchedulePeriod>::const_iterator& itt,
                const std::vector<EnhancedChargingSchedulePeriod>::const_iterator& end,
                EnhancedChargingSchedulePeriod& period, int& period_duration) {
    if (itt != end) {
        // default is to remain in the current period
        period = *itt;

        /*
         * calculate the duration of this period:
         * - the startPeriod of the next period in the vector, or
         * - forever where there is no next period
         */
        auto next = std::next(itt);
        period_duration = (next != end) ? next->startPeriod : std::numeric_limits<int>::max();

        if (schedule_duration >= period_duration) {
            /*
             * when the current duration is beyond the duration of this period
             * move to the next period in the vector and recalculate the period duration
             * (the handling for being at the last element is below)
             */
            itt++;
            if (itt != end) {
                period = *itt;
                next = std::next(itt);
                period_duration = (next != end) ? next->startPeriod : std::numeric_limits<int>::max();
            }
        }
    }

    /*
     * all periods in the schedule have been used
     * i.e. there are no future periods to consider in this schedule
     */
    if (itt == end) {
        period.startPeriod = -1;
        period_duration = std::numeric_limits<int>::max();
    }
}

} // namespace

namespace ocpp::v16 {

int32_t elapsed_seconds(const ocpp::DateTime& to, const ocpp::DateTime& from) {
    return duration_cast<seconds>(to.to_time_point() - from.to_time_point()).count();
}

ocpp::DateTime floor_seconds(const ocpp::DateTime& dt) {
    return ocpp::DateTime(std::chrono::floor<seconds>(dt.to_time_point()));
}

constexpr bool operator==(const ChargingSchedulePeriod& lhs, const ChargingSchedulePeriod& rhs) {
    return (lhs.startPeriod == rhs.startPeriod) && (lhs.limit == rhs.limit);
}

constexpr bool operator!=(const ChargingSchedulePeriod& lhs, const ChargingSchedulePeriod& rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const period_entry_t& entry) {
    os << entry.start << " " << entry.end << " S:" << entry.stack_level << " " << entry.limit
       << ((entry.charging_rate_unit == ChargingRateUnit::A) ? "A" : "W");
    return os;
}

/// \brief populate a schedule period
/// \param in_start the start time of the profile
/// \param in_duration the time in seconds from the start of the profile to the end of this period
/// \param in_period the details of this period
void period_entry_t::init(const DateTime& in_start, int in_duration, const ChargingSchedulePeriod& in_period,
                          const ChargingProfile& in_profile) {
    // note duration can be negative and hence end time is before start time
    // see period_entry_t::validate()
    const auto start_tp = std::chrono::floor<seconds>(in_start.to_time_point());
    start = std::move(DateTime(start_tp + seconds(in_period.startPeriod)));
    end = std::move(DateTime(start_tp + seconds(in_duration)));
    limit = in_period.limit;
    number_phases = in_period.numberPhases;
    stack_level = in_profile.stackLevel;
    charging_rate_unit = in_profile.chargingSchedule.chargingRateUnit;
    min_charging_rate = in_profile.chargingSchedule.minChargingRate;
}

/// \brief validate and update entry based on profile
/// \param profile the profile this entry is part of
/// \param now the current date and time
/// \return true when the entry is valid
bool period_entry_t::validate(const ChargingProfile& profile, const DateTime& now) {
    bool b_valid{true};

    if (profile.validFrom) {
        const auto valid_from = floor_seconds(profile.validFrom.value());
        if (valid_from > start) {
            // the calculated start is before the profile is valid
            if (valid_from >= end) {
                // the whole period isn't valid
                b_valid = false;
            } else {
                // adjust start to match validFrom
                start = valid_from;
            }
        }
    }

    b_valid = b_valid && end > start; // check end time is after the start time
    b_valid = b_valid && end > now;   // ignore expired periods
    return b_valid;
}

/// \brief calculate the start times for the profile
/// \param in_now the current date and time
/// \param in_end the end of the composite schedule
/// \param in_session_start optional when the charging session started
/// \param in_profile the charging profile
/// \return a list of the start times of the profile
std::vector<DateTime> calculate_start(const DateTime& in_now, const DateTime& in_end,
                                      const std::optional<DateTime>& in_session_start,
                                      const ChargingProfile& in_profile) {
    /*
     * Absolute schedules start at the defined startSchedule
     * Relative schedules start at session start
     * Recurring schedules start based on startSchedule and the current date/time
     * start can be affected by the profile validFrom. See period_entry_t::validate()
     */

    std::vector<DateTime> start_times;
    DateTime start = floor_seconds(in_now); // fallback when a better value can't be found

    switch (in_profile.chargingProfileKind) {
    case ChargingProfileKindType::Absolute:
        if (in_profile.chargingSchedule.startSchedule) {
            start = in_profile.chargingSchedule.startSchedule.value();
        } else {
            // Absolute should have a startSchedule
            EVLOG_warning << "Absolute charging profile (" << in_profile.chargingProfileId << ") without startSchedule";

            // use validFrom where available
            if (in_profile.validFrom) {
                start = in_profile.validFrom.value();
            }
        }
        start_times.push_back(floor_seconds(start));
        break;
    case ChargingProfileKindType::Recurring:
        if (in_profile.recurrencyKind && in_profile.chargingSchedule.startSchedule) {
            const auto start_schedule = floor_seconds(in_profile.chargingSchedule.startSchedule.value());
            const auto end = floor_seconds(in_end);
            const auto now_tp = start.to_time_point();
            int seconds_to_go_back{0};
            int seconds_to_go_forward{0};

            /*
             example problem case:
             - allow daily charging 08:00 to 18:00
               at 07:00 and 19:00 what should the start time be?

             a) profile could have 1 period (32A) at 0s with a duration of 36000s (10 hours)
                relying on a lower stack level to deny charging
             b) profile could have 2 periods (32A) at 0s and (0A) at 36000s (10 hours)
                i.e. the profile covers the full 24 hours

             at 07:00 is the start time in 1 hour, or 23 hours ago?

             23 hours ago is the chosen result - however the profile code needs to consider that
             a new daily profile is about to start hence the next start time is provided.

             Weekly has a similar problem
            */

            switch (in_profile.recurrencyKind.value()) {
            case RecurrencyKindType::Daily:
                seconds_to_go_back = duration_cast<seconds>(now_tp - start_schedule.to_time_point()).count() %
                                     (HOURS_PER_DAY * SECONDS_PER_HOUR);
                if (seconds_to_go_back < 0) {
                    seconds_to_go_back += HOURS_PER_DAY * SECONDS_PER_HOUR;
                }
                seconds_to_go_forward = HOURS_PER_DAY * SECONDS_PER_HOUR;
                break;
            case RecurrencyKindType::Weekly:
                seconds_to_go_back = duration_cast<seconds>(now_tp - start_schedule.to_time_point()).count() %
                                     (SECONDS_PER_DAY * DAYS_PER_WEEK);
                if (seconds_to_go_back < 0) {
                    seconds_to_go_back += SECONDS_PER_DAY * DAYS_PER_WEEK;
                }
                seconds_to_go_forward = SECONDS_PER_DAY * DAYS_PER_WEEK;
                break;
            }

            start = std::move(DateTime(now_tp - seconds(seconds_to_go_back)));

            while (start <= end) {
                start_times.push_back(start);
                start = DateTime(start.to_time_point() + seconds(seconds_to_go_forward));
            }
        }
        break;
    case ChargingProfileKindType::Relative:
        // if there isn't a session start then assume the session starts now
        if (in_session_start) {
            start = floor_seconds(in_session_start.value());
        }
        start_times.push_back(start);
        break;
    default:
        EVLOG_error << "Invalid ChargingProfileKindType: " << static_cast<int>(in_profile.chargingProfileKind);
        break;
    }

    return start_times;
}

/// \brief calculate the start times for the schedule period
/// \param in_now the current date and time
/// \param in_end the end of the composite schedule
/// \param in_session_start optional when the charging session started
/// \param in_profile the charging profile
/// \param in_period_index the schedule period index
/// \return the list of start times
std::vector<period_entry_t> calculate_profile_entry(const DateTime& in_now, const DateTime& in_end,
                                                    const std::optional<DateTime>& in_session_start,
                                                    const ChargingProfile& in_profile, std::uint8_t in_period_index) {
    std::vector<period_entry_t> entries;

    if (in_period_index >= in_profile.chargingSchedule.chargingSchedulePeriod.size()) {
        EVLOG_error << "Invalid schedule period index [" << static_cast<int>(in_period_index)
                    << "] (too large) for profile " << in_profile.chargingProfileId;
    } else {
        const auto& this_period = in_profile.chargingSchedule.chargingSchedulePeriod[in_period_index];

        if ((in_period_index == 0) && (this_period.startPeriod != 0)) {
            // invalid profile - first period must be 0
            EVLOG_error << "Invalid schedule period index [0] startPeriod " << this_period.startPeriod
                        << " for profile " << in_profile.chargingProfileId;
        } else if ((in_period_index > 0) &&
                   (in_profile.chargingSchedule.chargingSchedulePeriod[in_period_index - 1].startPeriod >=
                    this_period.startPeriod)) {
            // invalid profile - periods must be in order and with increasing startPeriod values
            EVLOG_error << "Invalid schedule period index [" << static_cast<int>(in_period_index) << "] startPeriod "
                        << this_period.startPeriod << " for profile " << in_profile.chargingProfileId;
        } else {
            const bool has_next_period =
                (in_period_index + 1) < in_profile.chargingSchedule.chargingSchedulePeriod.size();

            // start time(s) of the schedule
            // the start time of this period is calculated in period_entry_t::init()
            const auto schedule_start = calculate_start(in_now, in_end, in_session_start, in_profile);

            for (std::uint8_t i = 0; i < schedule_start.size(); i++) {
                const bool has_next_occurrance = (i + 1) < schedule_start.size();
                const auto& entry_start = schedule_start[i];

                /*
                 * The duration of this period (from the start of the schedule) is the sooner of
                 * - forever
                 * - next period start time
                 * - optional duration
                 * - the start of the next recurrence
                 * - optional validTo
                 */

                int duration = std::numeric_limits<int>::max(); // forever

                if (has_next_period) {
                    duration = in_profile.chargingSchedule.chargingSchedulePeriod[in_period_index + 1].startPeriod;
                }

                // check optional chargingSchedule duration field
                if (in_profile.chargingSchedule.duration && (in_profile.chargingSchedule.duration.value() < duration)) {
                    duration = in_profile.chargingSchedule.duration.value();
                }

                // check duration doesn't extend into the next recurrence
                if (has_next_occurrance) {
                    const auto next_start =
                        duration_cast<seconds>(schedule_start[i + 1].to_time_point() - entry_start.to_time_point())
                            .count();
                    if (next_start < duration) {
                        duration = next_start;
                    }
                }

                // check duration doesn't extend beyond profile validity
                if (in_profile.validTo) {
                    // note can be negative
                    const auto valid_to = floor_seconds(in_profile.validTo.value());
                    const auto valid_to_seconds =
                        duration_cast<seconds>(valid_to.to_time_point() - entry_start.to_time_point()).count();
                    if (valid_to_seconds < duration) {
                        duration = valid_to_seconds;
                    }
                }

                period_entry_t entry;
                entry.init(entry_start, duration, this_period, in_profile);
                const auto now = floor_seconds(in_now);

                if (entry.validate(in_profile, now)) {
                    entries.push_back(std::move(entry));
                }
            }
        }
    }

    return entries;
}

std::vector<period_entry_t> calculate_profile_unsorted(const DateTime& now, const DateTime& end,
                                                       const std::optional<DateTime>& session_start,
                                                       const ChargingProfile& profile) {
    std::vector<period_entry_t> entries;

    const auto nr_of_entries = profile.chargingSchedule.chargingSchedulePeriod.size();
    for (uint8_t i = 0; i < nr_of_entries; i++) {
        const auto results = calculate_profile_entry(now, end, session_start, profile, i);
        for (const auto& entry : results) {
            if (entry.start <= end) {
                entries.push_back(entry);
            }
        }
    }

    return entries;
}

void sort_periods_into_date_order(std::vector<period_entry_t>& periods) {
    // sort into date order
    struct {
        bool operator()(const period_entry_t& a, const period_entry_t& b) const {
            // earliest first
            return a.start < b.start;
        }
    } less_than;
    std::sort(periods.begin(), periods.end(), less_than);
}

std::vector<period_entry_t> calculate_profile(const DateTime& now, const DateTime& end,
                                              const std::optional<DateTime>& session_start,
                                              const ChargingProfile& profile) {
    std::vector<period_entry_t> entries = calculate_profile_unsorted(now, end, session_start, profile);

    sort_periods_into_date_order(entries);
    return entries;
}

std::vector<period_entry_t> calculate_all_profiles(const DateTime& now, const DateTime& end,
                                                   const std::optional<DateTime>& session_start,
                                                   const std::vector<ChargingProfile>& profiles,
                                                   ChargingProfilePurposeType purpose) {
    std::vector<period_entry_t> output;
    for (const auto& profile : profiles) {
        if (profile.chargingProfilePurpose == purpose) {
            std::vector<period_entry_t> periods = calculate_profile_unsorted(now, end, session_start, profile);
            output.insert(output.end(), periods.begin(), periods.end());
        }
    }

    sort_periods_into_date_order(output);
    return output;
}

IntermediateProfile generate_profile_from_periods(std::vector<period_entry_t>& periods, const DateTime& in_now,
                                                  const DateTime& in_end) {

    const auto now = floor_seconds(in_now);
    const auto end = floor_seconds(in_end);

    if (periods.empty()) {
        return {{0, NO_LIMIT_SPECIFIED, NO_LIMIT_SPECIFIED, 0, 0, std::nullopt, std::nullopt}};
    }

    // sort the combined_schedules in stack priority order
    struct {
        bool operator()(const period_entry_t& a, const period_entry_t& b) const {
            // highest stack level first
            return a.stack_level > b.stack_level;
        }
    } less_than;
    std::sort(periods.begin(), periods.end(), less_than);

    IntermediateProfile combined{};
    DateTime current = now;

    while (current < end) {
        // find schedule to use for time: current
        DateTime earliest = end;
        DateTime next_earliest = end;
        const period_entry_t* chosen{nullptr};

        for (const auto& schedule : periods) {
            if (schedule.start <= earliest) {
                // ensure the earlier schedule is valid at the current time
                if (schedule.end > current) {
                    next_earliest = earliest;
                    earliest = schedule.start;
                    chosen = &schedule;
                    if (earliest <= current) {
                        break;
                    }
                }
            }
        }

        if (earliest > current) {
            // there is a gap to fill
            combined.push_back({elapsed_seconds(current, now), NO_LIMIT_SPECIFIED, NO_LIMIT_SPECIFIED, 0, 0,
                                std::nullopt, std::nullopt});
            current = earliest;
        } else {
            // there is a schedule to use
            float current_limit = NO_LIMIT_SPECIFIED;
            float power_limit = NO_LIMIT_SPECIFIED;
            int32_t stack_level_current = 0;
            int32_t stack_level_power = 0;

            if (chosen->charging_rate_unit == ChargingRateUnit::A) {
                current_limit = chosen->limit;
                stack_level_current = chosen->stack_level;
            } else {
                power_limit = chosen->limit;
                stack_level_power = chosen->stack_level;
            }

            IntermediatePeriod charging_schedule_period{
                elapsed_seconds(current, now), current_limit, power_limit, stack_level_current, stack_level_power,
                chosen->number_phases,         std::nullopt};

            combined.push_back(charging_schedule_period);
            if (chosen->end < next_earliest) {
                current = chosen->end;
            } else {
                current = next_earliest;
            }
        }
    }

    return combined;
}

namespace {

using period_iterator = IntermediateProfile::const_iterator;
using period_pair_vector = std::vector<std::pair<period_iterator, period_iterator>>;
using IntermediateProfileRef = std::reference_wrapper<const IntermediateProfile>;

inline std::vector<IntermediateProfileRef> convert_to_ref_vector(const std::vector<IntermediateProfile>& profiles) {
    std::vector<IntermediateProfileRef> references{};
    for (auto& profile : profiles) {
        references.push_back(profile);
    }
    return references;
}

IntermediateProfile combine_list_of_profiles(const std::vector<IntermediateProfileRef>& profiles,
                                             std::function<IntermediatePeriod(const period_pair_vector&)> combinator) {
    if (profiles.empty()) {
        // We should never get here as there are always profiles, otherwise there is a mistake in the calling function
        // Return an empty profile to be safe
        return {{0, NO_LIMIT_SPECIFIED, NO_LIMIT_SPECIFIED, 0, 0, std::nullopt, std::nullopt}};
    }

    IntermediateProfile combined{};

    period_pair_vector profile_iterators{};
    for (const auto& wrapped_profile : profiles) {
        auto& profile = wrapped_profile.get();
        if (!profile.empty()) {
            profile_iterators.push_back(std::make_pair(profile.begin(), profile.end()));
        }
    }

    int32_t current_period = 0;
    while (std::any_of(profile_iterators.begin(), profile_iterators.end(),
                       [](const std::pair<period_iterator, period_iterator>& it) { return it.first != it.second; })) {

        IntermediatePeriod period = combinator(profile_iterators);
        period.startPeriod = current_period;

        if (combined.empty() || (period.current_limit != combined.back().current_limit) ||
            (period.power_limit != combined.back().power_limit) ||
            (period.numberPhases != combined.back().numberPhases) ||
            (period.stack_level_current != combined.back().stack_level_current) ||
            (period.stack_level_power != combined.back().stack_level_power)) {
            combined.push_back(period);
        }

        // Determine the next earliest period
        int32_t next_lowest_period = std::numeric_limits<int32_t>::max();

        for (const auto& [it, end] : profile_iterators) {
            auto next = it + 1;
            if (next != end && next->startPeriod > current_period && next->startPeriod < next_lowest_period) {
                next_lowest_period = next->startPeriod;
            }
        }

        // If there is none, we are done
        if (next_lowest_period == std::numeric_limits<int32_t>::max()) {
            break;
        }

        // Otherwise update to next earliest period
        for (auto& [it, end] : profile_iterators) {
            auto next = it + 1;
            if (next != end && next->startPeriod == next_lowest_period) {
                it++;
            }
        }
        current_period = next_lowest_period;
    }

    if (combined.empty()) {
        combined.push_back({0, NO_LIMIT_SPECIFIED, NO_LIMIT_SPECIFIED, 0, 0, std::nullopt, std::nullopt});
    }

    return combined;
}

} // namespace

IntermediateProfile merge_tx_profile_with_tx_default_profile(const IntermediateProfile& tx_profile,
                                                             const IntermediateProfile& tx_default_profile) {

    auto combinator = [](const period_pair_vector& periods) {
        IntermediatePeriod period{};
        period.current_limit = NO_LIMIT_SPECIFIED;
        period.power_limit = NO_LIMIT_SPECIFIED;
        period.stack_level_current = 0;
        period.stack_level_power = 0;

        for (const auto& [it, end] : periods) {
            if (it->current_limit != NO_LIMIT_SPECIFIED || it->power_limit != NO_LIMIT_SPECIFIED) {
                period.current_limit = it->current_limit;
                period.power_limit = it->power_limit;
                period.numberPhases = it->numberPhases;
                period.stack_level_current = it->stack_level_current;
                period.stack_level_power = it->stack_level_power;
                break;
            }
        }

        return period;
    };

    // This ordering together with the combinator will prefer the tx_profile above the default profile
    std::vector<IntermediateProfileRef> profiles{tx_profile, tx_default_profile};

    return combine_list_of_profiles(profiles, combinator);
}

IntermediateProfile merge_profiles_by_lowest_limit(const std::vector<IntermediateProfile>& profiles) {
    auto combinator = [](const period_pair_vector& periods) {
        IntermediatePeriod period{};
        period.current_limit = std::numeric_limits<float>::max();
        period.power_limit = std::numeric_limits<float>::max();

        for (const auto& [it, end] : periods) {
            if (it->current_limit >= 0.0F && it->current_limit < period.current_limit) {
                period.current_limit = it->current_limit;
                period.stack_level_current = it->stack_level_current;
            }
            if (it->power_limit >= 0.0F && it->power_limit < period.power_limit) {
                period.power_limit = it->power_limit;
                period.stack_level_power = it->stack_level_power;
            }

            // Copy number of phases if lower
            if (!period.numberPhases.has_value()) {
                // Don't care if this copies a nullopt, thats what it was already
                period.numberPhases = it->numberPhases;
            } else if (it->numberPhases.has_value() && it->numberPhases.value() < period.numberPhases.value()) {
                period.numberPhases = it->numberPhases;
            }
        }

        if (period.current_limit == std::numeric_limits<float>::max()) {
            period.current_limit = NO_LIMIT_SPECIFIED;
            period.stack_level_current = 0;
        }
        if (period.power_limit == std::numeric_limits<float>::max()) {
            period.power_limit = NO_LIMIT_SPECIFIED;
            period.stack_level_power = 0;
        }

        return period;
    };

    return combine_list_of_profiles(convert_to_ref_vector(profiles), combinator);
}

IntermediateProfile merge_profiles_by_summing_limits(const std::vector<IntermediateProfile>& profiles,
                                                     float current_default, float power_default) {
    auto combinator = [current_default, power_default](const period_pair_vector& periods) {
        IntermediatePeriod period{};
        for (const auto& [it, end] : periods) {
            period.current_limit += it->current_limit >= 0.0F ? it->current_limit : current_default;
            period.power_limit += it->power_limit >= 0.0F ? it->power_limit : power_default;
            period.stack_level_current = 0; // Stack level cant be determined when summing intermediate profiles
            period.stack_level_power = 0;   // Stack level cant be determined when summing intermediate profiles

            // Copy number of phases if higher
            if (!period.numberPhases.has_value()) {
                // Don't care if this copies a nullopt, thats what it was already
                period.numberPhases = it->numberPhases;
            } else if (it->numberPhases.has_value() && it->numberPhases.value() > period.numberPhases.value()) {
                period.numberPhases = it->numberPhases;
            }
        }
        return period;
    };

    return combine_list_of_profiles(convert_to_ref_vector(profiles), combinator);
}

std::vector<EnhancedChargingSchedulePeriod>
convert_intermediate_into_schedule(const IntermediateProfile& profile, ChargingRateUnit charging_rate_unit,
                                   float default_limit, int32_t default_number_phases, float supply_voltage) {

    std::vector<EnhancedChargingSchedulePeriod> output{};

    for (const auto& period : profile) {
        EnhancedChargingSchedulePeriod period_out{};
        period_out.startPeriod = period.startPeriod;
        period_out.numberPhases = period.numberPhases;

        if (period.current_limit == NO_LIMIT_SPECIFIED && period.power_limit == NO_LIMIT_SPECIFIED) {
            period_out.limit = default_limit;
        } else {
            float transform_value = supply_voltage * period_out.numberPhases.value_or(default_number_phases);
            period_out.limit = std::numeric_limits<float>::max();
            if (charging_rate_unit == ChargingRateUnit::A) {
                if (period.current_limit != NO_LIMIT_SPECIFIED) {
                    period_out.limit = period.current_limit;
                    period_out.stackLevel = period.stack_level_current;
                    period_out.periodTransformed = false;
                }
                if (period.power_limit != NO_LIMIT_SPECIFIED) {
                    const auto converted_power_limit = period.power_limit / transform_value;
                    if (converted_power_limit < period_out.limit) {
                        period_out.limit = converted_power_limit;
                        period_out.stackLevel = period.stack_level_power;
                        period_out.periodTransformed = true;
                    }
                }
            } else {
                if (period.power_limit != NO_LIMIT_SPECIFIED) {
                    period_out.limit = period.power_limit;
                    period_out.stackLevel = period.stack_level_power;
                    period_out.periodTransformed = false;
                }
                if (period.current_limit != NO_LIMIT_SPECIFIED) {
                    const auto converted_current_limit = period.current_limit * transform_value;
                    if (converted_current_limit < period_out.limit) {
                        period_out.limit = converted_current_limit;
                        period_out.stackLevel = period.stack_level_current;
                        period_out.periodTransformed = true;
                    }
                }
            }
        }

        if (output.empty() || (period_out.limit != output.back().limit) ||
            (period_out.numberPhases != output.back().numberPhases) ||
            period_out.stackLevel != output.back().stackLevel) {
            output.push_back(period_out);
        }
    }

    return output;
}

} // namespace ocpp::v16
