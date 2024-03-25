// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include "ocpp/v201/messages/SetNetworkProfile.hpp"
#include <everest/logging.hpp>
#include <ocpp/v201/connectivity_manager.hpp>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>

// cmake .. -DLIBOCPP_ENABLE_LIBWEBSOCKETS=ON
//     https://github.com/EVerest/libocpp

namespace ocpp {
namespace v201 {

/// \brief Default timeout for the return value (future) of the `configure_network_connection_profile_callback`
///        function.
constexpr int32_t default_network_config_timeout_seconds = 60;
/// \brief If no network connection profile is set, we will retry every x seconds if there already is a network
///        connection profile.
constexpr int32_t default_retry_network_connection_profile_seconds = 300;

ConnectivityManager::ConnectivityManager(DeviceModel& device_model, std::shared_ptr<EvseSecurity> evse_security,
                                         std::shared_ptr<MessageLogging> logging,
                                         std::function<void(const std::string& message)> message_callback) :
    connectivity_thread(),
    running(false),
    try_reconnect(true),
    config_slot_mutex(),
    reconnect_mutex(),
    reconnect_condition_variable(),
    device_model(device_model),
    evse_security(evse_security),
    logging(logging),
    websocket(nullptr),
    requested_network_slot(0),
    pending_network_slot(0),
    active_network_slot(0),
    reconnect_timer(),
    current_connection_options(),
    connection_attempts(0), // TODO don't forget to reset when new websocket is created.
    reconnect_backoff_ms(0),
    message_callback(message_callback) {
}

ConnectivityManager::~ConnectivityManager() {
    stop();
}

void ConnectivityManager::set_websocket_connected_callback(ConnectionCallback websocket_connected_callback) {
    this->websocket_connected_callback = websocket_connected_callback;
}

void ConnectivityManager::set_websocket_disconnected_callback(ConnectionCallback websocket_disconnected_callback) {
    this->websocket_disconnected_callback = websocket_disconnected_callback;
}

void ConnectivityManager::set_configure_network_connection_profile_callback(
    ConfigureProfileCallback configure_network_connection_profile_callback) {
    this->configure_network_connection_profile_callback = configure_network_connection_profile_callback;
}

void ConnectivityManager::start() {
    // Start the main thread and keep running it until 'running' is false.
    // Inside the 'run' function, there is a condition variable preventing the thread from spinning.
    if (!running) {
        running = true;
        connectivity_thread = std::thread([this]() {
            pthread_setname_np(pthread_self(), "ConnMann");
            while (this->running) {
                this->run();
            }
        });
    }
}

void ConnectivityManager::stop() {
    if (running) {
        EVLOG_debug << "Stop connectivity manager, stop reconnect timer and disconnect websocket";
        running = false;
        reconnect_timer.stop();
        this->websocket = nullptr;
        this->reconnect_condition_variable.notify_all();
        if (connectivity_thread.joinable()) {
            connectivity_thread.join();
        }
    }
}

void ConnectivityManager::disconnect_websocket(WebsocketCloseReason code) {
    if (this->websocket == nullptr) {
        return;
    }

    if (this->websocket->is_connected()) {
        this->websocket->disconnect(code);
    }
}

bool ConnectivityManager::on_try_switch_network_connection_profile(const int32_t configuration_slot) {
    if (is_higher_priority_profile(configuration_slot)) {
        // Priority is indeed higher. Continue.
        EVLOG_info << "Trying to connect with higher priority network connection profile (configuration slots: "
                   << this->get_active_network_configuration_slot() << " --> " << configuration_slot << ").";
    } else {
        return false;
    }

    const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
        this->get_network_connection_profile(configuration_slot);
    if (!network_connection_profile_opt.has_value()) {
        EVLOG_debug << "Could not find network connection profile belonging to configuration slot "
                    << configuration_slot;
        return false;
    }

    try {
        // Stop reconnect timer, if it is running -> we will reconnect with another websocket.
        reconnect_timer.stop();

        {
            std::unique_lock<std::recursive_mutex> lock(this->config_slot_mutex);
            this->disconnect_websocket(WebsocketCloseReason::Normal); // normal close
            this->active_network_slot = 0;
            this->pending_network_slot = 0;
            this->requested_network_slot = configuration_slot;
        }

        // Notify main thread that it should reconnect.
        std::unique_lock<std::mutex> reconnect_lock(this->reconnect_mutex);
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();

        return true;
    } catch (std::exception& e) {
        EVLOG_info << "Error when trying to switch network connection profile: " << e.what();
        return false;
    }
}

void ConnectivityManager::on_network_disconnected(const std::optional<int32_t> configuration_slot,
                                                  const std::optional<OCPPInterfaceEnum> ocpp_interface) {
    if (!configuration_slot.has_value() && !ocpp_interface.has_value()) {
        EVLOG_info << "Network disconnected. Not clear which network is disconnected: configuration slot and ocpp "
                      "interface are empty";
        return;
    }

    EVLOG_debug << "Network disconnected with configuration slot: " << configuration_slot.value();

    int32_t disconnected_network_slot = 0;

    // Find slot belonging to ocpp interface. That's only interesting if it is an active or pending network slot.
    const int32_t current_network_slot = get_active_network_configuration_slot();

    if (configuration_slot.has_value()) {
        disconnected_network_slot = configuration_slot.value();
    } else {
        if (current_network_slot != 0) {
            std::optional<NetworkConnectionProfile> profile =
                this->get_network_connection_profile(current_network_slot);
            if (profile.has_value() && profile.value().ocppInterface == ocpp_interface) {
                disconnected_network_slot = current_network_slot;
            }
        }
    }

    if ((disconnected_network_slot != 0) && (current_network_slot == disconnected_network_slot)) {
        // Websocket is indeed connecting with the given slot or already connected.
        this->disconnect_websocket(WebsocketCloseReason::GoingAway);
        // Since there is no connection anymore: reconnect with the next available network connection profile.
        reconnect(disconnected_network_slot, true);
    }
}

bool ConnectivityManager::is_websocket_connected() {
    return this->websocket != nullptr && this->websocket->is_connected();
}

bool ConnectivityManager::send_websocket_message(const std::string& message) {
    if (this->websocket == nullptr) {
        return false;
    }

    return this->websocket->send(message);
}

void ConnectivityManager::set_websocket_authorization_key(const std::string& authorization_key) {
    if (this->websocket != nullptr) {
        this->websocket->set_authorization_key(authorization_key);
    }
}

void ConnectivityManager::set_websocket_connection_options(const WebsocketConnectionOptions& connection_options) {
    this->current_connection_options = connection_options;
    if (this->websocket != nullptr) {
        this->websocket->set_connection_options(connection_options);
    }
}

void ConnectivityManager::set_websocket_connection_options_without_reconnect()
{
    if (this->websocket == nullptr) {
        return;
    }

    const int32_t active_slot = this->get_active_network_configuration_slot();

    WebsocketConnectionOptions connection_options =
        this->get_ws_connection_options(active_slot);
    if (this->current_connection_options.iface_or_ip.has_value())
    {
        // This is set later and not retrieved from the get_ws_connection_options function, so copy from the current
        // connection options.
        connection_options.iface_or_ip = this->current_connection_options.iface_or_ip.value();
    }


    this->current_connection_options = connection_options;
    this->websocket->set_connection_options(connection_options);
}

int32_t ConnectivityManager::get_active_network_configuration_slot() const {
    std::unique_lock<std::recursive_mutex> lock(this->config_slot_mutex);
    return (active_network_slot != 0 ? active_network_slot : pending_network_slot);
}

void ConnectivityManager::run() {
    {
        std::unique_lock<std::mutex> lock(reconnect_mutex);
        // Try_reconnect was set by the function that wanted to 'wake up' the thread an run this function again. We
        // set it to false so it will not continuously fun this function.
        this->try_reconnect.store(false);
    }

    std::unique_lock<std::recursive_mutex> config_slot_lock(this->config_slot_mutex);
    if (this->requested_network_slot != 0) {
        EVLOG_debug << "Init and connect websocket for requested network slot: " << this->requested_network_slot;
        if (this->init_websocket(this->requested_network_slot) && this->websocket != nullptr) {
            this->websocket->connect();
        } else {
            // TODO what to do here???
        }
        // else set try reconnect to true???
    } else if (this->active_network_slot == 0 && this->pending_network_slot == 0) {
        EVLOG_debug << "No active, pending or requested network slot, connect websocket with the highest priority "
                       "network connection profile.";
        if (this->init_websocket() && this->websocket != nullptr) {
            this->websocket->connect();
        } else {
            EVLOG_info << "Can not connect websocket: init websocket failed.";
            // TODO what to do here???
        }
    } else if (this->pending_network_slot != 0) {
        // There is a network slot pending, maybe because it wants to try a reconnect, try to connect the websocket now.
        EVLOG_debug << "Init and connect websocket for pending network slot: " << this->pending_network_slot;
        if (this->websocket != nullptr) {
            this->websocket->reconnect();
        } else {
            if (this->init_websocket(this->pending_network_slot) && this->websocket != nullptr) {
                this->websocket->connect();
            } else {
                // TODO what to do here???
            }
        }
    }

    config_slot_lock.unlock();

    // Wait until a reconnect is requested for whatever reason (normally someone called
    // 'reconnect_condition_variable.notify_<something>').
    std::unique_lock<std::mutex> lock(reconnect_mutex);
    // Return when 'running' is set to false (we want to stop the thread) or 'try_reconnect' is true (we want to
    // reconnect to the websocket).
    // TODO is there a situation where we wait here forever??
    reconnect_condition_variable.wait(lock, [this] { return !this->running.load() || this->try_reconnect.load(); });
    // At this point we want to re-run this function. Since 'run' is called in a while loop, it will be called again
    // directly after returning this function.
}

bool ConnectivityManager::init_websocket(std::optional<int32_t> config_slot) {
    std::string configuration_slot;
    if (this->device_model.get_value<std::string>(ControllerComponentVariables::ChargePointId).find(':') !=
        std::string::npos) {
        EVLOG_AND_THROW(std::runtime_error("ChargePointId must not contain \':\'"));
    }

    int32_t config_slot_int;
    if (config_slot.has_value()) {
        EVLOG_debug << "Init websocket using configuration slot " << config_slot.value();
        configuration_slot = std::to_string(config_slot.value());
        config_slot_int = config_slot.value();
    } else {
        // Get first available config slot.
        const std::vector<std::string> config_slot_vector = ocpp::get_vector_from_csv(
            this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
        if (config_slot_vector.size() > 0) {
            configuration_slot = config_slot_vector.at(0);
            config_slot_int = std::stoi(configuration_slot);
            EVLOG_debug << "Init websocket without config slot, use first available: " << config_slot_int;
        } else {
            // No network connection profile. Retry connecting after some time, maybe it is set manually.
            EVLOG_info << "No network connection profile set. Try again later.";
            sleep(default_retry_network_connection_profile_seconds);
            return false;
        }
    }

    std::unique_lock<std::recursive_mutex> config_slot_lock(this->config_slot_mutex);
    WebsocketConnectionOptions connection_options = this->get_ws_connection_options(config_slot_int);
    const std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(config_slot_int);

    if (!network_connection_profile.has_value()) {
        // No network connection profile found, what to connect to then? So get next profile and try to connect with
        // that one.
        this->requested_network_slot = this->get_next_network_configuration_priority_slot(config_slot_int);
        std::unique_lock<std::mutex> lock(reconnect_mutex);
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
        return false;
    }

    // Check if the callback is set to first configure the network connection profile. If it is, call the callback
    // and set the `requested_network_slot` accordingly. Then wait for the future to return.
    // If the network is available, the requested slot will be stored in the `pending_network_slot` until the connection
    // with the websocket is really established, then the `pending_network_slot` slot will be stored in the
    // `active_network_slot`.
    if (this->configure_network_connection_profile_callback.has_value()) {
        EVLOG_debug << "Request to configure network connection profile " << config_slot_int;
        this->requested_network_slot = config_slot_int;
        std::future<ConfigNetworkResult> config_status = this->configure_network_connection_profile_callback.value()(
            config_slot_int, network_connection_profile.value());
        const int32_t config_timeout =
            this->device_model.get_optional_value<int>(ControllerComponentVariables::NetworkConfigTimeout)
                .value_or(default_network_config_timeout_seconds);
        // Wait for the future until a value is set or the timeout has passed.
        std::future_status status = config_status.wait_for(std::chrono::seconds(config_timeout));
        switch (status) {
        case std::future_status::deferred:
        case std::future_status::timeout: {
            EVLOG_debug << "Timeout or deferred for config slot: " << config_slot_int;
            // Connect to next network configuration priority
            sleep(2);
            reconnect(config_slot_int, true);
            return false;
        }
        case std::future_status::ready: {
            ConfigNetworkResult result = config_status.get();
            if (result.success && result.network_profile_slot == config_slot_int) {
                EVLOG_debug << "Config slot " << config_slot_int << " is configured, try to connect";
                connection_options.iface_or_ip = result.interface_address;
                this->pending_network_slot = config_slot_int;
                this->requested_network_slot = 0;
            } else {
                EVLOG_debug << "Config slot " << config_slot_int << " is not configured, try again.";
                // No success, network could not be initialized?
                // Try again.
                reconnect(this->requested_network_slot, false);
                return false;
            }
            break;
        }
        }
    } else {
        // No callback configured, just connect to this profile.
        this->pending_network_slot = config_slot_int;
        this->requested_network_slot = 0;
    }

    config_slot_lock.unlock();

    this->current_connection_options = connection_options;
    this->connection_attempts = 0;
    if (this->websocket != nullptr) {
        this->websocket->disconnect(WebsocketCloseReason::ServiceRestart);
    }

    // TODO this sometimes let the application hang (a thread not closing or something???)
    this->websocket = nullptr;

    const auto& active_network_profile_cv = ControllerComponentVariables::ActiveNetworkProfile;
    if (active_network_profile_cv.variable.has_value()) {
        this->device_model.set_read_only_value(active_network_profile_cv.component,
                                               active_network_profile_cv.variable.value(), AttributeEnum::Actual,
                                               configuration_slot);
    }

    const auto& security_profile_cv = ControllerComponentVariables::SecurityProfile;
    if (security_profile_cv.variable.has_value()) {
        this->device_model.set_read_only_value(security_profile_cv.component, security_profile_cv.variable.value(),
                                                AttributeEnum::Actual,
                                                std::to_string(network_connection_profile.value().securityProfile));
    }

    this->websocket = std::make_unique<Websocket>(connection_options, this->evse_security, this->logging);
    this->websocket->register_connected_callback(
        [this, config_slot_int, network_connection_profile](const int security_profile) {
            this->on_websocket_connected_callback(config_slot_int, network_connection_profile);
        });

    this->websocket->register_closed_callback(
        [this, config_slot_int, network_connection_profile](const WebsocketCloseReason reason) {
            if (this->websocket == nullptr) {
                return;
            }

            this->on_websocket_closed_callback(config_slot_int, network_connection_profile, reason);
        });

    this->websocket->register_failed_callback(
        [this, config_slot_int, network_connection_profile](const WebsocketCloseReason reason) {
            this->on_websocket_failed_callback(config_slot_int, network_connection_profile, reason);
        });

    this->websocket->register_message_callback([this](const std::string& message) { this->message_callback(message); });

    return true;
}

WebsocketConnectionOptions ConnectivityManager::get_ws_connection_options(const int32_t configuration_slot) {
    const auto network_connection_profile_opt = this->get_network_connection_profile(configuration_slot);

    if (!network_connection_profile_opt.has_value()) {
        EVLOG_critical << "Could not retrieve NetworkProfile of configurationSlot: " << configuration_slot;
        throw std::runtime_error("Could not retrieve NetworkProfile");
    }

    const auto network_connection_profile = network_connection_profile_opt.value();

    auto uri = Uri::parse_and_validate(
        network_connection_profile.ocppCsmsUrl.get(),
        this->device_model.get_value<std::string>(ControllerComponentVariables::SecurityCtrlrIdentity),
        network_connection_profile.securityProfile);

    WebsocketConnectionOptions connection_options{
        OcppProtocolVersion::v201,
        uri,
        network_connection_profile.securityProfile,
        this->device_model.get_optional_value<std::string>(ControllerComponentVariables::BasicAuthPassword),
        this->device_model.get_value<int>(ControllerComponentVariables::RetryBackOffRandomRange),
        this->device_model.get_value<int>(ControllerComponentVariables::RetryBackOffRepeatTimes),
        this->device_model.get_value<int>(ControllerComponentVariables::RetryBackOffWaitMinimum),
        this->device_model.get_value<int>(ControllerComponentVariables::NetworkProfileConnectionAttempts),
        this->device_model.get_value<std::string>(ControllerComponentVariables::SupportedCiphers12),
        this->device_model.get_value<std::string>(ControllerComponentVariables::SupportedCiphers13),
        this->device_model.get_value<int>(ControllerComponentVariables::WebSocketPingInterval),
        this->device_model.get_optional_value<std::string>(ControllerComponentVariables::WebsocketPingPayload)
            .value_or("payload"),
        this->device_model.get_optional_value<int>(ControllerComponentVariables::WebsocketPongTimeout).value_or(5),
        this->device_model.get_optional_value<bool>(ControllerComponentVariables::UseSslDefaultVerifyPaths)
            .value_or(true),
        this->device_model.get_optional_value<bool>(ControllerComponentVariables::AdditionalRootCertificateCheck)
            .value_or(false),
        std::nullopt, // hostName
        true          // verify_csms_common_name
    };

    return connection_options;
}

std::optional<NetworkConnectionProfile>
ConnectivityManager::get_network_connection_profile(const int32_t configuration_slot) {
    std::vector<SetNetworkProfileRequest> network_connection_profiles =
        json::parse(this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConnectionProfiles));

    for (const auto& network_profile : network_connection_profiles) {
        if (network_profile.configurationSlot == configuration_slot) {
            auto security_profile = network_profile.connectionData.securityProfile;
            switch (security_profile) {
            case security::OCPP_1_6_ONLY_UNSECURED_TRANSPORT_WITHOUT_BASIC_AUTHENTICATION:
                throw std::invalid_argument("security_profile = " + std::to_string(security_profile) +
                                            " not officially allowed in OCPP 2.0.1");
            default:
                break;
            }

            return network_profile.connectionData;
        }
    }
    return std::nullopt;
}

int32_t ConnectivityManager::get_next_network_configuration_priority_slot(const int32_t configuration_slot) {
    const auto network_connection_priorities = ocpp::get_vector_from_csv(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
    if (network_connection_priorities.size() > 1) {
        EVLOG_info << "Switching to next network configuration priority";
    }

    const std::optional<int32_t> prio = this->get_configuration_slot_priority(configuration_slot);
    if (!prio.has_value()) {
        // TODO euh... there is no priority for this configuration slot, so is it invalid then?
        return 0;
    }

    const int32_t new_prio = (prio.value() + 1) % (static_cast<int32_t>(network_connection_priorities.size()));

    if (new_prio < 0) {
        return 0;
    }

    std::string new_prio_string = network_connection_priorities.at(static_cast<uint32_t>(new_prio));
    return std::stoi(new_prio_string);
}

void ConnectivityManager::on_websocket_connected_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile) {
    NetworkConnectionProfile profile;
    std::unique_lock<std::recursive_mutex> lock(this->config_slot_mutex);
    if (this->pending_network_slot != configuration_slot || !network_connection_profile.has_value()) {
        const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
            this->get_network_connection_profile(configuration_slot);
        if (!network_connection_profile_opt.has_value()) {
            EVLOG_error << "Could not find network connection profile that websocket is connected to.";
            // TODO what to do here???
            return;
        }

        profile = network_connection_profile_opt.value();
    } else {
        profile = network_connection_profile.value();
    }

    // Reset connection attempts
    this->connection_attempts = 0;
    // TODO something with calling ping? set ping interval etc?

    this->active_network_slot = configuration_slot;
    this->pending_network_slot = 0;

    lock.unlock();

    if (this->websocket_connected_callback.has_value()) {
        this->websocket_connected_callback.value()(configuration_slot, profile);
    }
}

void ConnectivityManager::on_websocket_closed_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile,
    const WebsocketCloseReason reason) {

    EVLOG_debug << "Websocket closed: " << configuration_slot;

    std::unique_lock<std::recursive_mutex> lock(this->config_slot_mutex);
    if (configuration_slot != this->get_active_network_configuration_slot()) {
        // Well this slot was already closed or something.
        return;
    }

    if (reason == WebsocketCloseReason::ServiceRestart || reason == WebsocketCloseReason::GoingAway) {
        EVLOG_debug << "Websocket closed, service restart or going away reason, do nothing...";
        // TODO Do nothing?? Because we did this ourselves in our own process so we will reconnect ourselves as well...
    } else if (reason != WebsocketCloseReason::Normal) {
        // TODO only call this when we did not initiate the close ourselves...
        reconnect(configuration_slot, false);
    } else {
        NetworkConnectionProfile profile;
        if (this->active_network_slot != configuration_slot || !network_connection_profile.has_value()) {
            const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
                this->get_network_connection_profile(configuration_slot);
            if (!network_connection_profile_opt.has_value()) {
                EVLOG_error << "Could not find network connection profile that websocket is connected to.";
                // TODO what to do here???
                return;
            }

            profile = network_connection_profile_opt.value();
        } else {
            profile = network_connection_profile.value();
        }

        if (this->websocket_disconnected_callback.has_value()) {
            this->websocket_disconnected_callback.value()(configuration_slot, profile);
        }

        // TODO only call this when we did not initiate the close ourselves... ???
        reconnect(configuration_slot, true);
    }
}

void ConnectivityManager::on_websocket_failed_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile,
    const WebsocketCloseReason reason) {

    EVLOG_debug << "Websocket failed, reason: " << static_cast<uint32_t>(reason);
    reconnect(configuration_slot, false);
}

