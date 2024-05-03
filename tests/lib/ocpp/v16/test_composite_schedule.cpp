// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>
namespace fs = std::filesystem;

#include "everest/logging.hpp"
#include <database_handler_mock.hpp>
#include <evse_security_mock.hpp>
#include <ocpp/common/call_types.hpp>
#include <ocpp/common/evse_security_impl.hpp>
#include <ocpp/v16/charge_point_impl.hpp>
#include <ocpp/v16/enums.hpp>
#include <ocpp/v16/smart_charging.hpp>
#include <optional>

namespace ocpp {
namespace v16 {

/**
 * CompositeSchedule Test Fixture
 */
class CompositeScheduleTestFixture : public testing::Test {
protected:
    void SetUp() override {
        this->evse_security = std::make_shared<EvseSecurityMock>();
    }

    void addConnector(int id) {
        auto connector = Connector{id};

        auto timer = std::unique_ptr<Everest::SteadyTimer>();

        connector.transaction =
            std::make_shared<Transaction>(-1, id, "test", "test", 1, std::nullopt, ocpp::DateTime(), std::move(timer));
        connectors[id] = std::make_shared<Connector>(connector);
    }

    SmartChargingHandler* createSmartChargingHandler(const int number_of_connectors) {
        for (int i = 0; i <= number_of_connectors; i++) {
            addConnector(i);
        }

        const std::string chargepoint_id = "1";
        const fs::path database_path = "na";
        const fs::path init_script_path = "na";

        auto database = std::make_unique<common::DatabaseConnection>(database_path / (chargepoint_id + ".db"));
        std::shared_ptr<DatabaseHandlerMock> database_handler =
            std::make_shared<DatabaseHandlerMock>(std::move(database), init_script_path);

        auto handler = new SmartChargingHandler(connectors, database_handler, true);

        return handler;
    }

    ChargingProfile getChargingProfileFromFile(const std::string& filename) {
        const std::string base_path = "/tmp/EVerest/libocpp/json/";
        const std::string full_path = base_path + filename;

        std::ifstream f(full_path.c_str());
        json data = json::parse(f);

        ChargingProfile cp;
        from_json(data, cp);
        return cp;
    }

    void log_duration(int32_t duration) {
        int32_t remaining = duration;

        std::string log_str = "    Duration> ";

        if (remaining >= 86400) {
            int32_t days = remaining / 86400;
            remaining = remaining % 86400;
            if (days > 1) {
                log_str += std::to_string(days) + " Days ";
            } else {
                log_str += std::to_string(days) + " Day ";
            }
        }
        if (remaining >= 3600) {
            int32_t hours = remaining / 3600;
            remaining = remaining % 3600;
            log_str += std::to_string(hours) + " Hours ";
        }
        if (remaining >= 60) {
            int32_t minutes = remaining / 60;
            remaining = remaining % 60;
            log_str += std::to_string(minutes) + " Minutes ";
        }
        if (remaining > 0) {
            log_str += std::to_string(remaining) + " Seconds ";
        }
        EVLOG_info << log_str;
    }

    void log_me(ChargingProfile& cp) {
        json cp_json;
        to_json(cp_json, cp);

        EVLOG_info << "  ChargingProfile> " << cp_json.dump(4);
        log_duration(cp.chargingSchedule.duration.value_or(0));
    }

    void log_me(std::vector<ChargingProfile> profiles) {
        EVLOG_info << "[";
        for (auto& profile : profiles) {
            log_me(profile);
        }
        EVLOG_info << "]";
    }

    void log_me(EnhancedChargingSchedule& ecs) {
        json ecs_json;
        to_json(ecs_json, ecs);

        EVLOG_info << "EnhancedChargingSchedule> " << ecs_json.dump(4);
    }

    /// \brief Returns a vector of ChargingProfiles to be used as a baseline for testing core functionality
    /// of generating an EnhancedChargingSchedule.
    std::vector<ChargingProfile> getBaselineProfileVector() {
        auto profile_01 = getChargingProfileFromFile("TxDefaultProfile_01.json");
        auto profile_100 = getChargingProfileFromFile("TxDefaultProfile_100.json");
        return {profile_01, profile_100};
    }

    // Default values used within the tests
    std::map<int32_t, std::shared_ptr<Connector>> connectors;
    std::shared_ptr<DatabaseHandler> database_handler;
    std::shared_ptr<EvseSecurityMock> evse_security;
};

TEST_F(CompositeScheduleTestFixture, CalculateEnhancedCompositeSchedule_ValidatedBaseline) {
    auto handler = createSmartChargingHandler(1);

    std::vector<ChargingProfile> profiles = getBaselineProfileVector();
    log_me(profiles);

    const DateTime my_date_start_range = ocpp::DateTime("2024-01-17T18:01:00");
    const DateTime my_date_end_range = ocpp::DateTime("2024-01-18T00:00:00");

    EVLOG_info << "    Start> " << my_date_start_range.to_rfc3339();
    EVLOG_info << "      End> " << my_date_end_range.to_rfc3339();

    auto composite_schedule = handler->calculate_enhanced_composite_schedule(
        profiles, my_date_start_range, my_date_end_range, 1, profiles.at(0).chargingSchedule.chargingRateUnit);

    log_me(composite_schedule);
    ASSERT_EQ(composite_schedule.chargingRateUnit, ChargingRateUnit::W);
    ASSERT_EQ(composite_schedule.duration, 21540);
    ASSERT_EQ(profiles.size(), 2);
    ASSERT_EQ(composite_schedule.chargingSchedulePeriod.size(), 2);
    auto& period_01 = composite_schedule.chargingSchedulePeriod.at(0);
    ASSERT_EQ(period_01.limit, 2000);
    ASSERT_EQ(period_01.numberPhases, 1);
    ASSERT_EQ(period_01.stackLevel, 1);
    ASSERT_EQ(period_01.startPeriod, 0);
    auto& period_02 = composite_schedule.chargingSchedulePeriod.at(1);
    ASSERT_EQ(period_02.limit, 11000);
    ASSERT_EQ(period_02.numberPhases, 3);
    ASSERT_EQ(period_02.stackLevel, 0);
    ASSERT_EQ(period_02.startPeriod, 1020);
}

TEST_F(CompositeScheduleTestFixture, CalculateEnhancedCompositeSchedule_TxProfile) {
    GTEST_SKIP();
    auto handler = createSmartChargingHandler(1);

    ChargingProfile profile_01 = getChargingProfileFromFile("TxDefaultProfile_01.json");
    ChargingProfile txprofile_02 = getChargingProfileFromFile("TxProfile_02.json");
    ChargingProfile profile_100 = getChargingProfileFromFile("TxDefaultProfile_100.json");

    std::vector<ChargingProfile> profiles = {profile_01, txprofile_02, profile_100};
    log_me(profiles);

    const DateTime my_date_start_range = ocpp::DateTime("2024-01-17T18:01:00");
    const DateTime my_date_end_range = ocpp::DateTime("2024-01-18T00:00:00");

    EVLOG_info << "    Start> " << my_date_start_range.to_rfc3339();
    EVLOG_info << "      End> " << my_date_end_range.to_rfc3339();

    auto composite_schedule = handler->calculate_enhanced_composite_schedule(
        profiles, my_date_start_range, my_date_end_range, 1, profiles.at(0).chargingSchedule.chargingRateUnit);
}

} // namespace v16
} // namespace ocpp
