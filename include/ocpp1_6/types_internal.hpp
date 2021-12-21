/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef OCPP1_6_TYPES_INTERNAL_HPP
#define OCPP1_6_TYPES_INTERNAL_HPP

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>

namespace ocpp1_6 {
// Ci = Case Insensitive and only printable ASCII characters are allowed
class CiString {
private:
    std::string data;
    size_t length;

public:
    CiString(const std::string& data, size_t length);
    CiString(size_t length);
    CiString() = delete;
    std::string get() const;
    void set(const std::string& data);
    ///
    /// \brief case insensitive compare for a case insensitive (Ci)String
    ///
    bool operator==(const char* c) const {
        return boost::iequals(this->data, c);
    }
    ///
    /// \brief case insensitive compare for a case insensitive (Ci)String
    ///
    bool operator==(const CiString& s) {
        return boost::iequals(this->data, s.get());
    }
    bool operator!=(const char* c) const {
        return !(this->data == c);
    }
    bool operator!=(const CiString& s) {
        return !(this->data == s.get());
    }
    friend std::ostream& operator<<(std::ostream& os, const CiString& str);
    operator std::string() {
        return this->data;
    }
};

class DateTimeImpl {
private:
    std::chrono::time_point<std::chrono::system_clock> timepoint;

public:
    DateTimeImpl();
    DateTimeImpl(std::chrono::time_point<std::chrono::system_clock> timepoint);
    DateTimeImpl(const std::string& timepoint_str);
    std::string to_rfc3339() const;
    std::chrono::time_point<std::chrono::system_clock> to_time_point();
    friend std::ostream& operator<<(std::ostream& os, const DateTimeImpl& dt);
    operator std::string() {
        return this->to_rfc3339();
    }
    friend bool operator>(const DateTimeImpl& lhs, const DateTimeImpl& rhs) {
        return lhs.to_rfc3339() > rhs.to_rfc3339();
    }
    friend bool operator>=(const DateTimeImpl& lhs, const DateTimeImpl& rhs) {
        return lhs.to_rfc3339() >= rhs.to_rfc3339();
    }
    friend bool operator<(const DateTimeImpl& lhs, const DateTimeImpl& rhs) {
        return lhs.to_rfc3339() < rhs.to_rfc3339();
    }
    friend bool operator<=(const DateTimeImpl& lhs, const DateTimeImpl& rhs) {
        return lhs.to_rfc3339() <= rhs.to_rfc3339();
    }
    friend bool operator==(const DateTimeImpl& lhs, const DateTimeImpl& rhs) {
        return lhs.to_rfc3339() == rhs.to_rfc3339();
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_TYPES_INTERNAL_HPP
