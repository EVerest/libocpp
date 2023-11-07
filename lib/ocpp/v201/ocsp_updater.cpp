// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <chrono>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <everest/logging.hpp>

#include <ocpp/v201/ocsp_updater.h>

namespace ocpp::v201 {
    OcspUpdater::OcspUpdater(std::shared_ptr<EvseSecurity> evse_security) : evse_security(std::move(evse_security)) {
        // Set the current deadline to "now" - no need to lock the mutex, as the updater thread is not running yet
        this->update_deadline = std::chrono::steady_clock::now();
        // Now create the updater thread - because the deadline is in the past, it will immediately attempt an update
        this->updater_thread = std::thread([this] {
            this->updater_thread_loop();
        });
    }

    void OcspUpdater::trigger_ocsp_cache_update() {
        this->update_ocsp_cache_lock.lock();
        // Move the deadline to "now" so the updater thread doesn't think this is a spurious wakeup
        this->update_deadline = std::chrono::steady_clock::now();
        this->update_ocsp_cache_lock.unlock();
        // Wake up the updater thread
        this->explicit_update_trigger.notify_one();
    }

    void OcspUpdater::updater_thread_loop() {
        while(true) {
            std::unique_lock lock(this->update_ocsp_cache_lock);
            auto current_deadline = this->update_deadline;
            // Wait until the last known deadline expires, or until we're woken up by the trigger
            this->explicit_update_trigger.wait_until(lock, current_deadline);
            // Check that the current deadline has expired - this controls for spurious wakeups
            if(std::chrono::steady_clock::now() <= this->update_deadline) {
                continue;
            }

            // Perform the OCPP cache update
            this->execute_ocsp_update();

            // Set the deadline at a week from now and go back to sleep
            this->update_deadline = std::chrono::steady_clock::now() + OCSP_CACHE_UPDATE_INTERVAL;
        }
    }

    void OcspUpdater::execute_ocsp_update() {
        EVLOG_info << "Updating OCSP cache!";
        // TODO
    }
}