std::optional<int32_t> ConnectivityManager::get_configuration_slot_priority(const int32_t configuration_slot) {
    // Convert to string as a vector of strings is used.
    const std::string configuration_slot_string = std::to_string(configuration_slot);

    const std::vector<std::string> network_connection_priorities = ocpp::get_vector_from_csv(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
    auto it = std::find(network_connection_priorities.begin(), network_connection_priorities.end(),
                        configuration_slot_string);
    if (it != network_connection_priorities.end()) {
        // Index is iterator - begin iterator
        return it - network_connection_priorities.begin();
    }
    return std::nullopt;
}

bool ConnectivityManager::is_higher_priority_profile(const int32_t new_configuration_slot) {

    const int32_t current_slot = get_active_network_configuration_slot();
    if (current_slot == 0) {
        // No slot in use, new is always higher priority.
        return true;
    }

    if (current_slot == new_configuration_slot) {
        // Slot is the same, probably already connected
        return false;
    }

    const std::optional<int32_t> new_priority = get_configuration_slot_priority(new_configuration_slot);
    if (!new_priority.has_value()) {
        // Slot not found.
        return false;
    }

    const std::optional<int32_t> current_priority = get_configuration_slot_priority(current_slot);
    if (!current_priority.has_value()) {
        // Slot not found.
        return false;
    }

    if (new_configuration_slot < current_slot) {
        // Priority is indeed higher (lower index means higher priority)
        return true;
    }

    return false;
}

void ConnectivityManager::set_retry_connection_timer(const std::chrono::milliseconds timeout) {
    EVLOG_info << "Trying to reconnect in " << timeout.count() / 1000 << " seconds";
    this->reconnect_timer.timeout(
        [this]() {
            EVLOG_debug << "Timer timed out, reconnecting.";
            std::unique_lock<std::mutex> lock(this->reconnect_mutex);
            // Notify main thread that it should reconnect.
            this->try_reconnect.store(true);
            this->reconnect_condition_variable.notify_all();
        },
        timeout);
}

std::chrono::milliseconds ConnectivityManager::get_reconnect_interval() {
    // We need to add 1 to the repeat times since the first try is already connection_attempt 1
    if (this->connection_attempts > (this->current_connection_options.retry_backoff_repeat_times + 1)) {
        return this->reconnect_backoff_ms;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, this->current_connection_options.retry_backoff_random_range_s);

    int random_number = distr(gen);

    if (this->connection_attempts == 1) {
        this->reconnect_backoff_ms = std::chrono::milliseconds(
            (this->current_connection_options.retry_backoff_wait_minimum_s + random_number) * 1000);
        return this->reconnect_backoff_ms;
    }

    this->reconnect_backoff_ms = this->reconnect_backoff_ms * 2 + std::chrono::milliseconds(random_number * 1000);
    return this->reconnect_backoff_ms;
}

void ConnectivityManager::reconnect(const int32_t configuration_slot, const bool next_profile) {
    if (!running) {
        EVLOG_debug << "Not reconnecting, because connectivity manager stopped running.";
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
        return;
    }

    if (!next_profile && (this->current_connection_options.max_connection_attempts == -1 ||
                          this->connection_attempts <= this->current_connection_options.max_connection_attempts)) {
        std::unique_lock<std::recursive_mutex> lock(this->config_slot_mutex);
        if (configuration_slot == this->active_network_slot || configuration_slot == this->pending_network_slot) {
            if (this->active_network_slot != 0) {
                // Set pending network slot to current active network slot.
                // TODO is that correct? Or should we do that somewhere else than here?
                this->pending_network_slot = this->active_network_slot;
            }

            this->active_network_slot = 0;
            this->connection_attempts += 1;
            set_retry_connection_timer(get_reconnect_interval());
        } else if (configuration_slot == this->requested_network_slot) {
            // Probably in the state to get information from the system if the requested network is up.
            this->connection_attempts += 1;
            set_retry_connection_timer(get_reconnect_interval());
        } else {
            // TODO what to do here? Think of different scenario's when 'reconnected' is called.
        }
    } else {
        if (this->websocket_disconnected_callback.has_value()) {
            std::optional<NetworkConnectionProfile> disconnected_profile =
                this->get_network_connection_profile(configuration_slot);
            if (disconnected_profile.has_value()) {
                this->websocket_disconnected_callback.value()(
                    configuration_slot, get_network_connection_profile(configuration_slot).value());
            }
        }
        std::unique_lock<std::recursive_mutex> config_slot_lock(this->config_slot_mutex);
        this->requested_network_slot = this->get_next_network_configuration_priority_slot(configuration_slot);
        EVLOG_info << "Connect with next network configuration priority slot: " << this->requested_network_slot;
        this->connection_attempts = 0;
        this->active_network_slot = 0;
        this->pending_network_slot = 0;
        config_slot_lock.unlock();
        std::unique_lock<std::mutex> lock(this->reconnect_mutex);
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
    }
}

} // namespace v201
} // namespace ocpp
