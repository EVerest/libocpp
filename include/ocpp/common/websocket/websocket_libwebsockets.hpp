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

    /*
        Behavior to clarity:
            1. what happens when we called 'start_connecting' and someone called it again:
                -
            2. what happens when we 'start_connecting' and it returns 'false' because of
            some very bad config. should we still attempt to reconnect on a 'false' return?
            3. why is a function called 'reconnect' required, if the 'start_connecting'
            handles the reconnection internally? does anyone use the 'reconnect' externally?
            4. what happens when we 'start_connecting' and are in the process of attempting to
            connect but we are not connected yet, just attempting to in 'lws_client_connect_via_info'
            and while we are not connecting someone calls 'start_connecting' again? Should we close
            the connection, reinitialize the connection options and restart the client thread? but if
            somebody checked 'is_connected' and because it returns false while we are 'trying' it doesn't
            attempt to disconnect first before calling 'start_connecting'
            5. instead of having a 'disconnect' or 'close' function shouldn't we have a 'stop_connecting'
            one that does the opposite of 'start_connecting', that is to stop the attempts to connect?
            6. in that case should we re-name 'bool Websocket::is_connected()' to 'is_connecting()'?


            7. Looks bad because of the above reasons
            if (this->is_websocket_connected()) {
                // After the websocket gets closed a reconnect will be triggered
                this->websocket->disconnect(WebsocketCloseReason::ServiceRestart);
            } else {
                this->try_connect_websocket();
            }

            should become:
            if (this->is_websocket_trying_to_connect()) {
                // After the websocket gets closed a reconnect will be triggered
                this->websocket->stop_connecting(WebsocketCloseReason::ServiceRestart);
            } else {
                this->try_start_connecting_websocket();
            }
    */

    /// \brief Starts the connection attempts. It will try to initialize the connection options and the
    ///        security context
    /// \returns true if the websocket successfully initialized the connection options and security context and
    ///          if it successfully started the connection thread. Does not wait for a successful connection
    bool start_connecting() override;

    /// \brief Reconnects the websocket using the delay, a reason for this reconnect can be provided with the
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

public:
    int process_callback(void* wsi_ptr, int callback_reason, void* user, void* in, size_t len);

private:
    bool tls_init(struct ssl_ctx_st* ctx, const std::string& path_chain, const std::string& path_key, bool custom_key,
                  std::optional<std::string>& password);
    void client_loop();
    void recv_loop();

    /// \brief Called when a TLS websocket connection is established, calls the connected callback
    void on_conn_connected();

    /// \brief Called when a TLS websocket connection is closed
    void on_conn_close();

    /// \brief Called when a TLS websocket connection fails to be established
    void on_conn_fail();

    /// \brief When the connection can send data
    void on_writable();

    /// \brief Called when a message is received over the TLS websocket, calls the message callback
    void on_message(std::string&& message);

    void request_write();

    void poll_message(const std::shared_ptr<WebsocketMessage>& msg);

    /// \brief Function to handle the deferred callbacks
    void handle_deferred_callback_queue();

    /// \brief Add a callback to the queue of callbacks to be executed. All will be executed from a single thread
    void push_deferred_callback(const std::function<void()>& callback);

private:
    std::shared_ptr<EvseSecurity> evse_security;

    // Connection related data
    Everest::SteadyTimer reconnect_timer_tpm;
    std::unique_ptr<std::thread> websocket_thread;
    std::shared_ptr<ConnectionData> conn_data;
    std::condition_variable conn_cv;

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
