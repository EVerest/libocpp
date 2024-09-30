#include "ocpp/common/message_queue.hpp"

#include "gmock/gmock.h"

namespace ocpp {

class MessageQueueMock : public MessageQueueInterface<v201::MessageType> {
public:
    MOCK_METHOD(void, start, ());
    MOCK_METHOD(void, reset_next_message_to_send, ());
    MOCK_METHOD(void, get_persisted_messages_from_db, (bool ignore_security_event_notifications));
    MOCK_METHOD(void, push, (const json& message, const bool stall_until_accepted));
    MOCK_METHOD(void, push_call_result, (const json& call_result_json, const MessageId& unique_id));
    MOCK_METHOD(void, push, (CallError call_error));
    MOCK_METHOD(std::future<EnhancedMessage<v201::MessageType>>, push_async_internal,
                (std::shared_ptr<ControlMessage<v201::MessageType>> message));
    MOCK_METHOD(EnhancedMessage<v201::MessageType>, receive, (std::string_view message));
    MOCK_METHOD(void, reset_in_flight, ());
    MOCK_METHOD(void, handle_call_result, (EnhancedMessage<v201::MessageType> & enhanced_message));
    MOCK_METHOD(void, handle_timeout_or_callerror,
                (const std::optional<EnhancedMessage<v201::MessageType>>& enhanced_message_opt));
    MOCK_METHOD(void, stop, ());
    MOCK_METHOD(void, pause, ());
    MOCK_METHOD(void, resume, (std::chrono::seconds delay_on_reconnect));
    MOCK_METHOD(void, set_registration_status_accepted, ());
    MOCK_METHOD(bool, is_transaction_message_queue_empty, ());
    MOCK_METHOD(bool, contains_transaction_messages, (const CiString<36> transaction_id));
    MOCK_METHOD(bool, contains_stop_transaction_message, (const int32_t transaction_id));
    MOCK_METHOD(void, update_transaction_message_attempts, (const int transaction_message_attempts));
    MOCK_METHOD(void, update_transaction_message_retry_interval, (const int transaction_message_retry_interval));
    MOCK_METHOD(void, update_message_timeout, (const int timeout));
    MOCK_METHOD(MessageId, createMessageId, ());
    MOCK_METHOD(void, add_stopped_transaction_id, (std::string stop_transaction_message_id, int32_t transaction_id));
    MOCK_METHOD(void, add_meter_value_message_id,
                (const std::string& start_transaction_message_id, const std::string& meter_value_message_id));
    MOCK_METHOD(void, notify_start_transaction_handled,
                (const std::string& start_transaction_message_id, const int32_t transaction_id));
    MOCK_METHOD(v201::MessageType, string_to_messagetype, (const std::string& s));
    MOCK_METHOD(std::string, messagetype_to_string, (v201::MessageType m));
};
} // namespace ocpp