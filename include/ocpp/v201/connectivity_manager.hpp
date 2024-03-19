// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <functional>
#include <future>
#include <optional>

#include "ocpp/common/websocket/websocket.hpp"
#include "ocpp/common/websocket/websocket_base.hpp"
#include "ocpp/v201/ocpp_types.hpp"

namespace ocpp {
namespace v201 {

class DeviceModel;

///
/// \brief Typedef for the connected and disconnected functions.
/// \param configuration_slot   The slot the websocket is connected with / disconnected from.
/// \param network_connection_profile   The network connection profile the websocket is connected with / disconnected
///                                     from.
///
typedef std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>
    ConnectionCallback;
///
/// \brief Typedef for the callback to configure the network profile.
/// \param configuration_slot   The requested configuration slot.
/// \param network_connection_profile   The requested network connection profile.
/// \return A future containing a ConfigNetworkResult: the value can be set directly when returning or later when the
///         network interface is ready.
///
typedef std::function<std::future<ConfigNetworkResult>(const int32_t configuration_slot,
                                                       const NetworkConnectionProfile& network_connection_profile)>
    ConfigureProfileCallback;

class ConnectivityManager {
private: // Members
    /// \brief The main thread.
    std::thread connectivity_thread;
    /// \brief True when the main thread is running.
    std::atomic_bool running;
    /// \brief When this is true, connectivity manager should try to reconnect. Works together with the condition
    ///        variable.
    std::atomic_bool try_reconnect;
    /// \brief Mutex for the requested, pending and active network slot.
    mutable std::recursive_mutex config_slot_mutex;
    /// \brief Mutex for the condition variable.
    mutable std::mutex reconnect_mutex;
    /// \brief Condition variable that waits until it gets a sign to reconnect.
    std::condition_variable reconnect_condition_variable;
    /// \brief Reference to the device model class.
    DeviceModel& device_model;
    /// \brief Pointer to the evse security class.
    std::shared_ptr<EvseSecurity> evse_security;
    /// \brief Pointer to the logger.
    std::shared_ptr<MessageLogging> logging;
    /// \brief The websocket.
    std::unique_ptr<Websocket> websocket;
    /// \brief The potential interface to use requested by libocpp, but not 'approved' by 'core' yet.
    int32_t requested_network_slot;
    /// \brief The interface which will be used to connect, pending connection at the websocket.
    int32_t pending_network_slot;
    /// \brief The interface which is currently in use by the websocket
    int32_t active_network_slot;
    /// \brief Reconnect timer for when it needs to wait for a specific time to reconnect.
    Everest::SteadyTimer reconnect_timer;
    /// \brief Connection options for the active / pending websocket connection (belonging to the 'websocket' member).
    WebsocketConnectionOptions current_connection_options;
    /// \brief The number of connection attempts, needed to calculate the wait to for a reconnect etc.
    int connection_attempts;
    /// \brief The last reconnect wait time.
    std::chrono::milliseconds reconnect_backoff_ms;

    /* Callbacks for networking */
    /// \brief The message callback.
    std::function<void(const std::string& message)> message_callback;

    /// \brief Callback that is called when the websocket is connected successfully
    std::optional<ConnectionCallback> websocket_connected_callback;

    /// \brief Callback that is called when the websocket connection is disconnected
    std::optional<ConnectionCallback> websocket_disconnected_callback;

    /// @brief Callback that is called when the network connection profile is to be configured.
    std::optional<ConfigureProfileCallback> configure_network_connection_profile_callback;

public:
    ///
    /// \brief Constructor.
    /// \param device_model     Device model reference, needed to read Variables.
    /// \param evse_security    EVSE Security module.
    /// \param logging          Logging module.
    /// \param message_callback The callback for sent messages.
    ///
    ConnectivityManager(DeviceModel& device_model, std::shared_ptr<EvseSecurity> evse_security,
                        std::shared_ptr<MessageLogging> logging,
                        std::function<void(const std::string& message)> message_callback);
    ///
    /// \brief Destructor.
    ///
    virtual ~ConnectivityManager();

    ///
    /// \brief Set the callback that is called when the websocket is connected.
    /// \param websocket_connected_callback The callback.
    ///
    void set_websocket_connected_callback(ConnectionCallback websocket_connected_callback);
    ///
    /// \brief Set the callback that is called when the websocket is disconnected.
    ///
    /// It will be called when the websocket is disconnected and there are no retry's currently for a reconnection.
    ///
    /// \param websocket_disconnected_callback  The callback.
    ///
    void set_websocket_disconnected_callback(ConnectionCallback websocket_disconnected_callback);

    ///
    /// \brief Callback to be able to configure the network connection profile.
    ///
    /// This callback will be called before trying to connect to a network connection profile. The callback will return
    /// a future, which can be set directly or later. This way, the application can set up the network first (for
    /// example enable a modem) and as soon as it is configured, set the future.
    ///
    /// The Variable 'NetworkConfigTimeout' in the 'InternalCtrlr' will define the timeout libocpp will wait for the
    /// future to return.
    ///
    /// \param configure_network_connection_profile_callback    The callback.
    ///
    void set_configure_network_connection_profile_callback(
        ConfigureProfileCallback configure_network_connection_profile_callback);

    /// \brief Starts the websocket
    void start();

    /// \brief Stops the ChargePoint. Disconnects the websocket connection and stops MessageQueue and all timers
    void stop();

    /// \brief Disconnects the the websocket connection to the CSMS if it is connected
    /// \param code Optional websocket close status code (default: normal).
    void disconnect_websocket(WebsocketCloseReason code = WebsocketCloseReason::Normal);

