// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest

/// @warning Do not include this file unless it can not conflict with the library!!! // TODO mz

#ifndef EVEREST_TIMER_HPP
#define EVEREST_TIMER_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <condition_variable>
#include <date/date.h>
#include <date/tz.h>
#include <functional>
#include <mutex>
#include <thread>

extern uint32_t timer_stub_stop_called_count;
extern uint32_t timer_stub_timeout_called_count;
extern uint32_t timer_stub_interval_called_count;
extern uint32_t timer_stub_at_called_count;
extern std::function<void()> timer_stub_callback;

namespace Everest {
// template <typename TimerClock = date::steady_clock> class Timer {
template <typename TimerClock = date::utc_clock> class Timer {
public:
private:
    // boost::asio::basic_waitable_timer<TimerClock>* timer = nullptr;
    std::function<void()> callback;
    std::function<void(const boost::system::error_code& e)> callback_wrapper;
    std::chrono::nanoseconds interval_nanoseconds;
    std::condition_variable cv;
    std::mutex wait_mutex;
    bool running = false;
    std::unique_ptr<std::thread> timer_thread = nullptr;
    uint32_t id;
    std::mutex timer_thread_mutex;
    bool call_callback_now = false;

    void run(const bool once = true) {
        running = true;
        while (running) {
            std::unique_lock<std::mutex> lock(timer_thread_mutex);
            cv.wait(lock, [this] { return !running || this->call_callback_now; });
            if (this->call_callback_now && this->callback) {
                this->callback();
                this->call_callback_now = false;

                if (once) {
                    running = false;
                }
            }
        }
    }

public:
    void trigger_timer() {
        {
            std::unique_lock<std::mutex> lock(timer_thread_mutex);
            this->call_callback_now = true;
        }
        cv.notify_all();
    }

    explicit Timer() {
    }

    explicit Timer(const std::function<void()>& callback) {
        timer_stub_callback = callback;
        this->callback = callback;
    }

    explicit Timer(boost::asio::io_context* /*io_context*/) {
    }

    explicit Timer(boost::asio::io_context* /*io_context*/, const std::function<void()>& callback) {
        this->callback = callback;
        timer_stub_callback = callback;
    }

    virtual ~Timer() {
        this->stop();
        if (this->timer_thread != nullptr) {
            if (this->timer_thread->joinable()) {
                this->timer_thread->join();
            }

            this->timer_thread = nullptr;
        }
    }

    /// Executes the given callback at the given timepoint
    template <class Clock, class Duration = typename Clock::duration>
    void at(const std::function<void()>& callback, const std::chrono::time_point<Clock, Duration>& time_point) {
        timer_stub_at_called_count++;
        this->stop();
        this->callback = callback;
        timer_stub_callback = callback;
        this->at(time_point);
    }

    /// Executes the at the given timepoint
    template <class Clock, class Duration = typename Clock::duration>
    void at(const std::chrono::time_point<Clock, Duration>& time_point) {
        timer_stub_at_called_count++;
        this->stop();

        if (this->callback == nullptr) {
            return;
        }

        if (this->timer_thread != nullptr) {
            this->timer_thread->join();
        }

        this->timer_thread = std::make_unique<std::thread>([this]() { this->run(); });
    }

    /// Execute the given callback peridically from now in the given interval
    template <class Rep, class Period>
    void interval(const std::function<void()>& callback, const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_interval_called_count++;
        this->stop();

        this->callback = callback;
        timer_stub_callback = callback;

        this->interval(interval);
    }

    /// Execute peridically from now in the given interval
    template <class Rep, class Period> void interval(const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_interval_called_count++;
        this->stop();
        this->interval_nanoseconds = interval;
        if (interval_nanoseconds == std::chrono::nanoseconds(0)) {
            return;
        }

        if (this->callback == nullptr) {
            return;
        }

        if (this->timer_thread != nullptr) {
            this->timer_thread->join();
        }

        this->timer_thread = std::make_unique<std::thread>([this]() { this->run(); });
    }

    // Execute the given callback once after the given interval
    template <class Rep, class Period>
    void timeout(const std::function<void()>& callback, const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_timeout_called_count++;
        this->stop();

        this->callback = callback;

        this->timeout(interval);

        if (this->timer_thread != nullptr) {
            this->timer_thread->join();
        }

        timer_stub_callback = callback;
        // this->timer_thread = std::make_unique<std::thread>([this]() { this->run(); });
    }

    // Execute the given callback once after the given interval
    template <class Rep, class Period> void timeout(const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_timeout_called_count++;
        this->stop();

        if (this->callback == nullptr) {
            return;
        }

        if (this->timer_thread != nullptr) {
            this->timer_thread->join();
        }

        // this->timer_thread = std::make_unique<std::thread>([this]() {
        //     this->run();
        // });
    }

    /// Stop timer from excuting its callback
    void stop() {
        timer_stub_stop_called_count++;
        running = false;
        cv.notify_all();
    }
};

using SteadyTimer = Timer<date::utc_clock>;
using SystemTimer = Timer<date::utc_clock>;
} // namespace Everest

#endif // EVEREST_TIMER_HPP
