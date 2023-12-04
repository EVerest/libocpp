// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <ocpp/v201/notify_report_requests_splitter.hpp>

namespace ocpp {
namespace v201 {

std::vector<json> NotifyReportRequestsSplitter::create_call_payloads() {

    if (!original_request.reportData.has_value()) {
        return std::vector<json>{
            {MessageTypeId::CALL, message_id_generator_callback().get(), "NotifyReport", json(original_request)}};
    }

    std::vector<json> payloads{};
    int seq_no = 0;
    auto report_data_iterator = original_request.reportData->begin();
    while (seq_no == 0 || report_data_iterator != original_request.reportData->end()) {
        payloads.emplace_back(create_next_payload(seq_no, report_data_iterator, original_request.reportData->end(),
                                                  message_id_generator_callback().get()));
        seq_no++;
    }

    if (seq_no > 1) {
        EVLOG_info << "Split NotifyReportRequest '" << original_request.requestId << "' into " << seq_no
                   << " messages.";
    }

    return payloads;
}

json NotifyReportRequestsSplitter::create_next_report_data_json(
    std::vector<ocpp::v201::ReportData>::const_iterator& report_data_iterator,
    const std::vector<ocpp::v201::ReportData>::const_iterator& report_data_end, const size_t& remaining_size) {

    if (report_data_iterator == report_data_end) {
        return json::array();
    }

    json report_data_json{*report_data_iterator};
    report_data_iterator++;

    if (report_data_iterator == report_data_end) {
        return report_data_json;
    }

    auto size = report_data_json.dump().size();

    for (; report_data_iterator != report_data_end; report_data_iterator++) {
        json current_json = *report_data_iterator;
        auto current_json_size = current_json.dump().size();
        if (size + 1 + current_json_size <= remaining_size) {
            size += 1 + current_json_size;
            report_data_json.emplace_back(std::move(current_json));
        } else {
            break;
        }
    }

    return report_data_json;
}

json NotifyReportRequestsSplitter::create_next_payload(
    const int& seq_no, std::vector<ocpp::v201::ReportData>::const_iterator& report_data_iterator,
    const std::vector<ocpp::v201::ReportData>::const_iterator& report_data_end, const std::string& message_id) {

    NotifyReportRequest req{};
    req.requestId = original_request.requestId;
    req.generatedAt = original_request.generatedAt;
    req.tbc = false;
    json request_json = req;

    json call_base{MessageTypeId::CALL, message_id, "NotifyReport"};
    size_t base_json_string_length =
        call_base.dump().size() + 1 + request_json.dump().size() + std::string{R"(,"reportData":)"}.size();
    size_t remaining_size = this->max_size >= base_json_string_length ? this->max_size - base_json_string_length : 0;
    request_json["reportData"] = create_next_report_data_json(report_data_iterator, report_data_end, remaining_size);
    request_json["tbc"] = report_data_iterator != report_data_end;
    request_json["seqNo"] = seq_no;

    call_base.emplace_back(request_json);

    return call_base;
}

} // namespace v201
} // namespace ocpp