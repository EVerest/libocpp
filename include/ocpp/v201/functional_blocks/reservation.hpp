// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/message_dispatcher.hpp>
#include <ocpp/v201/message_handler.hpp>
#include <ocpp/v201/messages/CancelReservation.hpp>
#include <ocpp/v201/messages/ReservationStatusUpdate.hpp>
#include <ocpp/v201/messages/ReserveNow.hpp>

#pragma once

namespace ocpp::v201 {

class EvseInterface;
class EvseManagerInterface;

class ReservationInterface : public MessageHandlerInterface {
public:
    virtual ~ReservationInterface(){};
    virtual void on_reservation_status(const int32_t reservation_id, const ReservationUpdateStatusEnum status) = 0;
    virtual ocpp::ReservationCheckStatus
    is_evse_reserved_for_other(const EvseInterface& evse, const IdToken& id_token,
                               const std::optional<IdToken>& group_id_token) const = 0;
    virtual void on_reserved(const int32_t evse_id, const int32_t connector_id) = 0;
    virtual void on_reservation_cleared(const int32_t evse_id, const int32_t connector_id) = 0;
};

class Reservation : public ReservationInterface {
private: // Members
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    std::shared_ptr<DeviceModel> device_model;
    EvseManagerInterface& evse_manager;

    /// \brief Callback function is called when a reservation request is received from the CSMS
    std::function<ReserveNowStatusEnum(const ReserveNowRequest& request)> reserve_now_callback;
    /// \brief Callback function is called when a cancel reservation request is received from the CSMS
    std::function<bool(const int32_t reservationId)> cancel_reservation_callback;
    ///
    /// \brief Check if the current reservation for the given evse id is made for the id token / group id token.
    /// \return The reservation check status of this evse / id token.
    ///
    const std::function<ocpp::ReservationCheckStatus(const int32_t evse_id, const CiString<36> idToken,
                                                     const std::optional<CiString<36>> groupIdToken)>
        is_reservation_for_token_callback;

public:
    Reservation(MessageDispatcherInterface<MessageType>& message_dispatcher, std::shared_ptr<DeviceModel> device_model,
                EvseManagerInterface& evse_manager,
                std::function<ReserveNowStatusEnum(const ReserveNowRequest& request)> reserve_now_callback,
                std::function<bool(const int32_t reservationId)> cancel_reservation_callback,
                const std::function<ocpp::ReservationCheckStatus(const int32_t evse_id, const CiString<36> idToken,
                                                                 const std::optional<CiString<36>> groupIdToken)>
                    is_reservation_for_token_callback);
    virtual void handle_message(const ocpp::EnhancedMessage<MessageType>& message) override;

    virtual void on_reservation_status(const int32_t reservation_id, const ReservationUpdateStatusEnum status) override;
    virtual ocpp::ReservationCheckStatus
    is_evse_reserved_for_other(const EvseInterface& evse, const IdToken& id_token,
                               const std::optional<IdToken>& group_id_token) const override;
    virtual void on_reserved(const int32_t evse_id, const int32_t connector_id) override;
    virtual void on_reservation_cleared(const int32_t evse_id, const int32_t connector_id) override;

private: // Functions
    void handle_reserve_now_request(Call<ReserveNowRequest> call);
    void handle_cancel_reservation_callback(Call<CancelReservationRequest> call);

    void send_reserve_now_rejected_response(const MessageId& unique_id, const std::string& status_info);

    ///
    /// \brief Check if the connector exists on the given evse id.
    /// \param evse_id          The evse id to check for.
    /// \param connector_type   The connector type.
    /// \return False if evse id does not exist or evse does not have the given connector type.
    ///
    bool does_connector_exist(const uint32_t evse_id, std::optional<ConnectorEnum> connector_type);
};

} // namespace ocpp::v201
