// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace ocpp {

enum class EThreadNotifyPolicy {
    // Never notify the waiting thread
    ThreadNotify_Never,
    // Notify the waiting thread when we push an element in the queue
    ThreadNotify_Push,
    // Notify the waiting thread when we pop an element from the queue
    ThreadNotify_Pop,
    // Always notify a waiting thread on all operations
    ThreadNotify_Always,
};

/// \brief Thread safe message queue
template <typename T, EThreadNotifyPolicy Policy = EThreadNotifyPolicy::ThreadNotify_Push> class SafeQueue {
    using safe_queue_reference = typename std::queue<T>::reference;
    using safe_queue_const_reference = typename std::queue<T>::const_reference;

public:
    /// \return True if the queue is empty
    inline bool empty() const {
        std::lock_guard lock(mutex);
        return queue.empty();
    }

    inline safe_queue_reference front() {
        std::lock_guard lock(mutex);
        return queue.front();
    }

    inline safe_queue_const_reference front() const {
        std::lock_guard lock(mutex);
        return queue.front();
    }

    /// \return retrieves and removes the first element in the queue. Undefined behavior if the queue is empty
    inline T pop() {
        std::unique_lock<std::mutex> lock(mutex);

        T front = std::move(queue.front());
        queue.pop();

        // Unlock here and notify
        lock.unlock();

        if constexpr (Policy == EThreadNotifyPolicy::ThreadNotify_Always ||
                      Policy == EThreadNotifyPolicy::ThreadNotify_Pop) {
            notify_waiting_thread();
        }

        return front;
    }

    /// \brief Queues an element and notifies any threads waiting on the internal conditional variable
    inline void push(T&& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(value);
        }

        if constexpr (Policy == EThreadNotifyPolicy::ThreadNotify_Always ||
                      Policy == EThreadNotifyPolicy::ThreadNotify_Push) {
            notify_waiting_thread();
        }
    }

    /// \brief Queues an element and notifies any threads waiting on the internal conditional variable
    inline void push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(value);
        }

        if constexpr (Policy == EThreadNotifyPolicy::ThreadNotify_Always ||
                      Policy == EThreadNotifyPolicy::ThreadNotify_Push) {
            notify_waiting_thread();
        }
    }

    /// \brief Clears the queue
    inline void clear() {
        {
            std::lock_guard<std::mutex> lock(mutex);

            std::queue<T> empty;
            empty.swap(queue);
        }

        if constexpr (Policy != EThreadNotifyPolicy::ThreadNotify_Never) {
            // Clear should make all waiting threads
            // wake to check for other states
            notify_waiting_thread();
        }
    }

    /// \brief Waits for the queue to receive an element
    /// \param timeout to wait for an element, pass in a value <= 0 to wait indefinitely
    inline void wait_on_queue_element(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        wait_on_queue_element_predicate([]() { return false; }, timeout);
    }

    /// \brief Same as 'wait_on_queue' but receives an additional predicate to wait upon
    template <class Predicate>
    inline void wait_on_queue_element_predicate(Predicate pred,
                                                std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> lock(mutex);

        if (timeout.count() > 0) {
            cv.wait_for(lock, timeout, [&]() { return (false == queue.empty()) or pred(); });
        } else {
            cv.wait(lock, [&]() { return (false == queue.empty()) or pred(); });
        }
    }

    /// \brief Waits on the queue for a custom event
    /// \param timeout to wait for an element, pass in a value <= 0 to wait indefinitely
    template <class Predicate>
    inline void wait_on_custom_event(Predicate pred, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> lock(mutex);

        if (timeout.count() > 0) {
            cv.wait_for(lock, timeout, [&]() { return pred(); });
        } else {
            cv.wait(lock, [&]() { return pred(); });
        }
    }

    /// \brief Notifies a single waiting thread to wake up
    inline void notify_waiting_thread() {
        cv.notify_one();
    }

private:
    std::queue<T> queue;

    mutable std::mutex mutex;
    std::condition_variable cv;
};

} // namespace ocpp