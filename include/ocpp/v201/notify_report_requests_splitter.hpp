// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#ifndef OCPP_NOTIFY_REPORT_REQUESTS_SPLITTER_HPP
#define OCPP_NOTIFY_REPORT_REQUESTS_SPLITTER_HPP

#include "ocpp/common/call_types.hpp"
#include "ocpp/v201/messages/NotifyReport.hpp"

namespace ocpp {
namespace v201 {

/// \brief Utility class that is used to split NotifyReportRequest into several ones in case ReportData is too big.
class NotifyReportRequestsSplitter {

private:
    const NotifyReportRequest& original_request;
    size_t max_size;
    const std::function<MessageId()>& message_id_generator_callback;

public:
    /// \brief Splits the provided NotifyReportRequest into (potentially) several Call payloads
    /// \returns the json messages that serialize the resulting Call<NotifyReportRequest> objects
    NotifyReportRequestsSplitter(const NotifyReportRequest& originalRequest, size_t max_size,
                                 const std::function<MessageId()>& message_id_generator_callback) :
        original_request(originalRequest),
        max_size(max_size),
        message_id_generator_callback(message_id_generator_callback) {
    }

    NotifyReportRequestsSplitter() = delete;

private:
    json create_next_payload(const int& seq_no,
                             std::vector<ocpp::v201::ReportData>::const_iterator& report_data_iterator,
                             const std::vector<ocpp::v201::ReportData>::const_iterator& report_data_end,
                             const std::string& message_id);

    static json create_next_report_data_json(std::vector<ocpp::v201::ReportData>::const_iterator& report_data_iterator,
                                             const std::vector<ocpp::v201::ReportData>::const_iterator& report_data_end,
                                             const size_t& remaining_size);

public:
    std::vector<json> create_call_payloads();
};

} // namespace v201
} // namespace ocpp

#endif // OCPP_NOTIFY_REPORT_REQUESTS_SPLITTER_HPP
