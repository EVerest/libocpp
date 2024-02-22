#include "ocpp/v201/messages/SetNetworkProfile.hpp"
#include <everest/logging.hpp>
#include <ocpp/v201/connectivity_manager.h>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/device_model.hpp>

namespace ocpp {
namespace v201 {

constexpr int32_t default_network_config_timeout_seconds = 60;

ConnectivityManager::ConnectivityManager(DeviceModel& device_model, std::shared_ptr<EvseSecurity> evse_security,
                                         std::shared_ptr<MessageLogging> logging) :
    device_model(device_model),
    evse_security(evse_security),
    logging(logging),
    network_configuration_priority(0),
    disable_automatic_websocket_reconnects(false) {
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

void ConnectivityManager::start_websocket() {
    this->init_websocket();
    if (this->websocket != nullptr) {
        this->websocket->connect();
    }
}

void ConnectivityManager::stop() {
}

void ConnectivityManager::connect_websocket(std::optional<int32_t> config_slot) {
    if (this->websocket == nullptr) {
        return;
    }
    // TODO check if websocket is connected???
    if (!this->websocket->is_connected()) {
        this->disable_automatic_websocket_reconnects = false;
        this->init_websocket(config_slot);
        // TODO this should be removed?? It should connect when the future is filled and returned
        this->websocket->connect();
    }
}

void ConnectivityManager::disconnect_websocket(WebsocketCloseReason code) {
    if (this->websocket == nullptr) {
        return;
    }

    if (this->websocket->is_connected()) {
        // TODO set this???
        // this->disable_automatic_websocket_reconnects = true;
        this->websocket->disconnect(code);
    }
}

bool ConnectivityManager::on_try_switch_network_connection_profile(const int32_t configuration_slot) {
    EVLOG_info << "=============on_try_switch_network_profile============" << configuration_slot;

    if (this->network_configuration_priority == 0) {
        // Can not connect to a lower configuration priority than it is currently connected to.
        return false;
    }

    // Convert to string as a vector of strings is used.
    const std::string configuration_slot_string = std::to_string(configuration_slot);

    // Check if configuration slot is valid and has higher priority.
    const std::vector<std::string> network_connection_priorities = ocpp::get_vector_from_csv(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
    if (network_connection_priorities.size() > 1) {
        if (network_connection_priorities.at(this->network_configuration_priority) == configuration_slot_string) {
            // This configuration slot is already connected.
            return true;
        }

        auto it = std::find(network_connection_priorities.begin(), network_connection_priorities.end(),
                            configuration_slot_string);
        if (it != network_connection_priorities.end()) {
            const uint32_t index = it - network_connection_priorities.begin();
            if (index < this->network_configuration_priority) {
                // Priority is indeed higher
                EVLOG_debug << "Trying to connect with higher priority network connection profile (new priority: "
                            << index + 1 << ", was: " << this->network_configuration_priority << ").";
            }
        } else {
            // Slot not found.
            return false;
        }
    }

    const std::optional<NetworkConnectionProfile> network_connection_profile_opt =
        this->get_network_connection_profile(configuration_slot);
    if (!network_connection_profile_opt.has_value()) {
        return false;
    }

    try {
        // TODO implement
        // call disconnect
        this->disconnect_websocket(); // normal close

        // TODO call configure network connection profile callback

        // call connect with the config_slot option
        this->connect_websocket(configuration_slot);
        return true;

    } catch (std::exception& e) {
        EVLOG_info << "ERROR===============>";
        return false;
    }
}

void ConnectivityManager::on_network_disconnected(const std::optional<int32_t> configuration_slot,
                                                  const std::optional<OCPPInterfaceEnum> ocpp_interface) {
    if (!configuration_slot.has_value() && !ocpp_interface.has_value()) {
        EVLOG_info << "Not clear which network is disconnected: configuration slot and ocpp interface are empty";
    }

    // TODO implementation:
    // - Check if configuration slot is valid
    // - Check if configuration_slot and ocpp_interface are pointing to the same ocpp interface
    // - Check if configuration slot and / or ocpp interface is in use
    // - If that is the case, disconnect websocket
}

void ConnectivityManager::init_websocket(std::optional<int32_t> config_slot) {
    std::string configuration_slot;
    if (this->device_model.get_value<std::string>(ControllerComponentVariables::ChargePointId).find(':') !=
        std::string::npos) {
        EVLOG_AND_THROW(std::runtime_error("ChargePointId must not contain \':\'"));
    }

    int32_t config_slot_int;
    if (config_slot.has_value()) {
        EVLOG_info << " using configuration slot --->" << config_slot.value();
        configuration_slot = std::to_string(config_slot.value());
        config_slot_int = config_slot.value();
    } else {
        configuration_slot = ocpp::get_vector_from_csv(this->device_model.get_value<std::string>(
                                                           ControllerComponentVariables::NetworkConfigurationPriority))
                                 .at(this->network_configuration_priority);
        config_slot_int = std::stoi(configuration_slot);
    }

    WebsocketConnectionOptions connection_options = this->get_ws_connection_options(config_slot_int);
    const std::optional<NetworkConnectionProfile> network_connection_profile =
        this->get_network_connection_profile(config_slot_int);
    if (this->configure_network_connection_profile_callback.has_value() and network_connection_profile) {
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
            // Connect to next network configuration priority
            this->next_network_configuration_priority();
            // TODO: in the old code there was a timeout for this. Should we do that???
            // TODO check if this works ok, etc
            this->start_websocket();
            return;
        }
        case std::future_status::ready: {
            ConfigNetworkResult result = config_status.get();
            if (result.success) {
                connection_options.iface_or_ip = result.interface_address;
                pending_network_slot = config_slot_int;
                // TODO set pending! -> remove requested???
            } else {
                // No success, network could not be initialized?
                // Connect to next network configuration priority
                this->next_network_configuration_priority();
                // TODO: in the old code there was a timeout for this. Should we do that???
                // TODO check if this works ok, etc
                this->start_websocket();
                return;
            }
            break;
        }
        }
    } else {
        // No callback configured, just connect to this profile.
    }

    this->websocket = std::make_unique<Websocket>(connection_options, this->evse_security, this->logging);

    // TODO register callbacks

    // TODO implement
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

void ConnectivityManager::next_network_configuration_priority() {
    const auto network_connection_priorities = ocpp::get_vector_from_csv(
        this->device_model.get_value<std::string>(ControllerComponentVariables::NetworkConfigurationPriority));
    if (network_connection_priorities.size() > 1) {
        EVLOG_info << "Switching to next network configuration priority";
    }
    this->network_configuration_priority =
        (this->network_configuration_priority + 1) % (network_connection_priorities.size());
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

} // namespace v201
} // namespace ocpp
