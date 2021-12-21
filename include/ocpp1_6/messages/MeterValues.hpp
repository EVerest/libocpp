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
#ifndef OCPP1_6_METERVALUES_HPP
#define OCPP1_6_METERVALUES_HPP

#include <ocpp1_6/ocpp_types.hpp>

namespace ocpp1_6 {
struct MeterValuesRequest : public Message {
    int32_t connectorId;
    std::vector<MeterValue> meterValue;
    boost::optional<int32_t> transactionId;

    std::string get_type() const {
        return "MeterValues";
    }

    friend void to_json(json& j, const MeterValuesRequest& k) {
        // the required parts of the message
        j = json{
            {"connectorId", k.connectorId},
            {"meterValue", k.meterValue},
        };
        // the optional parts of the message
        if (k.transactionId) {
            j["transactionId"] = k.transactionId.value();
        }
    }

    friend void from_json(const json& j, MeterValuesRequest& k) {
        // the required parts of the message
        k.connectorId = j.at("connectorId");
        for (auto val : j.at("meterValue")) {
            k.meterValue.push_back(val);
        }

        // the optional parts of the message
        if (j.contains("transactionId")) {
            k.transactionId.emplace(j.at("transactionId"));
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const MeterValuesRequest& k) {
        os << json(k).dump(4);
        return os;
    }
};

struct MeterValuesResponse : public Message {

    std::string get_type() const {
        return "MeterValuesResponse";
    }

    friend void to_json(json& j, const MeterValuesResponse& k) {
        // the required parts of the message
        j = json({});
        // the optional parts of the message
    }

    friend void from_json(const json& j, MeterValuesResponse& k) {
        // the required parts of the message

        // the optional parts of the message
    }

    friend std::ostream& operator<<(std::ostream& os, const MeterValuesResponse& k) {
        os << json(k).dump(4);
        return os;
    }
};

} // namespace ocpp1_6

#endif // OCPP1_6_METERVALUES_HPP
