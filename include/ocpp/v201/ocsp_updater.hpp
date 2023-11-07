// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#ifndef OCPP_OCSP_UPDATER_HPP
#define OCPP_OCSP_UPDATER_HPP

#include <chrono>
#include <stdexcept>
#include <thread>
#include <mutex>

#include <ocpp/common/evse_security.hpp>

namespace ocpp::v201 {

const std::chrono::duration OCSP_CACHE_UPDATE_INTERVAL = std::chrono::hours(168);

class OcspUpdateFailedException : public std::exception {
public:
    [[nodiscard]] const char* what() const noexcept override {
        return this->reason.c_str();
    }

    explicit OcspUpdateFailedException(std::string reason, bool can_be_retried = false) :
        reason(std::move(reason)), _allows_retry(can_be_retried) {
    }

    [[nodiscard]] bool allows_retry() const {
        return _allows_retry;
    }

private:
    std::string reason;
    const bool _allows_retry;
};

// Forward declaration to avoid include loops
class ChargePoint;

class OcspUpdater {
public:
    OcspUpdater() = delete;
    explicit OcspUpdater(std::shared_ptr<EvseSecurity> evse_security, ChargePoint* charge_point);

    void start();

    // Wake up the updater thread and tell it to update
    // Used e.g. when a new charging station cert was just installed
    void trigger_ocsp_cache_update();

private:
    // This mutex guards access to everything below it
    std::mutex update_ocsp_cache_lock;
    // Condition variable used to wake up the updater thread
    std::condition_variable explicit_update_trigger;
    // Deadline by which libocpp must automatically trigger an OCSP cache update
    std::chrono::time_point<std::chrono::steady_clock> update_deadline;
    // Worker thread responsible for the updates
    std::thread updater_thread;
    std::shared_ptr<EvseSecurity> evse_security;
    // This pointer will not go stale, because the OcspUpdater is part of the ChargePoint and will not outlive it
    ChargePoint* charge_point;
    boost::uuids::random_generator uuid_generator;

    void updater_thread_loop();
    // Helper function that actually performs the OCSP update. Only called within updater_thread_loop().
    void execute_ocsp_update();
};

} // namespace ocpp::v201

#endif // OCPP_OCSP_UPDATER_HPP
