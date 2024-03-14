#include "ocpp/v201/messages/SetNetworkProfile.hpp"
#include <everest/logging.hpp>
#include <ocpp/v201/connectivity_manager.h>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>

namespace ocpp {
namespace v201 {

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
    reconnect_condition_variable(),
    device_model(device_model),
    evse_security(evse_security),
    logging(logging),
    disable_automatic_websocket_reconnects(false),
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

void ConnectivityManager::set_websocket_connected_callback(
    std::function<void(const int, const NetworkConnectionProfile&)> websocket_connected_callback) {
    this->websocket_connected_callback = websocket_connected_callback;
}

void ConnectivityManager::set_websocket_disconnected_callback(
    std::function<void(const int, const NetworkConnectionProfile&)> websocket_disconnected_callback) {
    this->websocket_disconnected_callback = websocket_disconnected_callback;
}

void ConnectivityManager::set_configure_network_connection_profile_callback(
    std::function<std::future<ConfigNetworkResult>(const int32_t, const NetworkConnectionProfile&)>
        configure_network_connection_profile_callback) {
    this->configure_network_connection_profile_callback = configure_network_connection_profile_callback;
}

void ConnectivityManager::start() {
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
        running = false;
        reconnect_timer.stop();
        disconnect_websocket(WebsocketCloseReason::Normal);
        this->websocket = nullptr;
        reconnect_condition_variable.notify_all();
        if (connectivity_thread.joinable()) {
            connectivity_thread.join();
        }
    }
}

void ConnectivityManager::connect_websocket(std::optional<int32_t> config_slot) {
    if (this->websocket == nullptr) {
        return;
    }
    // TODO check if websocket is connected???
    if (!this->websocket->is_connected()) {
        this->init_websocket(config_slot);
        this->websocket->connect();
    }
}

void ConnectivityManager::disconnect_websocket(WebsocketCloseReason code) {
    if (this->websocket == nullptr) {
        return;
    }

    if (this->websocket->is_connected()) {
        // TODO set this???
        this->websocket->disconnect(code);
    }
}

bool ConnectivityManager::on_try_switch_network_connection_profile(const int32_t configuration_slot) {
    if (is_higher_priority_profile(configuration_slot)) {
        // Priority is indeed higher
        EVLOG_debug
            << "Trying to connect with higher priority network connection profile"; // TODO add from and to slots
    } else {
        return false;
    }

    const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
        this->get_network_connection_profile(configuration_slot);
    if (!network_connection_profile_opt.has_value()) {
        return false;
    }

    try {
        // Stop reconnect timer, if it is running -> we will reconnect with another websocket.
        reconnect_timer.stop();

        std::unique_lock<std::mutex> lock(this->config_slot_mutex);
        this->disconnect_websocket(); // normal close
        this->active_network_slot = 0;
        this->pending_network_slot = 0;
        this->requested_network_slot = configuration_slot;

        // Notify main thread that it should reconnect.
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
        EVLOG_info << "Not clear which network is disconnected: configuration slot and ocpp interface are empty";
        return;
    }

    EVLOG_debug << "Network disconnected: " << configuration_slot.value();

    int32_t disconnected_network_slot = 0;

    // Find slot belonging to ocpp interface. That's only interesting if it is an active or pending network slot.
    const int32_t current_network_slot =
        this->active_network_slot != 0 ? this->active_network_slot : this->pending_network_slot;

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
        std::unique_lock<std::mutex> lock(this->config_slot_mutex);
        this->disconnect_websocket(WebsocketCloseReason::Normal); // normal close???

        // TODO should I set the websocket to nullptr? Otherwise it is reconnecting later and websocketpp crashes with
        // 'invalid state'. But now the application crashes
        this->websocket = nullptr;
        this->requested_network_slot = this->get_next_network_configuration_priority_slot(configuration_slot.value());
        this->active_network_slot = 0;
        this->pending_network_slot = 0;

        // Notify main thread that it should reconnect.
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
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
    current_connection_options = connection_options;
    if (this->websocket != nullptr) {
        this->websocket->set_connection_options(connection_options);
    }
}

void ConnectivityManager::run() {
    EVLOG_debug << "Run! Requested network slot: " << this->requested_network_slot
                << ", active network slot: " << this->active_network_slot
                << ", pending network slot: " << this->pending_network_slot;
    if (this->requested_network_slot != 0) {
        this->try_reconnect.store(false);
        this->init_websocket(this->requested_network_slot);
        if (this->websocket != nullptr) {
            this->websocket->connect();
        }
    } else if (this->active_network_slot == 0 && this->pending_network_slot == 0) {
        this->try_reconnect.store(false);
        this->init_websocket();
        if (this->websocket != nullptr) {
            this->websocket->connect();
        }
    } else if (this->pending_network_slot != 0) {
        this->try_reconnect.store(false);
        // There is a network slot pending, maybe because it wants to try a reconnect, try to connect the websocket now.
        if (this->websocket != nullptr) {
            this->websocket->connect();
        } else {
            this->init_websocket(this->pending_network_slot);
            if (this->websocket != nullptr) {
                this->websocket->connect();
            }
        }
    }

    // Wait until a reconnect is requested for whatever reason.
    std::unique_lock<std::mutex> lock(config_slot_mutex);
    // Return when 'running' is set to false (we want to stop the thread) or 'try_reconnect' is true (we want to
    // reconnect to the websocket).
    EVLOG_debug << "Wait!! Try reconnect = " << (try_reconnect.load() ? "true" : "false");
    reconnect_condition_variable.wait(lock, [this] { return !this->running.load() || this->try_reconnect.load(); });
    // At this point we want to re-run this function. Since 'run' is called in a while loop, it will be called again
    // directly after returning this function.
    EVLOG_debug << "After wait!";
}

void ConnectivityManager::init_websocket(std::optional<int32_t> config_slot) {
    std::string configuration_slot;
    if (this->device_model.get_value<std::string>(ControllerComponentVariables::ChargePointId).find(':') !=
        std::string::npos) {
        EVLOG_AND_THROW(std::runtime_error("ChargePointId must not contain \':\'"));
    }

    int32_t config_slot_int;
    if (config_slot.has_value()) {
        EVLOG_info << "init websocket using configuration slot ---> " << config_slot.value();
        configuration_slot = std::to_string(config_slot.value());
        config_slot_int = config_slot.value();
    } else {
        EVLOG_debug << "Init websocket without config slot, use first available";
        // Get first available config slot.
        const std::vector<std::string> config_slot_vector = ocpp::get_vector_from_csv(
            this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
        if (config_slot_vector.size() > 0) {
            configuration_slot = config_slot_vector.at(0);
            config_slot_int = std::stoi(configuration_slot);
            EVLOG_debug << "First available is: " << config_slot_int;
        } else {
            // No network connection profile. Retry connecting after some time, maybe it is set manually.
            sleep(default_retry_network_connection_profile_seconds);
            return;
        }
    }

    WebsocketConnectionOptions connection_options = this->get_ws_connection_options(config_slot_int);
    const std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(config_slot_int);

    if (!network_connection_profile.has_value()) {
        // No network connection profile found, what to connect to then? So get next profile and try to connect with
        // that one.
        this->requested_network_slot = this->get_next_network_configuration_priority_slot(config_slot_int);
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
        return;
    }

    if (this->configure_network_connection_profile_callback.has_value()) {
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
            std::unique_lock<std::mutex> lock(this->config_slot_mutex);
            this->requested_network_slot = this->get_next_network_configuration_priority_slot(config_slot_int);
            this->active_network_slot = 0;
            this->pending_network_slot = 0;
            sleep(2);

            // Notify main thread that it should reconnect.
            this->try_reconnect.store(true);
            this->reconnect_condition_variable.notify_all();
            // TODO: in the old code there was a timeout for this. Should we do that???
            // TODO check if this works ok, etc
            return;
        }
        case std::future_status::ready: {
            EVLOG_debug << "Ready for config slot: " << config_slot_int;
            ConfigNetworkResult result = config_status.get();
            if (result.success) {
                std::unique_lock<std::mutex> lock(this->config_slot_mutex);

                EVLOG_debug << "Ready for config slot: " << config_slot_int << " and result.success = true";
                connection_options.iface_or_ip = result.interface_address;
                this->pending_network_slot = config_slot_int;
                this->requested_network_slot = 0;
            } else {
                EVLOG_debug << "Ready for config slot: " << config_slot_int << " and result.success = false";
                // No success, network could not be initialized?
                // Connect to next network configuration priority
                std::unique_lock<std::mutex> lock(this->config_slot_mutex);
                this->requested_network_slot = this->get_next_network_configuration_priority_slot(config_slot_int);
                EVLOG_debug << "Requested network slot: " << this->requested_network_slot;
                this->active_network_slot = 0;
                this->pending_network_slot = 0;

                // Notify main thread that it should reconnect.
                sleep(2);
                EVLOG_debug << "Try reconnect store set to true";
                this->try_reconnect.store(true);
                EVLOG_debug << "reconnect condition variable notify";
                this->reconnect_condition_variable.notify_all();
                EVLOG_debug << "after reconnect condition variable notify. Running = "
                            << (running.load() ? "true" : "false")
                            << ", try_reconnect = " << (try_reconnect.load() ? "true" : "false");
                // TODO: in the old code there was a timeout for this. Should we do that???
                // TODO check if this works ok, etc
                return;
            }
            break;
        }
        }
    } else {
        // No callback configured, just connect to this profile.
        this->requested_network_slot = 0;
    }

