// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>

#include <ocpp/v21/functional_blocks/bidirectional.hpp>

#include <ocpp/common/constants.hpp>
#include <ocpp/common/evse_security.hpp>
#include <ocpp/v2/connectivity_manager.hpp>
#include <ocpp/v2/ctrlr_component_variables.hpp>
#include <ocpp/v2/database_handler.hpp>
#include <ocpp/v2/device_model.hpp>
#include <ocpp/v2/functional_blocks/functional_block_context.hpp>
#include <ocpp/v2/utils.hpp>

ocpp::v2::Bidirectional::Bidirectional(const FunctionalBlockContext& context,
                                       std::optional<NotifyAllowedEnergyTransferCallback> callback) :
    context(context), notify_allowed_energy_transfer_callback(callback) {
}

ocpp::v2::Bidirectional::~Bidirectional() {
}

void ocpp::v2::Bidirectional::handle_message(const ocpp::EnhancedMessage<MessageType>& message) {
    const auto& json_message = message.message;

    if (message.messageType == MessageType::NotifyAllowedEnergyTransfer) {
        this->handle_notify_allowed_energy_transfer(json_message);
    } else {
        throw MessageTypeNotImplementedException(message.messageType);
    }
}

void ocpp::v2::Bidirectional::handle_notify_allowed_energy_transfer(
    Call<v21::NotifyAllowedEnergyTransferRequest> notify_allowed_energy_transfer) {
    if (this->context.ocpp_version != OcppProtocolVersion::v21) {
        EVLOG_error
            << "Received NotifyAllowedEnergyTransferRequest when not using OCPP2.1 this is not normal, ignoring...";
        return;
    }
    ocpp::v21::NotifyAllowedEnergyTransferResponse response;
    response.status = NotifyAllowedEnergyTransferStatusEnum::Accepted;
    // todo Check if service renegotiation supported (Device Model) + check ISO15118 version
    if (this->notify_allowed_energy_transfer_callback.has_value()) {
        this->notify_allowed_energy_transfer_callback.value()(notify_allowed_energy_transfer.msg.allowedEnergyTransfer,
                                                              notify_allowed_energy_transfer.msg.transactionId);
    } else {
        response.status = NotifyAllowedEnergyTransferStatusEnum::Rejected;
        ocpp::v2::StatusInfo status_info;
        status_info.reasonCode = "UnsupportedRequest";
        status_info.additionalInfo =
            (true ? "Impossible to trigger service renegotiation due to no support for service renegotiation."
                  : "Impossible to trigger service renegotiation due to the ISO15118 version that does not support "
                    "service renegotiation.");
        response.statusInfo = status_info;
    }
}
