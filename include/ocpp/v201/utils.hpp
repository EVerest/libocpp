// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef V201_UTILS_HPP
#define V201_UTILS_HPP

#include <openssl/sha.h>

#include <ocpp/v201/ocpp_types.hpp>
namespace ocpp {
namespace v201 {
namespace utils {

/// \brief This function returns the configured Measurand as an std::vector
/// \brief std::vector<MeasurandEnum> of the configured AlignedDataMeasurands
std::vector<MeasurandEnum> get_measurands_vec(const std::string& measurands_csv);

/// \brief This function determines if any of the \p measurands is present in the \p _meter_value at all
/// \return True if any measurand is found, false otherwise
bool meter_value_has_any_measurand(const MeterValue& _meter_value, const std::vector<MeasurandEnum>& measurands);

/// \brief Applies the given \p measurands to the given \p _meter_value . The returned meter value will only contain
/// SampledValues which measurand is listed in the given \param measurands . If no measurand is set for the
/// SampledValue, the SampledValue will also be omitted.
/// \param _meter_value the meter value to be filtered
/// \param measurands applied measurands
/// \return filtered meter value
MeterValue get_meter_value_with_measurands_applied(const MeterValue& _meter_value,
                                                   const std::vector<MeasurandEnum>& measurands);

/// \brief Applies the given measurands to \p meter_values based on their ReadingContext.
/// Transaction_Begin, Interruption_Begin, Transaction_End, Interruption_End and Sample_Periodic will be filtered using
/// \p sampled_tx_ended_measurands.
/// Sample_Clock will be filtered using \p aligned_tx_ended_measurands
/// Any metervalue after \p max_timestamp will also be removed.
/// \retval filtered meter values
std::vector<MeterValue> get_meter_values_with_measurands_applied(
    const std::vector<MeterValue>& meter_values, const std::vector<MeasurandEnum>& sampled_tx_ended_measurands,
    const std::vector<MeasurandEnum>& aligned_tx_ended_measurands, ocpp::DateTime max_timestamp);

/// \brief Converts the given \p stop_reason to a TriggerReasonEnum
/// \param stop_reason
/// \return
TriggerReasonEnum stop_reason_to_trigger_reason_enum(const ReasonEnum& stop_reason);

/// \brief Returns the given \p str hashed using SHA256
/// \param str
/// \return
std::string sha256(const std::string& str);

/// \brief Return a SHA256 hash generated from a combination of the \p token type and id
/// \param token the token to generate the hash for
/// \return A SHA256 hash string
std::string generate_token_hash(const IdToken& token);

/// \brief Align the clock aligned timestamps to the interval values
/// \param timestamp the timestamp to align
/// \param align_interval the clock aligned interval to align to since midnight 00:00
/// \return DateTime type timestamp
ocpp::DateTime align_timestamp(const DateTime timestamp, std::chrono::seconds align_interval);

} // namespace utils
} // namespace v201
} // namespace ocpp

#endif
