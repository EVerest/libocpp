// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/message_queue.hpp>

#include <everest/logging.hpp>

namespace ocpp {

MessageTransmissionPriority get_message_transmission_priority(bool is_boot_notification_message, bool triggered,
                                                              bool registration_already_accepted,
                                                              bool is_transaction_related, bool queue_all_message) {
    if (registration_already_accepted || is_boot_notification_message || triggered) {
        return MessageTransmissionPriority::SEND_IMMEDIATELY;
    }

    if (is_transaction_related || queue_all_message) {
        return MessageTransmissionPriority::SEND_AFTER_REGISTRATION_STATUS_ACCEPTED;
    }

    return MessageTransmissionPriority::DISCARD;
}

bool is_transaction_message(const ocpp::v16::MessageType message_type) {
    return (message_type == v16::MessageType::StartTransaction) ||
           (message_type == v16::MessageType::StopTransaction) || (message_type == v16::MessageType::MeterValues) ||
           (message_type == v16::MessageType::SecurityEventNotification);
}

bool is_transaction_message(const ocpp::v201::MessageType message_type) {
    return (message_type == v201::MessageType::TransactionEvent) ||
           (message_type == v201::MessageType::SecurityEventNotification);
}

bool is_boot_notification_message(const ocpp::v16::MessageType message_type) {
    return message_type == ocpp::v16::MessageType::BootNotification;
}

bool is_boot_notification_message(const ocpp::v201::MessageType message_type) {
    return message_type == ocpp::v201::MessageType::BootNotification;
}

template <>
ControlMessage<v16::MessageType>::ControlMessage(const json& message, const bool stall_until_accepted) :
    message(message.get<json::array_t>()),
    messageType(v16::conversions::string_to_messagetype(message.at(CALL_ACTION))),
    message_attempts(0),
    initial_unique_id(message[MESSAGE_ID]),
    stall_until_accepted(stall_until_accepted) {
}

template <> bool ControlMessage<v16::MessageType>::isTransactionUpdateMessage() const {
    return (this->messageType == v16::MessageType::MeterValues);
}

template <>
ControlMessage<v201::MessageType>::ControlMessage(const json& message, const bool stall_until_accepted) :
    message(message.get<json::array_t>()),
    messageType(v201::conversions::string_to_messagetype(message.at(CALL_ACTION))),
    message_attempts(0),
    initial_unique_id(message[MESSAGE_ID]),
    stall_until_accepted(stall_until_accepted) {
}

template <> bool ControlMessage<v201::MessageType>::isTransactionUpdateMessage() const {
    if (this->messageType == v201::MessageType::TransactionEvent) {
        return v201::TransactionEventRequest{this->message.at(CALL_PAYLOAD)}.eventType ==
               v201::TransactionEventEnum::Updated;
    }
    return false;
}

template <> bool ControlMessage<v201::MessageType>::isBootNotificationMessage() const {
    return this->messageType == v201::MessageType::BootNotification;
}

template <> v16::MessageType MessageQueue<v16::MessageType>::string_to_messagetype(const std::string& s) {
    return v16::conversions::string_to_messagetype(s);
}

template <> v201::MessageType MessageQueue<v201::MessageType>::string_to_messagetype(const std::string& s) {
    return v201::conversions::string_to_messagetype(s);
}

template <> std::string MessageQueue<v201::MessageType>::messagetype_to_string(const v201::MessageType m) {
    return v201::conversions::messagetype_to_string(m);
}

} // namespace ocpp
