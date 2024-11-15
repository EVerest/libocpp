// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/connectivity_manager.hpp>

#include <everest/logging.hpp>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>

namespace {
const auto WEBSOCKET_INIT_DELAY = std::chrono::seconds(2);
const std::string VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL = "internal";
/// \brief Default timeout for the return value (future) of the `configure_network_connection_profile_callback`
///        function.
constexpr int32_t default_network_config_timeout_seconds = 60;
} // namespace

namespace ocpp {
namespace v201 {

ConnectivityManager::ConnectivityManager(DeviceModel& device_model, std::shared_ptr<EvseSecurity> evse_security,
                                         std::shared_ptr<MessageLogging> logging,
                                         const std::function<void(const std::string& message)>& message_callback) :
    device_model{device_model},
    evse_security{evse_security},
    logging{logging},
    websocket{nullptr},
    message_callback{message_callback},
    disconnect_triggered{false},
    active_network_configuration_priority{0} {
    cache_network_connection_profiles();
}

void ConnectivityManager::set_websocket_authorization_key(const std::string& authorization_key) {
    if (this->websocket != nullptr) {
        this->websocket->set_authorization_key(authorization_key);
        this->websocket->disconnect(WebsocketCloseReason::ServiceRestart);
    }
}

void ConnectivityManager::set_websocket_connection_options(const WebsocketConnectionOptions& connection_options) {
    if (this->websocket != nullptr) {
        this->websocket->set_connection_options(connection_options);
    }
}

void ConnectivityManager::set_websocket_connection_options_without_reconnect() {
    const int configuration_slot = get_active_network_configuration_slot();
    const auto connection_options = this->get_ws_connection_options(configuration_slot);
    if (connection_options.has_value()) {
        this->set_websocket_connection_options(connection_options.value());
    }
}

void ConnectivityManager::set_websocket_connected_callback(WebsocketConnectionCallback callback) {
    this->websocket_connected_callback = callback;
}

void ConnectivityManager::set_websocket_disconnected_callback(WebsocketConnectionCallback callback) {
    this->websocket_disconnected_callback = callback;
}

void ConnectivityManager::set_websocket_connection_failed_callback(WebsocketConnectionFailedCallback callback) {
    this->websocket_connection_failed_callback = callback;
}

void ConnectivityManager::set_configure_network_connection_profile_callback(
    ConfigureNetworkConnectionProfileCallback callback) {
    this->configure_network_connection_profile_callback = callback;
}

std::optional<NetworkConnectionProfile>
ConnectivityManager::get_network_connection_profile(const int32_t configuration_slot) {

    for (const auto& network_profile : this->cached_network_connection_profiles) {
        if (network_profile.configurationSlot == configuration_slot) {
            switch (auto security_profile = network_profile.connectionData.securityProfile) {
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

std::optional<int32_t> ConnectivityManager::get_priority_from_configuration_slot(const int configuration_slot) {
    auto it =
        std::find(this->network_connection_slots.begin(), this->network_connection_slots.end(), configuration_slot);
    if (it != network_connection_slots.end()) {
        // Index is iterator - begin iterator
        return it - network_connection_slots.begin();
    }
    return std::nullopt;
}

int ConnectivityManager::get_active_network_configuration_slot() {
    return this->network_connection_slots.at(this->active_network_configuration_priority);
}

int ConnectivityManager::get_configuration_slot_from_priority(const int priority) {
    return this->network_connection_slots.at(priority);
}

const std::vector<int>& ConnectivityManager::get_network_connection_slots() const {
    return this->network_connection_slots;
}

bool ConnectivityManager::is_websocket_connected() {
    return this->websocket != nullptr && this->websocket->is_connected();
}

void ConnectivityManager::connect(std::optional<int32_t> configuration_slot_opt) {
    const int32_t configuration_slot = configuration_slot_opt.value_or(1);
    const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
        this->get_network_connection_profile(configuration_slot);
    if (!network_connection_profile_opt.has_value()) {
        EVLOG_warning << "Could not find network connection profile belonging to configuration slot "
                      << configuration_slot;
        return;
    }
    this->pending_configuration_slot = configuration_slot;
    if (this->is_websocket_connected()) {
        this->websocket->disconnect(WebsocketCloseReason::ServiceRestart);
    } else {
        this->try_connect_websocket();
    }
}

void ConnectivityManager::check_cache_for_invalid_security_profiles() {
    const auto security_level = this->device_model.get_value<int>(ControllerComponentVariables::SecurityProfile);

    if (this->last_security_level == security_level) {
        return;
    }
    this->last_security_level = security_level;

    auto before_slot = this->pending_configuration_slot.value_or(this->get_active_network_configuration_slot());

    EVLOG_info << "Before cleanup";
    for (auto slot : this->network_connection_slots) {
        EVLOG_info << "Slot " << slot << " sec level: " << this->get_network_connection_profile(slot)->securityProfile;
    }

    auto is_lower_security_level = [this, security_level](const int slot) {
        const auto opt_profile = this->get_network_connection_profile(slot);
        return !opt_profile.has_value() || opt_profile->securityProfile < security_level;
    };

    this->network_connection_slots.erase(std::remove_if(this->network_connection_slots.begin(),
                                                        this->network_connection_slots.end(), is_lower_security_level),
                                         this->network_connection_slots.end());

    EVLOG_info << "After cleanup";
    for (auto slot : this->network_connection_slots) {
        EVLOG_info << "Slot " << slot << " sec level: " << this->get_network_connection_profile(slot)->securityProfile;
    }

    auto opt_priority = this->get_priority_from_configuration_slot(before_slot);
    if (opt_priority) {
        this->pending_configuration_slot = before_slot;
    } else {
        this->pending_configuration_slot = this->get_next_configuration_slot(before_slot);
    }
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
                                 AttributeEnum::Actual, network_connection_profiles.dump(),
                                 VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL);

    // Update the NetworkConfigurationPriority so only remaining profiles are in there
    const auto network_priority = ocpp::split_string(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority), ',');

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
                                 AttributeEnum::Actual, new_network_priority, VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL);
}

void ConnectivityManager::confirm_successfull_connection() {
    const int config_slot_int = this->get_active_network_configuration_slot();

    const auto network_connection_profile = this->get_network_connection_profile(config_slot_int);

    if (const auto& security_profile_cv = ControllerComponentVariables::SecurityProfile;
        security_profile_cv.variable.has_value()) {
        this->device_model.set_read_only_value(security_profile_cv.component, security_profile_cv.variable.value(),
                                               AttributeEnum::Actual,
                                               std::to_string(network_connection_profile.value().securityProfile),
                                               VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL);
    }

    this->remove_network_connection_profiles_below_actual_security_profile();
    this->check_cache_for_invalid_security_profiles();
}

void ConnectivityManager::try_connect_websocket() {

    this->disconnect_triggered = false;

    if (this->device_model.get_value<std::string>(ControllerComponentVariables::ChargePointId).find(':') !=
        std::string::npos) {
        EVLOG_AND_THROW(std::runtime_error("ChargePointId must not contain \':\'"));
    }

    if (this->network_connection_slots.empty()) {
        EVLOG_warning << "No network connection profiles configured, aborting websocket connection.";
        return;
    }

    // Check the cache runtime since security profile might change async
    this->check_cache_for_invalid_security_profiles();

    const int configuration_slot_to_set =
        this->pending_configuration_slot.value_or(this->get_active_network_configuration_slot());
    const auto network_connection_profile = this->get_network_connection_profile(configuration_slot_to_set);
    // Not const as the iface member can be set by the configure network connection profile callback
    auto connection_options = this->get_ws_connection_options(configuration_slot_to_set);
    bool can_use_connection_profile = true;

    if (!network_connection_profile.has_value()) {
        EVLOG_warning << "No network connection profile configured for " << configuration_slot_to_set;
        can_use_connection_profile = false;
    } else if (!connection_options.has_value()) {
        EVLOG_warning << "Connection profile configured for " << configuration_slot_to_set << " failed: not valid URL";
        can_use_connection_profile = false;
    } else if (this->configure_network_connection_profile_callback.has_value()) {
        EVLOG_debug << "Request to configure network connection profile " << configuration_slot_to_set;

        std::future<ConfigNetworkResult> config_status = this->configure_network_connection_profile_callback.value()(
            configuration_slot_to_set, network_connection_profile.value());
        const int32_t config_timeout =
            this->device_model.get_optional_value<int>(ControllerComponentVariables::NetworkConfigTimeout)
                .value_or(default_network_config_timeout_seconds);

        std::future_status status = config_status.wait_for(std::chrono::seconds(config_timeout));

        switch (status) {
        case std::future_status::deferred:
        case std::future_status::timeout: {
            EVLOG_warning << "Timeout configuring config slot: " << configuration_slot_to_set;
            can_use_connection_profile = false;
            break;
        }
        case std::future_status::ready: {
            ConfigNetworkResult result = config_status.get();
            if (result.success and result.network_profile_slot == configuration_slot_to_set) {
                EVLOG_debug << "Config slot " << configuration_slot_to_set << " is configured";
                // Set interface or ip to connection options.
                connection_options->iface = result.interface_address;
            } else {
                EVLOG_warning << "Could not configure config slot " << configuration_slot_to_set;
                can_use_connection_profile = false;
            }
            break;
        }
        }
    }

    if (!can_use_connection_profile) {
        if (!this->disconnect_triggered) {
            this->websocket_timer.timeout(
                [this, configuration_slot_to_set] {
                    this->pending_configuration_slot = get_next_configuration_slot(configuration_slot_to_set);
                    this->try_connect_websocket();
                },
                WEBSOCKET_INIT_DELAY);
        }
        return;
    }

    this->pending_configuration_slot.reset();
    this->active_network_configuration_priority =
        get_priority_from_configuration_slot(configuration_slot_to_set).value();

    EVLOG_info << "Open websocket with NetworkConfigurationPriority: "
               << this->active_network_configuration_priority + 1 << " which is configurationSlot "
               << configuration_slot_to_set;

    if (const auto& active_network_profile_cv = ControllerComponentVariables::ActiveNetworkProfile;
        active_network_profile_cv.variable.has_value()) {
        this->device_model.set_read_only_value(
            active_network_profile_cv.component, active_network_profile_cv.variable.value(), AttributeEnum::Actual,
            std::to_string(configuration_slot_to_set), VARIABLE_ATTRIBUTE_VALUE_SOURCE_INTERNAL);
    }

    if (this->websocket == nullptr) {
        this->websocket = std::make_unique<Websocket>(connection_options.value(), this->evse_security, this->logging);

        this->websocket->register_connected_callback(
            std::bind(&ConnectivityManager::on_websocket_connected, this, std::placeholders::_1));
        this->websocket->register_disconnected_callback(
            std::bind(&ConnectivityManager::on_websocket_disconnected, this));
        this->websocket->register_closed_callback(
            std::bind(&ConnectivityManager::on_websocket_closed, this, std::placeholders::_1));
    } else {
        this->websocket->set_connection_options(connection_options.value());
    }

    // Attach external callbacks everytime since they might have changed
    if (websocket_connection_failed_callback.has_value()) {
        this->websocket->register_connection_failed_callback(websocket_connection_failed_callback.value());
    }

    this->websocket->register_message_callback([this](const std::string& message) { this->message_callback(message); });

    this->websocket->connect();
}

void ConnectivityManager::disconnect() {
    this->websocket_timer.stop();
    if (this->websocket != nullptr) {
        this->disconnect_triggered = true;
        this->websocket->disconnect(WebsocketCloseReason::Normal);
    }
}

int ConnectivityManager::get_next_configuration_slot(int32_t configuration_slot) {

    if (this->network_connection_slots.size() > 1) {
        EVLOG_info << "Switching to next network configuration priority";
    }
    const auto network_configuration_priority_opt = get_priority_from_configuration_slot(configuration_slot);

    const int network_configuration_priority =
        network_configuration_priority_opt.has_value()
            ? (network_configuration_priority_opt.value() + 1) % (this->network_connection_slots.size())
            : 0;

    return get_configuration_slot_from_priority(network_configuration_priority);
}

bool ConnectivityManager::send_to_websocket(const std::string& message) {
    if (this->websocket == nullptr) {
        return false;
    }

    return this->websocket->send(message);
}

void ConnectivityManager::on_network_disconnected(OCPPInterfaceEnum ocpp_interface) {

    const int actual_configuration_slot = get_active_network_configuration_slot();
    std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(actual_configuration_slot);

    if (!network_connection_profile.has_value()) {
        EVLOG_warning << "Network disconnected. No network connection profile configured";
    } else if (ocpp_interface == network_connection_profile.value().ocppInterface) {
        // Since there is no connection anymore: disconnect the websocket, the manager will try to connect with the next
        // available network connection profile as we enable reconnects.
        EVLOG_info << "ConnectivityManager::on_network_disconnected";
        this->websocket->disconnect(ocpp::WebsocketCloseReason::GoingAway);
    }
}

void ConnectivityManager::on_reconfiguration_of_security_parameters() {
    if (this->websocket != nullptr) {
        this->websocket->disconnect(WebsocketCloseReason::ServiceRestart);
    }
}

std::optional<WebsocketConnectionOptions>
ConnectivityManager::get_ws_connection_options(const int32_t configuration_slot) {
    const auto network_connection_profile_opt = this->get_network_connection_profile(configuration_slot);

    if (!network_connection_profile_opt.has_value()) {
        EVLOG_critical << "Could not retrieve NetworkProfile of configurationSlot: " << configuration_slot;
        throw std::runtime_error("Could not retrieve NetworkProfile");
    }

    const auto network_connection_profile = network_connection_profile_opt.value();

    try {
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
            this->device_model.get_optional_value<bool>(ControllerComponentVariables::VerifyCsmsCommonName)
                .value_or(true),
            this->device_model.get_optional_value<bool>(ControllerComponentVariables::UseTPM).value_or(false),
            this->device_model.get_optional_value<bool>(ControllerComponentVariables::VerifyCsmsAllowWildcards)
                .value_or(false),
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::IFace),
            this->device_model.get_optional_value<bool>(ControllerComponentVariables::EnableTLSKeylog).value_or(false),
            this->device_model.get_optional_value<std::string>(ControllerComponentVariables::TLSKeylogFile)};

        return connection_options;

    } catch (const std::invalid_argument& e) {
        EVLOG_error << "Could not configure the connection options: " << e.what();
    }

