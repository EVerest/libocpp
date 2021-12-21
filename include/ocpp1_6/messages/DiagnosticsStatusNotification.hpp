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
#ifndef OCPP1_6_DIAGNOSTICSSTATUSNOTIFICATION_HPP
#define OCPP1_6_DIAGNOSTICSSTATUSNOTIFICATION_HPP

#include <ocpp1_6/ocpp_types.hpp>

namespace ocpp1_6 {
struct DiagnosticsStatusNotificationRequest : public Message {
    DiagnosticsStatus status;

    std::string get_type() const {
        return "DiagnosticsStatusNotification";
    }

    friend void to_json(json& j, const DiagnosticsStatusNotificationRequest& k) {
        // the required parts of the message
        j = json{
            {"status", conversions::diagnostics_status_to_string(k.status)},
        };
        // the optional parts of the message
    }

    friend void from_json(const json& j, DiagnosticsStatusNotificationRequest& k) {
        // the required parts of the message
        k.status = conversions::string_to_diagnostics_status(j.at("status"));

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const DiagnosticsStatusNotificationRequest& k) {
        os << json(k).dump(4);
        return os;
    }
};

struct DiagnosticsStatusNotificationResponse : public Message {

    std::string get_type() const {
        return "DiagnosticsStatusNotificationResponse";
    }

    friend void to_json(json& j, const DiagnosticsStatusNotificationResponse& k) {
        // the required parts of the message
        j = json({});
        // the optional parts of the message
    }

    friend void from_json(const json& j, DiagnosticsStatusNotificationResponse& k) {
        // the required parts of the message

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const DiagnosticsStatusNotificationResponse& k) {
        os << json(k).dump(4);
        return os;
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_DIAGNOSTICSSTATUSNOTIFICATION_HPP
