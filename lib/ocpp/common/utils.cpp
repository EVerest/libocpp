// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <boost/algorithm/string/predicate.hpp>
#include <regex>
#include <sstream>

#include <ocpp/common/utils.hpp>

namespace ocpp {

bool iequals(const std::string& lhs, const std::string rhs) {
    return boost::algorithm::iequals(lhs, rhs);
}

std::vector<std::string> get_vector_from_csv(const std::string& csv_str) {
    std::vector<std::string> csv;
    std::string str;
    std::stringstream ss(csv_str);
    while (std::getline(ss, str, ',')) {
        csv.push_back(str);
    }
    return csv;
}

bool isInteger(const std::string& value) {
    if (value.empty() || ((!isdigit(value[0])) && (value[0] != '-') && (value[0] != '+'))) {
        return false;
    }

    char* p;
    strtol(value.c_str(), &p, 10);

    return (*p == 0);
}

bool isDecimal(const std::string& value) {
    const std::regex decimalRegex("^[-+]?([0-9]*\\.[0-9]+|[0-9]+)$");
    return std::regex_match(value, decimalRegex);
}

} // namespace ocpp
