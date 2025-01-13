// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest

/// @warning Do not include this file unless it can not conflict with the library!!! // TODO mz

#ifndef EVEREST_TIMER_HPP
#define EVEREST_TIMER_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <date/date.h>
#include <date/tz.h>
#include <functional>

#include "../timer_stub.hpp"

namespace Everest {
// template <typename TimerClock = date::steady_clock> class Timer {
template <typename TimerClock = date::utc_clock> class Timer {

public:


    explicit Timer() {
    }

    explicit Timer(const std::function<void()>& callback) {
        timer_stub_set_callback(callback);
    }

    explicit Timer(boost::asio::io_context* /*io_context*/) {
    }

    explicit Timer(boost::asio::io_context* /*io_context*/, const std::function<void()>& callback) {
        timer_stub_set_callback(callback);
    }

    virtual ~Timer() {
    }

    /// Executes the given callback at the given timepoint
    template <class Clock, class Duration = typename Clock::duration>
    void at(const std::function<void()>& callback, const std::chrono::time_point<Clock, Duration>& time_point) {
        timer_stub_at_called(1);
        timer_stub_set_callback(callback);
    }

    /// Executes the at the given timepoint
    template <class Clock, class Duration = typename Clock::duration>
    void at(const std::chrono::time_point<Clock, Duration>& time_point) {
        timer_stub_at_called(1);
    }

    /// Execute the given callback peridically from now in the given interval
    template <class Rep, class Period>
    void interval(const std::function<void()>& callback, const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_interval_called(1);
        timer_stub_set_callback(callback);
    }

    /// Execute peridically from now in the given interval
    template <class Rep, class Period> void interval(const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_interval_called(1);
    }

    // Execute the given callback once after the given interval
    template <class Rep, class Period>
    void timeout(const std::function<void()>& callback, const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_set_callback(callback);
        timer_stub_timeout_called(1);
    }

    // Execute the given callback once after the given interval
    template <class Rep, class Period> void timeout(const std::chrono::duration<Rep, Period>& interval) {
        timer_stub_timeout_called(1);
    }

    /// Stop timer from excuting its callback
    void stop() {
        timer_stub_stop_called(1);
    }
};

using SteadyTimer = Timer<date::utc_clock>;
using SystemTimer = Timer<date::utc_clock>;
} // namespace Everest

#endif // EVEREST_TIMER_HPP
