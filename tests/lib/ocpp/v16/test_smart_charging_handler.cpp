#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
namespace fs = std::filesystem;

#include <database_handler_mock.hpp>
#include <evse_security_mock.hpp>
#include <ocpp/common/call_types.hpp>
#include <ocpp/v16/smart_charging.hpp>
#include <optional>


namespace ocpp {
namespace v16 {

/**
 * Chargepoint Test Fixture
 *
 * Test Matrix:
 *
 * Positive Boundary Conditions:
 * - PB01 Valid Profile
 * - PB02 Valid Profile No startSchedule & handler allows no startSchedule & profile.chargingProfileKind == Absolute
 * - PB03 Valid Profile No startSchedule & handler allows no startSchedule & profile.chargingProfileKind == Relative
 * - PB04 Absolute ChargePointMaxProfile Profile with connector id 0
 * - PB05 Absolute TxDefaultProfile
 * - PB06 Absolute TxProfile ignore_no_transaction == true
 * - PB07 Absolute TxProfile && connector transaction != nullptr && transaction_id matches SKIPPED: was not able to test
 *
 * Negative Boundary Conditions:
 * - NB01 Valid Profile, ConnectorID gt this->connectors.size()
 * - NB02 Valid Profile, ConnectorID lt 0
 * - NB03 profile.stackLevel lt 0
 * - NB04 profile.stackLevel gt profile_max_stack_level
 * - NB05 profile.chargingProfileKind == Absolute && !profile.chargingSchedule.startSchedule
 * - NB06 Number of installed Profiles is > max_charging_profiles_installed
 * - NB07 Invalid ChargingSchedule
 * - NB08 profile.chargingProfileKind == Recurring && !profile.recurrencyKind
 * - NB09 profile.chargingProfileKind == Recurring && !startSchedule
 * - NB10 profile.chargingProfileKind == Recurring && !startSchedule && !allow_charging_profile_without_start_schedule
 * - NB11 Absolute ChargePointMaxProfile Profile with connector id not 0
 * - NB12 Absolute TxProfile connector_id == 0
 */
class ChargepointTestFixture : public testing::Test {
protected:
    void SetUp() override {
    }

    ChargingSchedule createChargeSchedule() {
        return ChargingSchedule{{}};
    }

    ChargingSchedule createChargeSchedule(ChargingRateUnit chargingRateUnit) {
        std::vector<ChargingSchedulePeriod> chargingSchedulePeriod;
        std::optional<int32_t> duration;
        std::optional<ocpp::DateTime> startSchedule;
        std::optional<float> minChargingRate;

        return ChargingSchedule{chargingRateUnit, chargingSchedulePeriod, duration, startSchedule, minChargingRate};
    }

    ChargingProfile createChargingProfile(ChargingSchedule chargingSchedule) {
        auto chargingProfileId = 1;
        auto stackLevel = 1;
        auto chargingProfilePurpose = ChargingProfilePurposeType::TxDefaultProfile;
        auto chargingProfileKind = ChargingProfileKindType::Absolute;
        auto recurrencyKind = RecurrencyKindType::Daily;
        return ChargingProfile{
            chargingProfileId,
            stackLevel,
            chargingProfilePurpose,
            chargingProfileKind,
            chargingSchedule,
            {}, // transactionId
            recurrencyKind,
            {}, // validFrom
            {}  // validTo
        };
    }

