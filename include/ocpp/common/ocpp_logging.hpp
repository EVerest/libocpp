// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_LOGGING_HPP
#define OCPP_COMMON_LOGGING_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ocpp/common/types.hpp>
#include <thread>

namespace ocpp {

/// Container for a formatted OCPP message with a message type
struct FormattedMessageWithType {
    std::string message_type; ///< The message type
    std::string message;      ///< The message content
};

/// Configuration for log rotation
struct LogRotationConfig {
    bool date_suffix; ///< If set to true the log rotation files use a date after the ".", if not use the traditional
                      ///< .0, .1 ... style
    uint64_t
        maximum_file_size_bytes; ///< The maximum size of the log file in bytes after which the file will be rotated
    uint64_t maximum_file_count; ///< The maximum number of log files to keep in rotation

    LogRotationConfig(bool date_suffix, uint64_t maximum_file_size_bytes, uint64_t maximum_file_count) :
        date_suffix(date_suffix),
        maximum_file_size_bytes(maximum_file_size_bytes),
        maximum_file_count(maximum_file_count) {
    }
};

/// \brief contains a ocpp message logging abstraction
class MessageLogging {
private:
    bool log_messages;
    std::string message_log_path; // FIXME: use fs::path here
    std::string output_file_name;
    bool log_to_console;
    bool detailed_log_to_console;
    bool log_to_file;
    bool log_to_html;
    bool log_security;
    bool session_logging;
    std::filesystem::path log_file;
    std::ofstream log_os;
    std::filesystem::path html_log_file;
    std::ofstream html_log_os;
    std::ofstream security_log_file;
    std::mutex output_file_mutex;
    std::function<void(const std::string& message, MessageDirection direction)> message_callback;
    std::map<std::string, std::string> lookup_map;
    std::recursive_mutex session_id_logging_mutex;
    std::map<std::string, std::shared_ptr<MessageLogging>> session_id_logging;
    bool rotate_logs;
    bool date_suffix;
    std::string logfile_basename;
    uint64_t maximum_file_size_bytes;
    uint64_t maximum_file_count;

    void initialize();
    void log_output(unsigned int typ, const std::string& message_type, const std::string& json_str);
    std::string html_encode(const std::string& msg);
    FormattedMessageWithType format_message(const std::string& message_type, const std::string& json_str);
    void open_html_tags(std::ofstream& os);
    void close_html_tags(std::ofstream& os);
    std::string get_datetime_string();

    /// \returns file size of the given path or 0 if the file does not exist
    std::uintmax_t file_size(const std::filesystem::path& path);
    void rotate_log(const std::string& file_basename);
    void rotate_log();
    void rotate_log_if_needed(const std::filesystem::path& path, std::ofstream& os);
    void rotate_log_if_needed(const std::filesystem::path& path, std::ofstream& os,
                              std::function<void(std::ofstream& os)> before_close_of_os,
                              std::function<void(std::ofstream& os)> after_open_of_os);

public:
    /// \brief Creates a new MessageLogging object with the provided \p configuration
    explicit MessageLogging(
        bool log_messages, const std::string& message_log_path, const std::string& output_file_name,
        bool log_to_console, bool detailed_log_to_console, bool log_to_file, bool log_to_html, bool log_security,
        bool session_logging,
        std::function<void(const std::string& message, MessageDirection direction)> message_callback);

    explicit MessageLogging(
        bool log_messages, const std::string& message_log_path, const std::string& output_file_name,
        bool log_to_console, bool detailed_log_to_console, bool log_to_file, bool log_to_html, bool log_security,
        bool session_logging,
        std::function<void(const std::string& message, MessageDirection direction)> message_callback,
        LogRotationConfig log_rotation_config);
    ~MessageLogging();

    void charge_point(const std::string& message_type, const std::string& json_str);
    void central_system(const std::string& message_type, const std::string& json_str);
    void sys(const std::string& msg);
    void security(const std::string& msg);
    void start_session_logging(const std::string& session_id, const std::string& log_path);
    void stop_session_logging(const std::string& session_id);
    std::string get_message_log_path();
    bool session_logging_active();
};

} // namespace ocpp
#endif // OCPP_COMMON_LOGGING_HPP
