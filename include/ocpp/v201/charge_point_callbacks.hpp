#pragma once

#include <memory>
#include <cstdint>

#include <ocpp/v201/device_model.hpp>

#include <ocpp/v201/messages/BootNotification.hpp>
#include <ocpp/v201/messages/ClearDisplayMessage.hpp>
#include <ocpp/v201/messages/DataTransfer.hpp>
#include <ocpp/v201/messages/GetDisplayMessages.hpp>
#include <ocpp/v201/messages/GetLog.hpp>
#include <ocpp/v201/messages/RequestStartTransaction.hpp>
#include <ocpp/v201/messages/SetDisplayMessage.hpp>
#include <ocpp/v201/messages/TransactionEvent.hpp>
#include <ocpp/v201/messages/UnlockConnector.hpp>
#include <ocpp/v201/messages/UpdateFirmware.hpp>


namespace ocpp::v201
{
struct Callbacks {
    /// \brief Function to check if the callback struct is completely filled. All std::functions should hold a function,
    ///       all std::optional<std::functions> should either be empty or hold a function.
    /// \param device_model The device model, to check if certain modules are enabled / available.
    ///
    /// \retval false if any of the normal callbacks are nullptr or any of the optional ones are filled with a nullptr
    ///        true otherwise
    bool all_callbacks_valid(std::shared_ptr<DeviceModel> device_model) const;

    ///
    /// \brief Callback if reset is allowed. If evse_id has a value, reset only applies to the given evse id. If it has
    ///        no value, applies to complete charging station.
    ///
    std::function<bool(const std::optional<const int32_t> evse_id, const ResetEnum& reset_type)>
        is_reset_allowed_callback;
    std::function<void(const std::optional<const int32_t> evse_id, const ResetEnum& reset_type)> reset_callback;
    std::function<void(const int32_t evse_id, const ReasonEnum& stop_reason)> stop_transaction_callback;
    std::function<void(const int32_t evse_id)> pause_charging_callback;

    /// \brief Used to notify the user of libocpp that the Operative/Inoperative state of the charging station changed
    /// If as a result the state of EVSEs or connectors changed as well, libocpp will additionally call the
    /// evse_effective_operative_status_changed_callback once for each EVSE whose status changed, and
    /// connector_effective_operative_status_changed_callback once for each connector whose status changed.
    /// If left empty, the callback is ignored.
    /// \param new_status The operational status the CS switched to
    std::optional<std::function<void(const OperationalStatusEnum new_status)>>
        cs_effective_operative_status_changed_callback;

    /// \brief Used to notify the user of libocpp that the Operative/Inoperative state of an EVSE changed
    /// If as a result the state of connectors changed as well, libocpp will additionally call the
    /// connector_effective_operative_status_changed_callback once for each connector whose status changed.
    /// If left empty, the callback is ignored.
    /// \param evse_id The id of the EVSE
    /// \param new_status The operational status the EVSE switched to
    std::optional<std::function<void(const int32_t evse_id, const OperationalStatusEnum new_status)>>
        evse_effective_operative_status_changed_callback;

    /// \brief Used to notify the user of libocpp that the Operative/Inoperative state of a connector changed.
    /// \param evse_id The id of the EVSE
    /// \param connector_id The ID of the connector within the EVSE
    /// \param new_status The operational status the connector switched to
    std::function<void(const int32_t evse_id, const int32_t connector_id, const OperationalStatusEnum new_status)>
        connector_effective_operative_status_changed_callback;