    /**
     * TxDefaultProfile, stack #1: time-of-day limitation to 2 kW, recurring every day from 17:00h to 20:00h.
     *
     * This profile is Example #1 taken from the OCPP 2.0.1 Spec Part 2, page 241.
     */
    ChargingProfile createChargingProfile_Example1() {
        auto chargingRateUnit = ChargingRateUnit::W;
        auto chargingSchedulePeriod = std::vector<ChargingSchedulePeriod>{ChargingSchedulePeriod{0, 2000, 1}};
        auto duration = 1080;
        auto startSchedule = ocpp::DateTime("2024-01-17T17:00:00");
        float minChargingRate = 0;
        auto chargingSchedule =
            ChargingSchedule{chargingRateUnit, chargingSchedulePeriod, duration, startSchedule, minChargingRate};

        auto chargingProfileId = 1;
        auto stackLevel = 1;
        auto chargingProfilePurpose = ChargingProfilePurposeType::TxDefaultProfile;
        auto chargingProfileKind = ChargingProfileKindType::Absolute;
        auto recurrencyKind = RecurrencyKindType::Daily;
        return ChargingProfile{
            chargingProfileId,
            stackLevel,
            chargingProfilePurpose,
            chargingProfileKind,
            chargingSchedule,
            {}, // transactionId
            recurrencyKind,
            {}, // validFrom
            {}  // validTo
        };
    }

    /**
     * TxDefaultProfile, stack #2: overruling Sundays to no limit, recurring every week starting 2020-01-05.
     *
     * This profile is Example #2 taken from the OCPP 2.0.1 Spec Part 2, page 241.
     */
    ChargingProfile createChargingProfile_Example2() {
        auto chargingRateUnit = ChargingRateUnit::W;
        auto chargingSchedulePeriod = std::vector<ChargingSchedulePeriod>{ChargingSchedulePeriod{0, 999999, 1}};
        auto duration = 0;
        auto startSchedule = ocpp::DateTime("2020-01-19T00:00:00");
        float minChargingRate = 0;
        auto chargingSchedule =
            ChargingSchedule{chargingRateUnit, chargingSchedulePeriod, duration, startSchedule, minChargingRate};

        auto chargingProfileId = 11;
        auto stackLevel = 2;
        auto chargingProfilePurpose = ChargingProfilePurposeType::TxDefaultProfile;
        auto chargingProfileKind = ChargingProfileKindType::Recurring;
        auto recurrencyKind = RecurrencyKindType::Weekly;
        return ChargingProfile{chargingProfileId,
                               stackLevel,
                               chargingProfilePurpose,
                               chargingProfileKind,
                               chargingSchedule,
                               {}, // transactionId
                               recurrencyKind,
                               {},
                               {}};
    }

    SmartChargingHandler* createSmartChargingHandler() {
        connectors[0] = std::make_shared<Connector>(Connector{1});

        const std::string chargepoint_id = "1";
        const fs::path database_path = "na";
        const fs::path init_script_path = "na";

        std::shared_ptr<DatabaseHandlerMock> database_handler =
            std::make_shared<DatabaseHandlerMock>(chargepoint_id, database_path, init_script_path);

        auto handler = new SmartChargingHandler(connectors, database_handler, true);

        return handler;
    }

    // Default values used within the tests
    std::map<int32_t, std::shared_ptr<Connector>> connectors;
    std::shared_ptr<DatabaseHandler> database_handler;