    this->current_connection_options = connection_options;
    this->connection_attempts = 0;
    this->reconnect_backoff_ms = std::chrono::seconds(0);
    this->websocket = std::make_unique<Websocket>(connection_options, this->evse_security, this->logging);
    this->websocket->register_connected_callback([this, network_connection_profile](const int configuration_slot) {
        this->on_websocket_connected_callback(configuration_slot, network_connection_profile);
    });

    this->websocket->register_disconnected_callback([this, config_slot_int, network_connection_profile]() {
        this->on_websocket_disconnected_callback(config_slot_int, network_connection_profile);
    });

    this->websocket->register_closed_callback(
        [this, config_slot_int, network_connection_profile](const WebsocketCloseReason reason) {
            this->on_websocket_closed_callback(config_slot_int, network_connection_profile, reason);
        });

    this->websocket->register_failed_callback(
        [this, config_slot_int, network_connection_profile](const WebsocketCloseReason reason) {
            this->on_websocket_failed_callback(config_slot_int, network_connection_profile, reason);
        });

    this->websocket->register_message_callback([this](const std::string& message) { this->message_callback(message); });
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

void ConnectivityManager::remove_network_connection_profiles_below_actual_security_profile() {
    // Remove all the profiles that are a lower security level than security_level
    const auto security_level = this->device_model.get_value<int>(ControllerComponentVariables::SecurityProfile);

    auto network_connection_profiles =
        json::parse(this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConnectionProfiles));

