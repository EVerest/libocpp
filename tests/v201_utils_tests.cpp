// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <gtest/gtest.h>
#include <ocpp/v201/utils.hpp>

namespace ocpp {
namespace common {

class V201UtilsTest : public ::testing::Test {
protected:
    const std::string empty_input;
    const std::string short_input = "hello there";
    const std::string long_input =
        "hello there hello there hello there hello there hello there hello there hello there hello there "
        "hello there hello there hello there hello there hello there hello there hello there hello there "
        "hello there hello there hello there hello there hello there hello there hello there hello there "
        "hello there hello there hello there hello there hello there hello there hello there hello there";
    const std::string invalid = "354cacb2d2c45cb28af92ca348ea3a2236ecc48c81c78e0924bf46bd68d9c407";
    const ocpp::v201::IdToken valid_central_token = {"valid", ocpp::v201::IdTokenEnum::Central};
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(V201UtilsTest, test_valid_sha256) {
    ASSERT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
              ocpp::v201::utils::sha256(empty_input));
    ASSERT_EQ("12998c017066eb0d2a70b94e6ed3192985855ce390f321bbdb832022888bd251",
              ocpp::v201::utils::sha256(short_input));
    ASSERT_EQ("34aa8868354dc8eb0e76fbc8b5f13259094bcdf5688c6a48a2bcb89ed863d441",
              ocpp::v201::utils::sha256(long_input));
}

TEST_F(V201UtilsTest, test_invalid_sha256) {
    ASSERT_NE(invalid, ocpp::v201::utils::sha256(empty_input));
    ASSERT_NE(invalid, ocpp::v201::utils::sha256(short_input));
    ASSERT_NE(invalid, ocpp::v201::utils::sha256(long_input));
}

TEST_F(V201UtilsTest, test_valid_generate_token_hash) {
    ocpp::v201::IdToken valid_iso14443_token = {"ABAD1DEA", ocpp::v201::IdTokenEnum::ISO14443};

    ASSERT_EQ("63f3202a9c2e08a033a861481c6259e7a70a2b6e243f91233ebf26f33859c113",
              ocpp::v201::utils::generate_token_hash(valid_central_token));
    ASSERT_EQ("1cc0ce8b95f44d43273c46a062af3d15a06e3d2170909b1fdebd634027aebef1",
              ocpp::v201::utils::generate_token_hash(valid_iso14443_token));
}

TEST_F(V201UtilsTest, test_invalid_generate_token_hash) {
    ASSERT_NE("73f3202a9c2e08a033a861481c6259e7a70a2b6e243f91233ebf26f33859c113",
              ocpp::v201::utils::generate_token_hash(valid_central_token));
}

} // namespace common
} // namespace ocpp
