// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

// execute: ./libocpp_unit_tests --gtest_filter=ProfileTests.*

#include "ocpp/common/database/sqlite_statement.hpp"
#include "ocpp/v16/ocpp_types.hpp"
#include "ocpp/v16/types.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <ocpp/v16/connector.hpp>
#include <ocpp/v16/database_handler.hpp>
#include <ocpp/v16/smart_charging.hpp>
#include <optional>
#include <ostream>
#include <string>

using namespace ocpp::v16;
using namespace ocpp;
namespace fs = std::filesystem;
using json = nlohmann::json;

// ----------------------------------------------------------------------------
// helper functions
namespace ocpp::v16 {

template <typename A> bool optional_equal(const std::optional<A>& a, const std::optional<A>& b) {
    bool bRes = true;
    if (bRes && a.has_value() && b.has_value()) {
        bRes = a.value() == b.value();
    }
    return bRes;
}

std::ostream& operator<<(std::ostream& os, const std::vector<ChargingProfile>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            os << "[" << count++ << "] " << i;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<ChargingSchedulePeriod>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            json j;
            to_json(j, i);
            os << "[" << count++ << "] " << j << std::endl;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<EnhancedChargingSchedulePeriod>& profiles) {
    if (profiles.size() > 0) {
        std::uint32_t count = 0;
        for (const auto& i : profiles) {
            json j;
            to_json(j, i);
            os << "[" << count++ << "] " << j << std::endl;
        }
    } else {
        os << "<no profiles>";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const EnhancedChargingSchedule& schedule) {
    json j;
    to_json(j, schedule);
    os << j;
    return os;
}

bool operator==(const ChargingSchedulePeriod& a, const ChargingSchedulePeriod& b) {
    auto diff = std::abs(a.startPeriod - b.startPeriod);
    bool bRes = diff < 10; // allow for a small difference
    bRes = bRes && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    return bRes;
}

bool operator==(const ChargingSchedule& a, const ChargingSchedule& b) {
    bool bRes = true;
    auto min = std::min(a.chargingSchedulePeriod.size(), b.chargingSchedulePeriod.size());
    EXPECT_GT(min, 0);
    for (std::uint32_t i = 0; bRes && i < min; i++) {
        SCOPED_TRACE(std::string("i=") + std::to_string(i));
        bRes = bRes && a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
        EXPECT_EQ(a.chargingSchedulePeriod[i], b.chargingSchedulePeriod[i]);
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    EXPECT_EQ(a.chargingRateUnit, b.chargingRateUnit);
    if (a.minChargingRate.has_value() && b.minChargingRate.has_value()) {
        EXPECT_EQ(a.minChargingRate.value(), b.minChargingRate.value());
    }
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    if (a.startSchedule.has_value() && b.startSchedule.has_value()) {
        EXPECT_EQ(a.startSchedule.value(), b.startSchedule.value());
    }
    if (a.duration.has_value() && b.duration.has_value()) {
        EXPECT_EQ(a.duration.value(), b.duration.value());
    }
    return bRes;
}

inline bool operator!=(const ChargingSchedule& a, const ChargingSchedule& b) {
    return !(a == b);
}

bool operator==(const ChargingSchedulePeriod& a, const EnhancedChargingSchedulePeriod& b) {
    auto diff = std::abs(a.startPeriod - b.startPeriod);
    bool bRes = diff < 10; // allow for a small difference
    bRes = bRes && (a.limit == b.limit);
    bRes = bRes && optional_equal(a.numberPhases, b.numberPhases);
    // b.stackLevel ignored
    return bRes;
}

bool operator==(const ChargingSchedule& a, const EnhancedChargingSchedule& b) {
    bool bRes = true;
    auto min = std::min(a.chargingSchedulePeriod.size(), b.chargingSchedulePeriod.size());
    EXPECT_GT(min, 0);
    for (std::uint32_t i = 0; bRes && i < min; i++) {
        SCOPED_TRACE(std::string("i=") + std::to_string(i));
        bRes = bRes && a.chargingSchedulePeriod[i] == b.chargingSchedulePeriod[i];
        EXPECT_EQ(a.chargingSchedulePeriod[i], b.chargingSchedulePeriod[i]);
    }
    bRes = bRes && (a.chargingRateUnit == b.chargingRateUnit) && optional_equal(a.minChargingRate, b.minChargingRate);
    EXPECT_EQ(a.chargingRateUnit, b.chargingRateUnit);
    if (a.minChargingRate.has_value() && b.minChargingRate.has_value()) {
        EXPECT_EQ(a.minChargingRate.value(), b.minChargingRate.value());
    }
    bRes = bRes && optional_equal(a.startSchedule, b.startSchedule) && optional_equal(a.duration, b.duration);
    if (a.startSchedule.has_value() && b.startSchedule.has_value()) {
        EXPECT_EQ(a.startSchedule.value(), b.startSchedule.value());
    }
    if (a.duration.has_value() && b.duration.has_value()) {
        EXPECT_EQ(a.duration.value(), b.duration.value());
    }
    return bRes;
}

bool operator==(const ChargingProfile& a, const ChargingProfile& b) {
    bool bRes = (a.chargingProfileId == b.chargingProfileId) && (a.stackLevel == b.stackLevel) &&
                (a.chargingProfilePurpose == b.chargingProfilePurpose) &&
                (a.chargingProfileKind == b.chargingProfileKind) && (a.chargingSchedule == b.chargingSchedule);
    bRes = bRes && optional_equal(a.transactionId, b.transactionId) &&
           optional_equal(a.recurrencyKind, b.recurrencyKind) && optional_equal(a.validFrom, b.validFrom) &&
           optional_equal(a.validTo, b.validTo);
    return bRes;
}

} // namespace ocpp::v16

// ----------------------------------------------------------------------------
// Test anonymous namespace
namespace {
using namespace std::chrono;
using ocpp::common::SQLiteString;

// ----------------------------------------------------------------------------
// Test charging profiles

const auto now = date::utc_clock::now();
const ocpp::DateTime profileA_start_time(now - seconds(600));
const ocpp::DateTime profileA_end_time(now + hours(2));
const ChargingProfile profileA{
    301,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Absolute,            // chargingProfileKind
    {
        // ChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // ChargingSchedulePeriod
                0,            // startPeriod
                32.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                6000,         // startPeriod
                24.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                12000,        // startPeriod
                21.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,        // optional - int32_t duration
        profileA_start_time, // optional - ocpp::DateTime - startSchedule
        std::nullopt,        // optional - float - minChargingRate
    },                       // chargingSchedule
    std::nullopt,            // transactionId
    std::nullopt,            // recurrencyKind
    profileA_start_time,     // validFrom
    profileA_end_time,       // validTo
};

ocpp::DateTime profileB_start_time(now);
ocpp::DateTime profileB_end_time(now + hours(4));
ChargingProfile profileB{
    302,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Absolute,            // chargingProfileKind
    {
        // ChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // ChargingSchedulePeriod
                0,            // startPeriod
                10.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
            {
                // ChargingSchedulePeriod
                7000,         // startPeriod
                20.0,         // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,        // optional - int32_t duration
        profileB_start_time, // optional - ocpp::DateTime - startSchedule
        std::nullopt,        // optional - float - minChargingRate
    },                       // chargingSchedule
    std::nullopt,            // transactionId
    std::nullopt,            // recurrencyKind
    profileB_start_time,     // validFrom
    profileB_end_time,       // validTo
};

ocpp::DateTime profileNoCharge_start_time(now - seconds(300));
ocpp::DateTime profileNoCharge_end_time(now + hours(300));
ChargingProfile profileNoCharge{
    302,                                          // chargingProfileId
    5,                                            // stackLevel
    ChargingProfilePurposeType::TxDefaultProfile, // chargingProfilePurpose
    ChargingProfileKindType::Relative,            // chargingProfileKind
    {
        // ChargingSchedule
        ChargingRateUnit::A, // chargingRateUnit
        {
            {
                // ChargingSchedulePeriod
                0,            // startPeriod
                0.0,          // limit
                std::nullopt, // optional - int32_t - numberPhases
            },
        },
        std::nullopt,           // optional - int32_t duration
        std::nullopt,           // optional - ocpp::DateTime - startSchedule
        std::nullopt,           // optional - float - minChargingRate
    },                          // chargingSchedule
    std::nullopt,               // transactionId
    std::nullopt,               // recurrencyKind
    profileNoCharge_start_time, // validFrom
    profileNoCharge_end_time,   // validTo
};

// ----------------------------------------------------------------------------
// provide access to the SQLite database handle
struct DatabaseHandlerTest : public DatabaseHandler {
    using DatabaseHandler::DatabaseHandler;
};

struct SQLiteStatementTest : public ocpp::common::SQLiteStatementInterface {
    virtual int step() {
        return SQLITE_DONE;
    }
    virtual int reset() {
        return 0;
    }
    virtual int bind_text(const int idx, const std::string& val, SQLiteString lifetime = SQLiteString::Static) {
        return 0;
    }
    virtual int bind_text(const std::string& param, const std::string& val,
                          SQLiteString lifetime = SQLiteString::Static) {
        return 0;
    }
    virtual int bind_int(const int idx, const int val) {
        return 0;
    }
    virtual int bind_int(const std::string& param, const int val) {
        return 0;
    }
    virtual int bind_datetime(const int idx, const ocpp::DateTime val) {
        return 0;
    }
    virtual int bind_datetime(const std::string& param, const ocpp::DateTime val) {
        return 0;
    }
    virtual int bind_double(const int idx, const double val) {
        return 0;
    }
    virtual int bind_double(const std::string& param, const double val) {
        return 0;
    }
    virtual int bind_null(const int idx) {
        return 0;
    }
    virtual int bind_null(const std::string& param) {
        return 0;
    }
    virtual int column_type(const int idx) {
        return SQLITE_INTEGER;
    }
    virtual std::string column_text(const int idx) {
        return std::string();
    }
    virtual std::optional<std::string> column_text_nullable(const int idx) {
        return std::nullopt;
    }
    virtual int column_int(const int idx) {
        return 0;
    }
    virtual ocpp::DateTime column_datetime(const int idx) {
        return ocpp::DateTime();
    }
    virtual double column_double(const int idx) {
        return 0.0;
    }
};

struct DatabaseConnectionTest : public common::DatabaseConnectionInterface {
    virtual bool open_connection() {
        return true;
    }
    virtual bool close_connection() {
        return true;
    }
    virtual std::unique_ptr<ocpp::common::DatabaseTransactionInterface> begin_transaction() {
        return std::unique_ptr<ocpp::common::DatabaseTransactionInterface>{};
    }
    virtual bool commit_transaction() {
        return true;
    }
    virtual bool rollback_transaction() {
        return true;
    }
    virtual bool execute_statement(const std::string& statement) {
        return true;
    }
    virtual std::unique_ptr<ocpp::common::SQLiteStatementInterface> new_statement(const std::string& sql) {
        return std::make_unique<SQLiteStatementTest>();
    }
    virtual const char* get_error_message() {
        return "";
    }
    virtual bool clear_table(const std::string& table) {
        return true;
    }
    virtual int64_t get_last_inserted_rowid() {
        return 1;
    }
};

// ----------------------------------------------------------------------------
// Test class
class ProfileTests : public testing::Test {
protected:
    const std::string chargepoint_id = "12345678";
    const fs::path database_path = "/tmp/";
    const fs::path init_script_path = "./core_migrations";
    const fs::path db_filename = database_path / (chargepoint_id + ".db");

    std::map<int32_t, std::shared_ptr<Connector>> connectors;
    std::shared_ptr<DatabaseHandlerTest> database_handler;
    std::unique_ptr<ocpp::common::DatabaseConnectionInterface> database_interface;

    void add_connectors(unsigned int n) {
        for (unsigned int i = 1; i <= n; i++) {
            connectors[i] = std::make_shared<Connector>(i);
        }
    }

    void SetUp() override {
        database_interface = std::make_unique<DatabaseConnectionTest>();
        database_handler = std::make_shared<DatabaseHandlerTest>(std::move(database_interface), init_script_path, 1);
    }

    void TearDown() override {
        std::filesystem::remove(db_filename);
        connectors.clear();
    }
};

// ----------------------------------------------------------------------------
// Test cases
TEST_F(ProfileTests, init) {
    add_connectors(2);
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);
    ChargingProfile profile{
        101,                                                                 // chargingProfileId
        20,                                                                  // stackLevel
        ChargingProfilePurposeType::TxDefaultProfile,                        // chargingProfilePurpose
        ChargingProfileKindType::Relative,                                   // chargingProfileKind
        {ChargingRateUnit::A, {}, std::nullopt, std::nullopt, std::nullopt}, // chargingSchedule
        std::nullopt,                                                        // transactionId
        std::nullopt,                                                        // recurrencyKind
        std::nullopt,                                                        // validFrom
        std::nullopt,                                                        // validTo
    };
    handler.add_tx_default_profile(profile, 1);
    handler.clear_all_profiles();
}

TEST_F(ProfileTests, validate_profileA) {
    // need to have a transaction for calculate_composite_schedule()
    // and calculate_enhanced_composite_schedule()
    int connector_id = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp(now);
    add_connectors(5);
    connectors[connector_id]->transaction =
        std::make_shared<Transaction>(-1, connector_id, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);
    auto tmp_profile = profileA;
    EXPECT_TRUE(
        handler.validate_profile(tmp_profile, 0, true, 100, 10, 10, {ChargingRateUnit::A, ChargingRateUnit::W}));
    // check profile not updated
    EXPECT_EQ(tmp_profile, profileA);
    handler.add_tx_default_profile(tmp_profile, connector_id);
    auto valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, connector_id);
    auto schedule = handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time,
                                                         connector_id, std::nullopt);
    // std::cout << "chargingSchedule:\n" << profileA << std::endl;
    // std::cout << "schedule:\n" << schedule << std::endl;
    EXPECT_EQ(profileA.chargingSchedule, schedule);
    auto enhanced_schedule = handler.calculate_enhanced_composite_schedule(
        valid_profiles, profileA_start_time, profileA_end_time, connector_id, std::nullopt);
    // std::cout << "enhanced schedule:\n" << enhanced_schedule << std::endl;
    EXPECT_EQ(profileA.chargingSchedule, enhanced_schedule);
}

TEST_F(ProfileTests, validate_profileB) {
    // need to have a transaction for calculate_composite_schedule()
    // and calculate_enhanced_composite_schedule()
    int connector_id = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp(now + seconds(5));
    add_connectors(5);
    connectors[connector_id]->transaction =
        std::make_shared<Transaction>(-1, connector_id, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);
    auto tmp_profile = profileB;
    EXPECT_TRUE(
        handler.validate_profile(tmp_profile, 0, true, 100, 10, 10, {ChargingRateUnit::A, ChargingRateUnit::W}));
    // check profile not updated
    EXPECT_EQ(tmp_profile, profileB);
    handler.add_tx_default_profile(tmp_profile, connector_id);
    auto valid_profiles = handler.get_valid_profiles(profileB_start_time, profileB_end_time, connector_id);
    auto schedule = handler.calculate_composite_schedule(valid_profiles, profileB_start_time, profileB_end_time,
                                                         connector_id, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, schedule);
    auto enhanced_schedule = handler.calculate_enhanced_composite_schedule(
        valid_profiles, profileB_start_time, profileB_end_time, connector_id, std::nullopt);
    EXPECT_EQ(profileB.chargingSchedule, enhanced_schedule);
}

TEST_F(ProfileTests, tx_default_0) {
    add_connectors(5);
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);
    ChargingProfile profile{
        201,                                                                 // chargingProfileId
        22,                                                                  // stackLevel
        ChargingProfilePurposeType::TxDefaultProfile,                        // chargingProfilePurpose
        ChargingProfileKindType::Relative,                                   // chargingProfileKind
        {ChargingRateUnit::A, {}, std::nullopt, std::nullopt, std::nullopt}, // chargingSchedule
        std::nullopt,                                                        // transactionId
        std::nullopt,                                                        // recurrencyKind
        std::nullopt,                                                        // validFrom
        std::nullopt,                                                        // validTo
    };
    handler.add_tx_default_profile(profile, 0);
    handler.clear_all_profiles();
}