    auto is_lower_security_level = [security_level](const SetNetworkProfileRequest& item) {
        return item.connectionData.securityProfile < security_level;
    };

    network_connection_profiles.erase(
        std::remove_if(network_connection_profiles.begin(), network_connection_profiles.end(), is_lower_security_level),
        network_connection_profiles.end());

    this->device_model.set_value(ControllerComponentVariables::NetworkConnectionProfiles.component,
                                 ControllerComponentVariables::NetworkConnectionProfiles.variable.value(),
                                 AttributeEnum::Actual, network_connection_profiles.dump());

    // Update the NetworkConfigurationPriority so only remaining profiles are in there
    const auto network_priority = ocpp::get_vector_from_csv(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));

    auto in_network_profiles = [&network_connection_profiles](const std::string& item) {
        auto is_same_slot = [&item](const SetNetworkProfileRequest& profile) {
            return std::to_string(profile.configurationSlot) == item;
        };
        return std::any_of(network_connection_profiles.begin(), network_connection_profiles.end(), is_same_slot);
    };

    std::string new_network_priority;
    for (const auto& item : network_priority) {
        if (in_network_profiles(item)) {
            if (!new_network_priority.empty()) {
                new_network_priority += ',';
            }
            new_network_priority += item;
        }
    }

    this->device_model.set_value(ControllerComponentVariables::NetworkConfigurationPriority.component,
                                 ControllerComponentVariables::NetworkConfigurationPriority.variable.value(),
                                 AttributeEnum::Actual, new_network_priority);
}

void ConnectivityManager::on_websocket_connected_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile) {
    NetworkConnectionProfile profile;
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
    this->connection_attempts = 1;
    // TODO something with calling ping? set ping interval etc?

    {
        std::unique_lock<std::mutex> lock(this->config_slot_mutex);

        this->active_network_slot = configuration_slot;
        this->pending_network_slot = 0;
    }

    if (this->websocket_connected_callback.has_value()) {
        this->websocket_connected_callback.value()(this->active_network_slot, profile);
    }
}

// void ConnectivityManager::on_websocket_disconnected_callback(
//     const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile) {

//     EVLOG_info << "Websocket disconnected: " << configuration_slot;

//     NetworkConnectionProfile profile;
//     if (this->active_network_slot != configuration_slot || !network_connection_profile.has_value()) {
//         const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
//             this->get_network_connection_profile(configuration_slot);
//         if (!network_connection_profile_opt.has_value()) {
//             EVLOG_error << "Could not find network connection profile that websocket is connected to.";
//             // TODO what to do here??? Pending = active, active = 0???
//             return;
//         }

//         profile = network_connection_profile_opt.value();
//     } else {
//         profile = network_connection_profile.value();
//     }

