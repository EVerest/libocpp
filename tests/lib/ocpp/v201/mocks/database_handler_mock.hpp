// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2025 Pionix GmbH and Contributors to EVerest

#pragma once

#include "gmock/gmock.h"

#include "ocpp/v201/database_handler.hpp"

namespace ocpp::v201 {
class DatabaseHandlerMock : public DatabaseHandlerInterface {
public:
    // DatabaseHandlerCommon

    MOCK_METHOD(void, open_connection, (), (override));

    MOCK_METHOD(void, close_connection, (), (override));

    MOCK_METHOD(std::vector<DBTransactionMessage>, get_message_queue_messages, (const QueueType queue_type),
                (override));

    MOCK_METHOD(void, insert_message_queue_message, (const DBTransactionMessage& message, const QueueType queue_type),
                (override));

    MOCK_METHOD(void, remove_message_queue_message, (const std::string& unique_id, const QueueType queue_type),
                (override));

    MOCK_METHOD(void, clear_message_queue, (const QueueType queue_type), (override));

    // Authorization cache

    MOCK_METHOD(void, authorization_cache_insert_entry,
                (const std::string& id_token_hash, const IdTokenInfo& id_token_info), (override));

    MOCK_METHOD(void, authorization_cache_update_last_used, (const std::string& id_token_hash), (override));

    MOCK_METHOD(std::optional<AuthorizationCacheEntry>, authorization_cache_get_entry,
                (const std::string& id_token_hash), (override));

    MOCK_METHOD(void, authorization_cache_delete_entry, (const std::string& id_token_hash), (override));

    MOCK_METHOD(void, authorization_cache_delete_nr_of_oldest_entries, (size_t nr_to_remove), (override));

    MOCK_METHOD(void, authorization_cache_delete_expired_entries,
                (std::optional<std::chrono::seconds> auth_cache_lifetime), (override));

    MOCK_METHOD(void, authorization_cache_clear, (), (override));

    MOCK_METHOD(size_t, authorization_cache_get_binary_size, (), (override));

    // Availability

    MOCK_METHOD(void, insert_cs_availability, (OperationalStatusEnum operational_status, bool replace), (override));
    MOCK_METHOD(OperationalStatusEnum, get_cs_availability, (), (override));

    MOCK_METHOD(void, insert_evse_availability,
                (int32_t evse_id, OperationalStatusEnum operational_status, bool replace), (override));

    MOCK_METHOD(OperationalStatusEnum, get_evse_availability, (int32_t evse_id), (override));

    MOCK_METHOD(void, insert_connector_availability,
                (int32_t evse_id, int32_t connector_id, OperationalStatusEnum operational_status, bool replace),
                (override));
    MOCK_METHOD(OperationalStatusEnum, get_connector_availability, (int32_t evse_id, int32_t connector_id), (override));

    // Local authorization list management

    MOCK_METHOD(void, insert_or_update_local_authorization_list_version, (int32_t version), (override));

    MOCK_METHOD(int32_t, get_local_authorization_list_version, (), (override));

    MOCK_METHOD(void, insert_or_update_local_authorization_list_entry,
                (const IdToken& id_token, const IdTokenInfo& id_token_info), (override));

    MOCK_METHOD(void, insert_or_update_local_authorization_list,
                (const std::vector<v201::AuthorizationData>& local_authorization_list), (override));

    MOCK_METHOD(void, delete_local_authorization_list_entry, (const IdToken& id_token), (override));

    MOCK_METHOD(std::optional<v201::IdTokenInfo>, get_local_authorization_list_entry, (const IdToken& id_token),
                (override));

    MOCK_METHOD(void, clear_local_authorization_list, (), (override));

    MOCK_METHOD(int32_t, get_local_authorization_list_number_of_entries, (), (override));

    // Transaction metervalues

    MOCK_METHOD(void, transaction_metervalues_insert,
                (const std::string& transaction_id, const MeterValue& meter_value), (override));

    MOCK_METHOD(std::vector<MeterValue>, transaction_metervalues_get_all, (const std::string& transaction_id),
                (override));

    MOCK_METHOD(void, transaction_metervalues_clear, (const std::string& transaction_id), (override));

    // transactions
    MOCK_METHOD(void, transaction_insert, (const EnhancedTransaction& transaction, int32_t evse_id), (override));

    MOCK_METHOD(std::unique_ptr<EnhancedTransaction>, transaction_get, (const int32_t evse_id), (override));

    MOCK_METHOD(void, transaction_update_seq_no, (const std::string& transaction_id, int32_t seq_no), (override));

    MOCK_METHOD(void, transaction_update_charging_state,
                (const std::string& transaction_id, const ChargingStateEnum charging_state), (override));

    MOCK_METHOD(void, transaction_update_id_token_sent, (const std::string& transaction_id, bool id_token_sent),
                (override));

    MOCK_METHOD(void, transaction_delete, (const std::string& transaction_id), (override));

    /// charging profiles
    MOCK_METHOD(void, insert_or_update_charging_profile_impl,
                (const int evse_id, const v201::ChargingProfile& profile, const ChargingLimitSourceEnum), ());

    void insert_or_update_charging_profile(
        const int evse_id, const v201::ChargingProfile& profile,
        const ChargingLimitSourceEnum charging_limit_source = ChargingLimitSourceEnum::CSO) override {
        insert_or_update_charging_profile_impl(evse_id, profile, charging_limit_source);
    }

    MOCK_METHOD(bool, delete_charging_profile, (const int profile_id), (override));

    MOCK_METHOD(void, delete_charging_profile_by_transaction_id, (const std::string& transaction_id), (override));

    MOCK_METHOD(bool, clear_charging_profiles, (), (override));

    MOCK_METHOD(bool, clear_charging_profiles_matching_criteria,
                (const std::optional<int32_t> profile_id, const std::optional<ClearChargingProfile>& criteria),
                (override));

    MOCK_METHOD(std::vector<ReportedChargingProfile>, get_charging_profiles_matching_criteria,
                (const std::optional<int32_t> evse_id, const ChargingProfileCriterion& criteria), (override));

    MOCK_METHOD(std::vector<v201::ChargingProfile>, get_charging_profiles_for_evse, (const int evse_id), (override));

    MOCK_METHOD(std::vector<v201::ChargingProfile>, get_all_charging_profiles, (), (override));

    MOCK_METHOD((std::map<int32_t, std::vector<v201::ChargingProfile>>), get_all_charging_profiles_group_by_evse, (),
                (override));

    MOCK_METHOD(ChargingLimitSourceEnum, get_charging_limit_source_for_profile, (const int profile_id), (override));

    MOCK_METHOD(std::unique_ptr<common::SQLiteStatementInterface>, new_statement, (const std::string& sql), (override));
};
} // namespace ocpp::v201