    std::function<GetLogResponse(const GetLogRequest& request)> get_log_request_callback;
    std::function<UnlockConnectorResponse(const int32_t evse_id, const int32_t connecor_id)> unlock_connector_callback;
    // callback to be called when the request can be accepted. authorize_remote_start indicates if Authorize.req needs
    // to follow or not
    std::function<void(const RequestStartTransactionRequest& request, const bool authorize_remote_start)>
        remote_start_transaction_callback;
    ///
    /// \brief Check if the current reservation for the given evse id is made for the id token / group id token.
    /// \return True if evse is reserved for the given id token / group id token, false if it is reserved for another
    ///         one.
    ///
    std::function<bool(const int32_t evse_id, const CiString<36> idToken,
                       const std::optional<CiString<36>> groupIdToken)>
        is_reservation_for_token_callback;
    std::function<UpdateFirmwareResponse(const UpdateFirmwareRequest& request)> update_firmware_request_callback;
    // callback to be called when a variable has been changed by the CSMS
    std::optional<std::function<void(const SetVariableData& set_variable_data)>> variable_changed_callback;
    // callback is called when receiving a SetNetworkProfile.req from the CSMS
    std::optional<std::function<SetNetworkProfileStatusEnum(
        const int32_t configuration_slot, const NetworkConnectionProfile& network_connection_profile)>>
        validate_network_profile_callback;
    std::optional<std::function<bool(const NetworkConnectionProfile& network_connection_profile)>>
        configure_network_connection_profile_callback;
    std::optional<std::function<void(const ocpp::DateTime& currentTime)>> time_sync_callback;

    /// \brief callback to be called to congfigure ocpp message logging
    std::optional<std::function<void(const std::string& message, MessageDirection direction)>> ocpp_messages_callback;

    ///
    /// \brief callback function that can be used to react to a security event callback. This callback is
    /// called only if the SecurityEvent occured internally within libocpp
    /// Typically this callback is used to log security events in the security log
    ///
    std::function<void(const CiString<50>& event_type, const std::optional<CiString<255>>& tech_info)>
        security_event_callback;

    /// \brief Callback for indicating when a charging profile is received and was accepted.
    std::function<void()> set_charging_profiles_callback;

    /// \brief  Callback for when a bootnotification response is received
    std::optional<std::function<void(const ocpp::v201::BootNotificationResponse& boot_notification_response)>>
        boot_notification_callback;

    /// \brief Callback function that can be used to get (human readable) customer information based on the given
    /// arguments
    std::optional<std::function<std::string(const std::optional<CertificateHashDataType> customer_certificate,
                                            const std::optional<IdToken> id_token,
                                            const std::optional<CiString<64>> customer_identifier)>>
        get_customer_information_callback;

    /// \brief Callback function that can be called to clear customer information based on the given arguments
    std::optional<std::function<void(const std::optional<CertificateHashDataType> customer_certificate,
                                     const std::optional<IdToken> id_token,
                                     const std::optional<CiString<64>> customer_identifier)>>
        clear_customer_information_callback;

    /// \brief Callback function that can be called when all connectors are unavailable
    std::optional<std::function<void()>> all_connectors_unavailable_callback;

    /// \brief Callback function that can be used to handle arbitrary data transfers for all vendorId and
    /// messageId
    std::optional<std::function<DataTransferResponse(const DataTransferRequest& request)>> data_transfer_callback;

    /// \brief Callback function that is called when a transaction_event was sent to the CSMS
    std::optional<std::function<void(const TransactionEventRequest& transaction_event)>> transaction_event_callback;

    /// \brief Callback function that is called when a transaction_event_response was received from the CSMS
    std::optional<std::function<void(const TransactionEventRequest& transaction_event,
                                     const TransactionEventResponse& transaction_event_response)>>
        transaction_event_response_callback;

    /// \brief Callback function is called when the websocket connection status changes
    std::optional<std::function<void(const bool is_connected)>> connection_state_changed_callback;

    // TODO mz if one of the three is defined, they must all be defined
    /// \brief Callback functions called for get / set / clear display messages
    std::optional<std::function<std::vector<DisplayMessage>(const GetDisplayMessagesRequest& request /* TODO */)>>
        get_display_message_callback;
    std::optional<std::function<SetDisplayMessageResponse(const std::vector<DisplayMessage>& display_messages)>>
        set_display_message_callback;
    std::optional<std::function<ClearDisplayMessageResponse(const ClearDisplayMessageRequest& request /* TODO */)>>
        clear_display_message_callback;

    /// \brief Callback function is called when running cost is set.
    std::optional<std::function<void(const RunningCost& running_cost)>> set_running_cost_callback;
};
}
