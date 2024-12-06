// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace ocpp {

/// \brief Thread safe message queue
template <typename T> class SafeQueue {
    using safe_queue_reference = typename std::queue<T>::reference;
    using safe_queue_const_reference = typename std::queue<T>::const_reference;

public:
    /// \return True if the queue is empty
    inline bool empty() const {
        std::lock_guard lock(mutex);
        return queue.empty();
    }

    inline safe_queue_reference front() const {
        std::lock_guard lock(mutex);
        return queue.front();
    }

    inline safe_queue_const_reference front() {
        std::lock_guard lock(mutex);
        return queue.front();
    }

    /// \return retrieves and removes the first element in the queue. Undefined behavior if the queue is empty
    inline T pop() {
        std::lock_guard lock(mutex);

        T front = std::move(queue.front());
        queue.pop();

        return front;
    }

    /// \brief Queues an element and notifies any threads waiting on the internal conditional variable
    inline void push(T&& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(value);
        }

        notify_waiting_thread();
    }

    /// \brief Queues an element and notifies any threads waiting on the internal conditional variable
    inline void push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(value);
        }

        notify_waiting_thread();
    }

    /// \brief Clears the queue
    inline void clear() {
        std::lock_guard<std::mutex> lock(mutex);

        std::queue<T> empty;
        empty.swap(queue);
    }

    /// \brief Waits seconds for the queue to receive an element
    /// \param seconds Count of seconds to wait, pass in a value <= 0 to wait indefinitely
    inline void wait_on_queue(int seconds = -1) {
        std::unique_lock<std::mutex> lock(mutex);

        if (seconds > 0) {
            cv.wait_for(lock, std::chrono::seconds(seconds), [&]() { return (false == queue.empty()); });
        } else {
            cv.wait(lock, [&]() { return (false == queue.empty()); });
        }
    }

    /// \brief Same as 'wait_on_queue' but receives an additional predicate to wait upon
    template <class Predicate> inline void wait_on_queue(Predicate pred, int seconds = -1) {
        std::unique_lock<std::mutex> lock(mutex);

        if (seconds > 0) {
            cv.wait_for(lock, std::chrono::seconds(seconds), [&]() { return (false == queue.empty()) or pred(); });
        } else {
            cv.wait(lock, [&]() { return (false == queue.empty()) or pred(); });
        }
    }

    /// \brief Waits on the queue for a custom event
    template <class Predicate> inline void wait_on_custom(Predicate pred, int seconds = -1) {
        std::unique_lock<std::mutex> lock(mutex);

        if (seconds > 0) {
            cv.wait_for(lock, std::chrono::seconds(seconds), [&]() { return pred(); });
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