    const int connector_id = 1;
    bool ignore_no_transaction = true;
    const int profile_max_stack_level = 1;
    const int max_charging_profiles_installed = 1;
    const int charging_schedule_max_periods = 1;
};

/**
 * PB01 Valid Profile
 *
 * Happy path simple test
 */
TEST_F(ChargepointTestFixture, ValidateProfile) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * PB01 Valid Profile: Example 1
 *
 * This example is taken from the OCPP 2.0.1 Spec page. 241
 */
TEST_F(ChargepointTestFixture, ValidateProfile__example1) {
    auto profile = createChargingProfile_Example1();
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::W};
    auto handler = createSmartChargingHandler();

    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * NB01 Valid Profile, ConnectorID gt this->connectors.size()
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ConnectorIdGreaterThanConnectorsSize__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    const int connector_id = INT_MAX;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB02 Valid Profile, ConnectorID lt 0
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_NegativeConnectorIdTest__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    const int connector_id = -1;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB03 profile.stackLevel lt 0
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_ConnectorIdZero__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.stackLevel = -1;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB04 profile.stackLevel gt this->profile_max_stack_level
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_StackLevelGreaterThanMaxStackLevel__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.stackLevel = profile_max_stack_level + 1;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB05 profile.chargingProfileKind == Absolute && !profile.chargingSchedule.startSchedule
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_ChargingProfileKindAbsoluteNoStartSchedule__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    // Create a SmartChargingHandler where allow_charging_profile_without_start_schedule is set to false
    auto c1 = std::make_shared<Connector>(Connector{1});
    connectors[1] = c1;
    auto allow_charging_profile_without_start_schedule = false;
    auto handler =
        new SmartChargingHandler(connectors, database_handler, allow_charging_profile_without_start_schedule);

    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    profile.chargingSchedule.startSchedule = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * PB02 Valid Profile No startSchedule & handler allows no startSchedule
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_AllowsNoStartSchedule__ReturnsTrue) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    // Create a SmartChargingHandler where allow_charging_profile_without_start_schedule is set to false
    auto handler = createSmartChargingHandler();

    // Configure to have no start schedule
    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    profile.chargingSchedule.startSchedule = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * NB06 Number of installed Profiles is > max_charging_profiles_installed
 */
TEST_F(ChargepointTestFixture,
       ValidateProfile__ValidProfile_InstalledProfilesGreaterThanMaxInstalledProfiles__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    const int max_charging_profiles_installed = 0;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB07 Invalid ChargingSchedule
 *
 * Creating a ChargingProfile with a different ChargingRateUnit
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_InvalidChargingSchedule__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    profile.chargingSchedule.chargingSchedulePeriod = std::vector<ChargingSchedulePeriod>{};
    auto handler = createSmartChargingHandler();

    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::W};
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 *  NB08 profile.chargingProfileKind == Recurring && !profile.recurrencyKind
 */
TEST_F(ChargepointTestFixture,
       ValidateProfile__ValidProfile_ChargingProfileKindRecurringNoRecurrencyKind__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfileKind = ChargingProfileKindType::Recurring;
    profile.recurrencyKind = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * NB09 profile.chargingProfileKind == Recurring && !profile.chargingSchedule.startSchedule
 */
TEST_F(ChargepointTestFixture,
       ValidateProfile__ValidProfile_ChargingProfileKindRecurringNoStartSchedule__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    // Create a SmartChargingHandler where allow_charging_profile_without_start_schedule is set to false
    auto c1 = std::make_shared<Connector>(Connector{1});
    connectors[1] = c1;
    auto allow_charging_profile_without_start_schedule = false;
    auto handler =
        new SmartChargingHandler(connectors, database_handler, allow_charging_profile_without_start_schedule);

    profile.chargingProfileKind = ChargingProfileKindType::Recurring;
    profile.chargingSchedule.startSchedule = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * PB03 Valid Profile No startSchedule & handler allows no startSchedule & profile.chargingProfileKind == Relative
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_NoStartScheduleAllowedRelative__ReturnsTrue) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfileKind = ChargingProfileKindType::Recurring;
    profile.chargingSchedule.startSchedule = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * NB10 profile.chargingProfileKind == Recurring && !startSchedule && !allow_charging_profile_without_start_schedule
 */
TEST_F(ChargepointTestFixture, ValidateProfile__RecurringNoStartScheduleNotAllowed__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto c1 = std::make_shared<Connector>(Connector{1});
    connectors[1] = c1;
    auto allow_charging_profile_without_start_schedule = false;
    auto handler =
        new SmartChargingHandler(connectors, database_handler, allow_charging_profile_without_start_schedule);

    profile.chargingProfileKind = ChargingProfileKindType::Recurring;
    profile.chargingSchedule.startSchedule = std::nullopt;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * PB04 Absolute ChargePointMaxProfile Profile with connector id 0
 *
 * Absolute ChargePointMaxProfile Profile need a connector id of 0
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_NotRecurrencyKindConnectorId0__ReturnsTrue) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfilePurpose = ChargingProfilePurposeType::ChargePointMaxProfile;
    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    const int connector_id = 0;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * NB11 Absolute ChargePointMaxProfile Profile with connector id not 0
 *
 * ChargePointMaxProfile Profiles where chargingProfileKind == Absolute need a connector id of 0 and not had a
 * ChargingProfileKindType of Relative
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfile_NotRecurrencyKindConnectorIdNot0__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfilePurpose = ChargingProfilePurposeType::ChargePointMaxProfile;
    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    const int connector_id = 1;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}

/**
 * PB05 Absolute TxDefaultProfile
 */
TEST_F(ChargepointTestFixture, ValidateProfile__ValidProfileTxDefaultProfile__ReturnsTrue) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfilePurpose = ChargingProfilePurposeType::TxDefaultProfile;
    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * PB06 Absolute TxProfile ignore_no_transaction == true
 */
TEST_F(ChargepointTestFixture, ValidateProfile__AbsoluteTxProfileIgnoreNoTransaction__ReturnsTrue) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfilePurpose = ChargingProfilePurposeType::TxProfile;
    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_TRUE(sut);
}