    return std::nullopt;
}

void ConnectivityManager::on_websocket_connected([[maybe_unused]] int security_profile) {
    const int actual_configuration_slot = get_active_network_configuration_slot();
    std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(actual_configuration_slot);

    if (this->websocket_connected_callback.has_value() and network_connection_profile.has_value()) {
        this->websocket_connected_callback.value()(actual_configuration_slot, network_connection_profile.value());
    }
}

void ConnectivityManager::on_websocket_disconnected() {
    std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(this->get_active_network_configuration_slot());

    if (this->websocket_disconnected_callback.has_value() and network_connection_profile.has_value()) {
        this->websocket_disconnected_callback.value()(this->get_active_network_configuration_slot(),
                                                      network_connection_profile.value());
    }
}

void ConnectivityManager::on_websocket_closed(ocpp::WebsocketCloseReason reason) {
    EVLOG_warning << "Closed websocket of NetworkConfigurationPriority: "
                  << this->active_network_configuration_priority + 1 << " which is configurationSlot "
                  << this->get_active_network_configuration_slot();

    if (!this->disconnect_triggered) {
        this->websocket_timer.timeout(
            [this, reason] {
                if (reason != WebsocketCloseReason::ServiceRestart) {
                    this->pending_configuration_slot =
                        get_next_configuration_slot(get_active_network_configuration_slot());
                }
                this->try_connect_websocket();
            },
            WEBSOCKET_INIT_DELAY);
    }
}

void ConnectivityManager::cache_network_connection_profiles() {
    // get all the network connection profiles from the device model and cache them
    this->cached_network_connection_profiles =
        json::parse(this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConnectionProfiles));

    for (const std::string& str : ocpp::split_string(
             this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority),
             ',')) {
        int num = std::stoi(str);
        this->network_connection_slots.push_back(num);
    }
}

} // namespace v201
} // namespace ocpp
