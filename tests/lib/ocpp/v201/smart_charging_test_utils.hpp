// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "everest/logging.hpp"
#include "ocpp/v201/ocpp_types.hpp"
#include "ocpp/v201/profile.hpp"
#include "ocpp/v201/utils.hpp"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <sstream>
#include <vector>

namespace ocpp::v201 {

static const std::string BASE_JSON_PATH = std::string(TEST_PROFILES_LOCATION_V201) + "/json";

static ocpp::DateTime dt(const std::string& dt_string) {
    ocpp::DateTime dt;

    if (dt_string.length() == 4) {
        dt = ocpp::DateTime("2024-01-01T0" + dt_string + ":00Z");
    } else if (dt_string.length() == 5) {
        dt = ocpp::DateTime("2024-01-01T" + dt_string + ":00Z");
    } else if (dt_string.length() == 7) {
        dt = ocpp::DateTime("2024-01-0" + dt_string + ":00Z");
    } else if (dt_string.length() == 8) {
        dt = ocpp::DateTime("2024-01-" + dt_string + ":00Z");
    } else if (dt_string.length() == 11) {
        dt = ocpp::DateTime("2024-" + dt_string + ":00Z");
    } else if (dt_string.length() == 16) {
        dt = ocpp::DateTime(dt_string + ":00Z");
    }

    return dt;
}

class SmartChargingTestUtils {
public:
    static std::vector<ChargingProfile> get_charging_profiles_from_directory(const std::string& path) {
        EVLOG_debug << "get_charging_profiles_from_directory: " << path;
        std::vector<ChargingProfile> profiles;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_directory()) {
                fs::path path = entry.path();
                if (path.extension() == ".json") {
                    ChargingProfile profile = get_charging_profile_from_path(path);
                    std::cout << path << std::endl;
                    profiles.push_back(profile);
                }
            }
        }

        // Sort profiles by id in ascending order
        std::sort(profiles.begin(), profiles.end(),
                  [](const ChargingProfile& a, const ChargingProfile& b) { return a.id < b.id; });

        EVLOG_debug << "get_charging_profiles_from_directory END";
        return profiles;
    }

    static ChargingProfile get_charging_profile_from_path(const std::string& path) {
        EVLOG_debug << "get_charging_profile_from_path: " << path;
        std::ifstream f(path.c_str());
        json data = json::parse(f);

        ChargingProfile cp;
        from_json(data, cp);
        return cp;
    }

    static ChargingProfile get_charging_profile_from_file(const std::string& filename) {
        const std::string full_path = BASE_JSON_PATH + "/" + filename;

        return get_charging_profile_from_path(full_path);
    }

    static std::vector<ChargingProfile> get_charging_profiles_from_file(const std::string& filename) {
        std::vector<ChargingProfile> profiles;
        profiles.push_back(get_charging_profile_from_file(filename));
        return profiles;
    }

    /// \brief Returns a vector of ChargingProfiles to be used as a baseline for testing core functionality
    /// of generating an EnhancedChargingSchedule.
    static std::vector<ChargingProfile> get_baseline_profile_vector() {
        return get_charging_profiles_from_directory(BASE_JSON_PATH + "/" + "baseline/");
    }

    static std::string to_string(std::vector<ChargingProfile>& profiles) {
        std::string s;
        json cp_json;
        for (auto& profile : profiles) {
            if (!s.empty())
                s += ", ";
            to_json(cp_json, profile);
            s += cp_json.dump(4);
        }

        return "[" + s + "]";
    }

    /// \brief Validates that there is no overlap in the submitted period_entry_t collection
    /// \param period_entry_t collection
    /// \note If there are any overlapping period_entry_t entries the function returns false
    static bool validate_profile_result(const std::vector<period_entry_t>& result) {
        bool bRes{true};
        DateTime last{"1900-01-01T00:00:00Z"};
        for (const auto& i : result) {
            // ensure no overlaps
            bRes = i.start < i.end;
            bRes = bRes && i.start >= last;
            last = i.end;
            if (!bRes) {
                break;
            }
        }
        return bRes;
    }
};

} // namespace ocpp::v201