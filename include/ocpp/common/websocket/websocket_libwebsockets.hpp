// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_WEBSOCKET_TLS_TPM_HPP
#define OCPP_WEBSOCKET_TLS_TPM_HPP

#include <ocpp/common/evse_security.hpp>
#include <ocpp/common/websocket/websocket_base.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

struct ssl_ctx_st;

namespace ocpp {

struct ConnectionData;
struct WebsocketMessage;

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

/// \brief Experimental libwebsockets TLS connection
class WebsocketLibwebsockets final : public WebsocketBase {
public:
    /// \brief Creates a new Websocket object with the providede \p connection_options
    explicit WebsocketLibwebsockets(const WebsocketConnectionOptions& connection_options,
                                    std::shared_ptr<EvseSecurity> evse_security);

    ~WebsocketLibwebsockets();

    void set_connection_options(const WebsocketConnectionOptions& connection_options) override;

    /// \brief Starts the connection attempts. It will init the websocket processing thread
    /// \returns true if the websocket is successfully initialized, false otherwise. Does
    ///          not wait for a successful connection
    bool start_connecting() override;

    /// \brief Reconnects the websocket after the delay. Will stop the current connection attempts
    ///        and will call the 'start_connecting' after the delay
    /// \param reason parameter
    /// \param delay delay of the reconnect attempt
    void reconnect(long delay) override;

    /// \brief closes the websocket
    void close(const WebsocketCloseReason code, const std::string& reason) override;

    /// \brief send a \p message over the websocket
    /// \returns true if the message was sent successfully
    bool send(const std::string& message) override;

    /// \brief send a websocket ping
    void ping() override;

    /// \brief Indicates if the websocket has a valid connection data and is trying to
    ///        connect/reconnect internally even if for the moment it might not be connected
    bool is_trying_to_connect();

public:
    int process_callback(void* wsi_ptr, int callback_reason, void* user, void* in, size_t len);

private:
    bool is_trying_to_connect_internal();
    void close_internal(const WebsocketCloseReason code, const std::string& reason);

private:
    /// \brief Initializes the connection options, including the security info
    /// \return True if it was successful, false otherwise
    bool initialize_connection_options(std::shared_ptr<ConnectionData>& new_connection_data);

    bool tls_init(struct ssl_ctx_st* ctx, const std::string& path_chain, const std::string& path_key, bool custom_key,
                  std::optional<std::string>& password);

    /// \brief Websocket processing thread loop
    void thread_websocket_client_loop(std::shared_ptr<ConnectionData> local_data);

    /// \brief Function to handle received messages. Required since from the received message
    ///        callback we also send messages that must block and wait on the client thread
    void thread_websocket_message_recv_loop(std::shared_ptr<ConnectionData> local_data);

    /// \brief Function to handle the deferred callbacks
    void thread_deferred_callback_queue();

    /// \brief Called when a TLS websocket connection is established, calls the connected callback
    void on_conn_connected();

    /// \brief Called when a TLS websocket connection is closed
    void on_conn_close();

    /// \brief Called when a TLS websocket connection fails to be established
    void on_conn_fail();

    /// \brief When the connection can send data
    void on_conn_writable();

    /// \brief Called when a message is received over the TLS websocket, calls the message callback
    void on_conn_message(std::string&& message);

    /// \brief Requests a message write, awakes the websocket loop from 'poll'
    void request_write();

    void poll_message(const std::shared_ptr<WebsocketMessage>& msg);

    /// \brief Add a callback to the queue of callbacks to be executed. All will be executed from a single thread
    void push_deferred_callback(const std::function<void()>& callback);

    // \brief Safely closes the already running connection threads
    void safe_close_threads();

    /// \brief Clears all messages and message queues both incoming and outgoing
    void clear_all_queues();

private:
    std::shared_ptr<EvseSecurity> evse_security;

    // Connection related data
    Everest::SteadyTimer reconnect_timer_tpm;
    std::unique_ptr<std::thread> websocket_thread;
    std::shared_ptr<ConnectionData> conn_data;

    // Queue of outgoing messages
    SafeQueue<std::shared_ptr<WebsocketMessage>> message_queue;

    std::unique_ptr<std::thread> recv_message_thread;
    SafeQueue<std::string> recv_message_queue;
    std::string recv_buffered_message;

    std::unique_ptr<std::thread> deferred_callback_thread;
    SafeQueue<std::function<void()>> deferred_callback_queue;
    std::atomic_bool stop_deferred_handler;

    OcppProtocolVersion connected_ocpp_version;
};

} // namespace ocpp
#endif // OCPP_WEBSOCKET_HPP