    /// \brief Switch to a specifc network connection profile given the configuration slot.
    ///
    /// Switch will only be done when the configuration slot has a higher priority.
    ///
    /// \param configuration_slot Slot in which the configuration is stored
    /// \return true if the switch is possible.
    bool on_try_switch_network_connection_profile(const int32_t configuration_slot);

    ///
    /// \brief Called when a network is disconnected, for example when an ethernet cable is removed.
    ///
    /// This callback is introduced because the system might see a lot earlier when a network cable is disconnected
    /// than the websocket. For the websocket it might take several minutes while the system might know within seconds
    /// or even earlier. So when the system detects a disconnect of the network, it can call this function. If the
    /// websocket is connected with the network profile in this slot, it can disconnect the websocket.
    ///
    /// \param configuration_slot   The slot of the network connection profile that is disconnected.
    /// \param ocpp_interface       The interface that is disconnected.
    ///
    /// \note At least one of the two params must be provided, otherwise libocpp will not know which interface is down.
    ///
    void on_network_disconnected(const std::optional<int32_t> configuration_slot,
                                 const std::optional<OCPPInterfaceEnum> ocpp_interface);

    ///
    /// \brief Check if websocket is currently connected
    /// \return True if websocket is connected.
    ///
    bool is_websocket_connected();

    ///
    /// \brief Send a message over the websocket
    /// \param message  The message to send.
    /// \return True if the message was sent successfully.
    ///
    bool send_websocket_message(const std::string& message);

    ///
    /// \brief Set the authorization key of the connection_options in Websocket instance
    /// \param authorization_key    Authorization key to set.
    ///
    void set_websocket_authorization_key(const std::string& authorization_key);

    ///
    /// \brief Set connection options for websocket.
    /// \param Connection options.
    ///
    void set_websocket_connection_options(const WebsocketConnectionOptions& connection_options);

    ///
    /// \brief Get the active network configuration slot in use.
    /// \return The active slot the network is connected to or the pending slot.
    ///
    int32_t get_active_network_configuration_slot() const;

private: // Functions
    // Disable copy constructor.
    ConnectivityManager(const ConnectivityManager&) = delete;
    // Disable assignment operator.
    ConnectivityManager operator=(const ConnectivityManager&) = delete;

    void run();

    /// @brief Initialize the websocket connection.
    /// @param configuration_slot Optional configuration slot to initialize the websocket to.
    bool init_websocket(std::optional<int32_t> config_slot = std::nullopt);
    WebsocketConnectionOptions get_ws_connection_options(const int32_t configuration_slot);

    /// \brief Gets the configured NetworkConnectionProfile based on the given \p configuration_slot . The
    /// central system uri ofthe connection options will not contain ws:// or wss:// because this method removes it if
    /// present
    /// \param configuration_slot   The network profile slot to get the network connection profile from.
    /// \return The network connection profile belonging to the slot or std::nullopt if not found.
    std::optional<NetworkConnectionProfile> get_network_connection_profile(const int32_t configuration_slot);
    /// \brief Get next network slot for next priority.
    int32_t get_next_network_configuration_priority_slot(const int32_t configuration_slot);

    ///
    /// \brief Called when the websocket is connected.
    /// \param configuration_slot           Configuration slot the websocket is connected to.
    /// \param network_connection_profile   Network profile the websocket is connected to.
    ///
    void on_websocket_connected_callback(const int configuration_slot,
                                         const std::optional<NetworkConnectionProfile> network_connection_profile);

    ///
    /// \brief Called when the websocket is closed / disconnected.
    /// \param configuration_slot           Configuration slot the websocket is connected to.
    /// \param network_connection_profile   Network profile the websocket is connected to.
    /// \param reason                       The reason the websocket is closed.
    ///
    void on_websocket_closed_callback(const int configuration_slot,
                                      const std::optional<NetworkConnectionProfile> network_connection_profile,
                                      const WebsocketCloseReason reason);

    ///
    /// \brief Called when the websocket failed to connect.
    /// \param configuration_slot           Configuration slot the websocket is connected to.
    /// \param network_connection_profile   Network profile the websocket is connected to.
    /// \param reason                       The reason the websocket is closed.
    ///
    void on_websocket_failed_callback(const int configuration_slot,
                                      const std::optional<NetworkConnectionProfile> network_connection_profile,
                                      const WebsocketCloseReason reason);

    ///
    /// \brief Get the priority of the given configuration slot.
    /// \param configuration_slot   The configuration slot to get the priority from.
    /// \return The priority, if the configuration slot exists.
    ///
    std::optional<int32_t> get_configuration_slot_priority(const int32_t configuration_slot);

    ///
    /// \brief Returns true if the provided configuration slot is of higher priority compared to the one currently
    ///        in use.
    /// \param new_configuration_slot   The configuration slot to check.
    /// \return True when given slot is of higher priority.
    ///
    bool is_higher_priority_profile(const int32_t new_configuration_slot);

    ///
    /// \brief Set the timer to retry to connect the websocket.
    /// \param timeout  The timeout.
    ///
    void set_retry_connection_timer(const std::chrono::milliseconds timeout);

    ///
    /// \brief Get next reconnect interval.
    /// \return The interval.
    ///
    std::chrono::milliseconds get_reconnect_interval();

    ///
    /// \brief Reconnect the websocket.
    /// \param configuration_slot   The slot to reconnect.
    /// \param next_profile         True if it should connect with the next profile.
    ///
    void reconnect(const int32_t configuration_slot, const bool next_profile);
};

} // namespace v201
} // namespace ocpp