//     // TODO the whole reconnect timeout stuff. When to reconnect and when to connect to new slot and when to wait for
//     // a timeout and when not?
//     reconnect(configuration_slot);

//     // TODO what to do here? Get next available profile? Call init? Set indeed pending_network_slot? ???

//     if (this->websocket_disconnected_callback.has_value()) {
//         // TODO in this callback, the interface should not yet been closed. That should be done in the 'closed'
//         // callback??? Add websocket_closed_callback as well then (for external sources)???
//         this->websocket_disconnected_callback.value()(this->active_network_slot, profile);
//     }
// }

void ConnectivityManager::on_websocket_closed_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile,
    const WebsocketCloseReason reason) {

    // TODO when is 'closed' exactly called? After max retries?
    EVLOG_info << "Websocket closed: " << configuration_slot;
    // TODO there was a timout here, put it back???
    if (reason != WebsocketCloseReason::Normal) {
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

        reconnect(configuration_slot, true);
    }

    // TODO why this if for service restart???
    // if (reason != WebsocketCloseReason::ServiceRestart) {
    //     std::unique_lock<std::mutex> lock(this->config_slot_mutex);
    //     // TODO what if requested_network_slot is now also 0???
    //     this->requested_network_slot = this->get_next_network_configuration_priority_slot(configuration_slot);
    //     this->active_network_slot = 0;
    //     this->pending_network_slot = 0;

    //     EVLOG_info << "next network config ---->";
    // }

    EVLOG_info << "start websocket after close---->";
    // this->start();
}

void ConnectivityManager::on_websocket_failed_callback(
    const int configuration_slot, const std::optional<NetworkConnectionProfile> network_connection_profile,
    const WebsocketCloseReason reason) {

    reconnect(configuration_slot, false);
}

int32_t ConnectivityManager::get_active_network_configuration_slot() const {
    std::unique_lock<std::mutex> lock(this->config_slot_mutex);
    return (active_network_slot != 0 ? active_network_slot : pending_network_slot);
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

void ConnectivityManager::set_retry_connection_timer(const std::chrono::seconds timeout) {
    this->reconnect_timer.timeout(
        [this]() {
            std::unique_lock<std::mutex> lock(this->config_slot_mutex);
            // Notify main thread that it should reconnect.
            this->try_reconnect.store(true);
            this->reconnect_condition_variable.notify_all();
        },
        timeout);
}

std::chrono::seconds ConnectivityManager::get_reconnect_interval() {
    // We need to add 1 to the repeat times since the first try is already connection_attempt 1
    if (this->connection_attempts > (this->current_connection_options.retry_backoff_repeat_times + 1)) {
        return this->reconnect_backoff_ms;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, this->current_connection_options.retry_backoff_random_range_s);

    int random_number = distr(gen);

    if (this->connection_attempts == 1) {
        this->reconnect_backoff_ms = std::chrono::seconds(
            (this->current_connection_options.retry_backoff_wait_minimum_s + random_number) * 1000);
        return this->reconnect_backoff_ms;
    }

    this->reconnect_backoff_ms = this->reconnect_backoff_ms * 2 + std::chrono::seconds(random_number * 1000);
    return this->reconnect_backoff_ms;
}

void ConnectivityManager::reconnect(const int32_t configuration_slot, const bool next_profile) {
    if (!next_profile || (this->current_connection_options.max_connection_attempts == -1 ||
                          this->connection_attempts <= this->current_connection_options.max_connection_attempts)) {
        std::unique_lock<std::mutex> lock(this->config_slot_mutex);
        if (configuration_slot == this->active_network_slot) {
            this->pending_network_slot = this->active_network_slot;
            this->active_network_slot = 0;
            this->connection_attempts += 1;
            set_retry_connection_timer(get_reconnect_interval());
        } else {
            // TODO what to do here? Think of different scenario's when 'reconnect' is called.
        }
    } else {
        // Try next profile.
        // First remove current websocket.
        this->websocket = nullptr;
        // And call disconnected callback.
        if (this->websocket_disconnected_callback.has_value()) {
            std::optional<NetworkConnectionProfile> disconnected_profile =
                this->get_network_connection_profile(configuration_slot);
            if (disconnected_profile.has_value()) {
                this->websocket_disconnected_callback.value()(
                    configuration_slot, get_network_connection_profile(configuration_slot).value());
            }
        }
        this->requested_network_slot = this->get_next_network_configuration_priority_slot(configuration_slot);
        this->connection_attempts = 1;
        this->active_network_slot = 0;
        this->pending_network_slot = 0;
        this->try_reconnect.store(true);
        this->reconnect_condition_variable.notify_all();
    }
}

} // namespace v201
} // namespace ocpp