TEST_F(ProfileTests, single_profile) {
    std::int32_t connector = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime timestamp(now + seconds(5));
    add_connectors(1);

    connectors[1]->transaction =
        std::make_shared<Transaction>(-1, connector, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);

    handler.add_tx_default_profile(profileA, 1);

    auto valid_profiles = handler.get_valid_profiles(profileA_start_time, profileA_end_time, 1);
    // std::cout << valid_profiles << std::endl;
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileA.chargingSchedule, valid_profiles[0].chargingSchedule);

    auto schedule =
        handler.calculate_composite_schedule(valid_profiles, profileA_start_time, profileA_end_time, 1, std::nullopt);
    // std::cout << schedule << std::endl;
    EXPECT_EQ(profileA.chargingSchedule, schedule);
}

TEST_F(ProfileTests, startup_no_charge) {
    std::int32_t connector = 1;
    std::int32_t meter_start = 0;
    ocpp::DateTime start_time(now);
    ocpp::DateTime end_time(now + hours(1));
    ocpp::DateTime timestamp(now);
    add_connectors(1);

    // no active transaction
    // map doesn't include connector 0, database does
    SmartChargingHandler handler(connectors, database_handler, true);

    handler.add_tx_default_profile(profileNoCharge, 1);

    auto valid_profiles = handler.get_valid_profiles(start_time, end_time, 1);
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileNoCharge.chargingSchedule, valid_profiles[0].chargingSchedule);

    // std::cout << "profileNoCharge: no transaction" << std::endl;
    auto schedule = handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, profileNoCharge_end_time,
                                                                  1, std::nullopt);
    // std::cout << "chargingSchedule:" << profileNoCharge.chargingSchedule << std::endl;
    // std::cout << "schedule:" << schedule.chargingSchedulePeriod << std::endl;
    EXPECT_EQ(profileNoCharge.chargingSchedule, schedule);

    // now with a transaction
    connectors[1]->transaction =
        std::make_shared<Transaction>(-1, connector, "1234", "4567", meter_start, std::nullopt, timestamp, nullptr);
    valid_profiles = handler.get_valid_profiles(start_time, profileNoCharge_end_time, 1);
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileNoCharge.chargingSchedule, valid_profiles[0].chargingSchedule);

    // std::cout << "profileNoCharge: with transaction" << std::endl;
    schedule = handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, profileNoCharge_end_time, 1,
                                                             std::nullopt);
    // std::cout << "chargingSchedule:" << profileNoCharge.chargingSchedule << std::endl;
    // std::cout << "schedule:" << schedule.chargingSchedulePeriod << std::endl;
    EXPECT_EQ(profileNoCharge.chargingSchedule, schedule);

    // transaction ended
    start_time = ocpp::DateTime(now + minutes(60));
    connectors[1]->transaction = nullptr;
    // std::cout << "profileNoCharge: with transaction finished" << std::endl;
    valid_profiles = handler.get_valid_profiles(start_time, profileNoCharge_end_time, 1);
    ASSERT_EQ(valid_profiles.size(), 1);
    EXPECT_EQ(profileNoCharge.chargingSchedule, valid_profiles[0].chargingSchedule);

    schedule = handler.calculate_enhanced_composite_schedule(valid_profiles, start_time, profileNoCharge_end_time, 1,
                                                             std::nullopt);
    // std::cout << "chargingSchedule:" << profileNoCharge.chargingSchedule << std::endl;
    // std::cout << "schedule:" << schedule.chargingSchedulePeriod << std::endl;
    EXPECT_EQ(profileNoCharge.chargingSchedule, schedule);
}

} // namespace
