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

class ConnectivityManager {
private: // Members
    std::thread connectivity_thread;
    std::atomic_bool running;
    std::atomic_bool try_reconnect;
    mutable std::mutex config_slot_mutex;
    std::condition_variable reconnect_condition_variable;
    DeviceModel &device_model;
    std::shared_ptr<EvseSecurity> evse_security;
    std::shared_ptr<MessageLogging> logging;
    bool disable_automatic_websocket_reconnects;
    std::unique_ptr<Websocket> websocket;
    /// \brief The potential interface to use requested by libocpp, but not 'approved' by 'core' yet.
    int32_t requested_network_slot;
    /// \brief The interface which will be used to connect
    int32_t pending_network_slot;
    /// \brief The interface which is currently in use by the websocket
    int32_t active_network_slot;

    /* Callbacks for networking */
    std::function<void(const std::string& message)> message_callback;

    /// \brief register a \p callback that is called when the websocket is connected successfully
    std::optional<
        std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>>
        websocket_connected_callback;

    /// \brief register a \p callback that is called when the websocket connection is disconnected
    std::optional<
        std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>>
        websocket_disconnected_callback;

     /// @brief register a \p callback that is called when the network connection profile is to be configured.
    std::optional<std::function<std::future<ConfigNetworkResult>(
        const int32_t configurationSlot, const NetworkConnectionProfile& network_connection_profile)>>
        configure_network_connection_profile_callback;

public:
    ConnectivityManager(DeviceModel &device_model, std::shared_ptr<EvseSecurity> evse_security,
                        std::shared_ptr<MessageLogging> logging,
                        std::function<void(const std::string& message)> message_callback);
    ~ConnectivityManager();
    void set_websocket_connected_callback(
        std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>
            websocket_connected_callback);
    void set_websocket_disconnected_callback(
        std::function<void(const int configuration_slot, const NetworkConnectionProfile& network_connection_profile)>
            websocket_disconnected_callback);
    void set_configure_network_connection_profile_callback(
        std::function<std::future<ConfigNetworkResult>(const int32_t configurationSlot,
                                                       const NetworkConnectionProfile& network_connection_profile)>
            configure_network_connection_profile_callback);

    /// \brief Starts the websocket
    void start();

    /// \brief Stops the ChargePoint. Disconnects the websocket connection and stops MessageQueue and all timers
    void stop();

    /// @brief Initializes the websocket and connects to CSMS if it is not yet connected.
    /// @param configuration_slot Optional configuration slot to connect to
    void connect_websocket(std::optional<int32_t> config_slot = std::nullopt);

    /// \brief Disconnects the the websocket connection to the CSMS if it is connected
    /// \param code Optional websocket close status code (default: normal).
    void disconnect_websocket(WebsocketCloseReason code = WebsocketCloseReason::Normal);

    /// \brief Switch to a specifc network connection profile given the configuration slot.
    /// This disregards the prority
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

private: // Functions
    void run();

    /// @brief Initialize the websocket connection.
    /// @param configuration_slot Optional configuration slot to initialize the websocket to.
    void init_websocket(std::optional<int32_t> config_slot = std::nullopt);
    WebsocketConnectionOptions get_ws_connection_options(const int32_t configuration_slot);

    /// \brief Gets the configured NetworkConnectionProfile based on the given \p configuration_slot . The
    /// central system uri ofthe connection options will not contain ws:// or wss:// because this method removes it if
    /// present
    /// \param configuration_slot   The network profile slot to get the network connection profile from.
    /// \return The network connection profile belonging to the slot or std::nullopt if not found.
    std::optional<NetworkConnectionProfile> get_network_connection_profile(const int32_t configuration_slot);
    /// \brief Get next network slot for next priority.
    int32_t get_next_network_configuration_priority_slot(const int32_t configuration_slot);

    /// @brief Removes all network connection profiles below the actual security profile and stores the new list in the
    /// device model
    void remove_network_connection_profiles_below_actual_security_profile();

    void on_websocket_connected_callback(const int configuration_slot,
                                         const std::optional<NetworkConnectionProfile> network_connection_profile);
    void on_websocket_disconnected_callback(const int configuration_slot,
                                            const std::optional<NetworkConnectionProfile> network_connection_profile);
    void on_websocket_closed_callback(const int configuration_slot,
                                      const std::optional<NetworkConnectionProfile> network_connection_profile,
                                      const WebsocketCloseReason reason);

    ///
    /// \brief Get the active network configuration slot in use.
    /// \return The active slot the network is connected to or the pending slot.
    ///
    int32_t get_active_network_configuration_slot() const;

    std::optional<int32_t> get_configuration_slot_priority(const int32_t configuration_slot);

    ///
    /// \brief Returns true if the provided configuration slot is of higher priority compared to the one currently
    ///        in use.
    /// \param new_configuration_slot   The configuration slot to check.
    /// \return True when given slot is of higher priority.
    ///
    bool is_higher_priority_profile(const int32_t new_configuration_slot);
};

} // namespace v201
} // namespace ocpp