/**
 * NB12 Absolute TxProfile connector_id == 0
 */
TEST_F(ChargepointTestFixture, ValidateProfile__AbsoluteTxProfileConnectorId0__ReturnsFalse) {
    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));
    const std::vector<ChargingRateUnit>& charging_schedule_allowed_charging_rate_units{ChargingRateUnit::A};
    auto handler = createSmartChargingHandler();

    profile.chargingProfileKind = ChargingProfileKindType::Absolute;
    profile.chargingProfilePurpose = ChargingProfilePurposeType::TxProfile;
    const int connector_id = 0;
    bool sut = handler->validate_profile(profile, connector_id, ignore_no_transaction, profile_max_stack_level,
                                         max_charging_profiles_installed, charging_schedule_max_periods,
                                         charging_schedule_allowed_charging_rate_units);

    ASSERT_FALSE(sut);
}
/**
 *
 * 2. Testing the branches within clear_all_profiles_with_filter ClearAllProfilesWithFilter
 *
 */

/**
 * NB
 */
TEST_F(ChargepointTestFixture, ClearAllProfilesWithFilter__AllOptionalsEmpty_DoNotCheckIdOnly__ReturnsFalse) {
    auto handler = createSmartChargingHandler();

    bool sut = handler->clear_all_profiles_with_filter({}, {}, {}, {}, false);

    ASSERT_FALSE(sut);
}

TEST_F(ChargepointTestFixture, ClearAllProfilesWithFilter__AllOptionalsEmpty_CheckIdOnly__ReturnsFalse) {
    auto handler = createSmartChargingHandler();

    bool sut = handler->clear_all_profiles_with_filter({}, {}, {}, {}, true);

    ASSERT_FALSE(sut);
}

TEST_F(ChargepointTestFixture, ClearAllProfilesWithFilter__OnlyOneMatchingProfileId_CheckIdOnly__ReturnsTrue) {

    auto handler = createSmartChargingHandler();

    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));

    handler->add_charge_point_max_profile(profile);

    bool sut = handler->clear_all_profiles_with_filter(1, {}, {}, {}, true);

    ASSERT_TRUE(sut);
}

TEST_F(ChargepointTestFixture, ClearAllProfilesWithFilter__NoMatchingProfileId_CheckIdOnly__ReturnsFalse) {
    auto handler = createSmartChargingHandler();

    auto profile = createChargingProfile(createChargeSchedule(ChargingRateUnit::A));

    handler->add_charge_point_max_profile(profile);

    bool sut = handler->clear_all_profiles_with_filter(2, {}, {}, {}, true);

    ASSERT_FALSE(sut);
}

} // namespace v16
} // namespace ocpp