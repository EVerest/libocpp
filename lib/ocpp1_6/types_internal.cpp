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
#include <iostream>
#include <ocpp1_6/types_internal.hpp>
#include <sstream>
#include <stdexcept>

#include <date/date.h>
#include <everest/logging.hpp>

namespace ocpp1_6 {
CiString::CiString(const std::string& data, size_t length) : length(length) {
    this->set(data);
}

CiString::CiString(size_t length) : length(length) {
}

std::string CiString::get() const {
    return this->data;
}

void CiString::set(const std::string& data) {
    if (data.length() <= this->length) {
        for (const char& character : data) {
            // printable ASCII starts at code 0x20 (space) and ends with code 0x7e (tilde)
            if (character < 0x20 || character > 0x7e) {
                throw std::runtime_error("CiString can only contain printable ASCII characters");
            }
        }
        this->data = data;
    } else {
        throw std::runtime_error("CiString length (" + std::to_string(data.length()) + ") exceeds permitted length (" +
                                 std::to_string(this->length) + ")");
    }
}

std::ostream& operator<<(std::ostream& os, const CiString& str) {
    os << str.get();
    return os;
}

DateTimeImpl::DateTimeImpl() {
    this->timepoint = std::chrono::system_clock::now();
}

DateTimeImpl::DateTimeImpl(std::chrono::time_point<std::chrono::system_clock> timepoint) : timepoint(timepoint) {
}

DateTimeImpl::DateTimeImpl(const std::string& timepoint_str) {
    std::istringstream in{timepoint_str};
    in >> date::parse("%FT%T%Ez", this->timepoint);
    if (in.fail()) {
        in.clear();
        in.seekg(0);
        in >> date::parse("%FT%TZ", this->timepoint);
        if (in.fail()) {
            EVLOG(error) << "timepoint string parsing failed";
        }
    }
}

std::string DateTimeImpl::to_rfc3339() const {
    return date::format("%FT%TZ", std::chrono::time_point_cast<std::chrono::milliseconds>(this->timepoint));
}

std::chrono::time_point<std::chrono::system_clock> DateTimeImpl::to_time_point() {
    return this->timepoint;
}

std::ostream& operator<<(std::ostream& os, const DateTimeImpl& dt) {
    os << dt.to_rfc3339();
    return os;
}

} // namespace ocpp1_6
