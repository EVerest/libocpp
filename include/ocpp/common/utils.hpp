// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_UTILS_HPP
#define OCPP_COMMON_UTILS_HPP

#include <string>
#include <vector>

namespace ocpp {

/// \brief Case insensitive compare for a case insensitive (Ci)String
bool iequals(const std::string& lhs, const std::string rhs);

std::vector<std::string> get_vector_from_csv(const std::string& csv_str);

///
/// \brief Check if the given string is an integer.
/// \param value    The value to check.
/// \return True when value is an integer.
///
bool isInteger(const std::string& value);

///
/// \brief Check if the given string is a decimal.
/// \param value    The value to check.
/// \return True when value is a decimal number.
///
bool isDecimal(const std::string& value);

} // namespace ocpp

#endif
