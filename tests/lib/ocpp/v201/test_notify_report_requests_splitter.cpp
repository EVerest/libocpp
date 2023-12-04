
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ocpp/v201/notify_report_requests_splitter.hpp>

namespace ocpp {
namespace v201 {

class NotifyReportRequestsSplitterTest : public ::testing::Test {
protected:

    // verify returned payloads are actual serializations of Call<NotifyReportRequest> instances
    static void check_valid_call_payload( json payload ) {

        Call<NotifyReportRequest> call{};
        from_json(payload, call);
        ASSERT_EQ(call.msg.get_type(), "NotifyReport");

    }

};

/// \brief Test a request with no report data results into a single message
TEST_F(NotifyReportRequestsSplitterTest, test_create_single_request_no_report_data) {

    NotifyReportRequest req{};
    req.reportData = std::nullopt;
    json req_json = req;

    NotifyReportRequestsSplitter splitter{req, 1000, []() { return MessageId{"test_message_id"}; }};

    auto res = splitter.create_call_payloads();

    ASSERT_EQ(res.size(), 1);
    auto request = res[0];

    ASSERT_EQ(request.size(), 4);
    ASSERT_EQ("2", request[0].dump());
    ASSERT_EQ("\"test_message_id\"", request[1].dump());
    ASSERT_EQ("\"NotifyReport\"", request[2].dump());

    ASSERT_EQ(json(req_json).dump(), request[3].dump());

    check_valid_call_payload(res[0]);
}


/// \brief Test a request that fits exactly the provided bound is not split
TEST_F(NotifyReportRequestsSplitterTest, test_create_single_request) {

    NotifyReportRequest req{};
    req.reportData = {ReportData{{"component_name"}, {"variable_name"}, {}, {}, {}},
                      ReportData{{"component_name2"}, {"variable_name2"}, {}, {}, {}}};
    req.requestId = 42;
    req.tbc = false;
    req.seqNo = 0;
    json req_json = req;


    size_t full_size = json{2,"test_message_id","NotifyReport",req }.dump().size();


    NotifyReportRequestsSplitter splitter{
        req, full_size ,
        []() { return MessageId{"test_message_id"}; }};

    auto res = splitter.create_call_payloads();

    ASSERT_EQ(res.size(), 1);
    auto request = res[0];
    check_valid_call_payload(request);

    ASSERT_EQ(request.size(), 4);
    ASSERT_EQ(request[0].dump(), "2");
    ASSERT_EQ(request[1].dump(), "\"test_message_id\"");
    ASSERT_EQ("\"NotifyReport\"", request[2].dump());

    std::stringstream expected_report_data_json;
    expected_report_data_json << "[" << json(req.reportData.value()[0]) << "," << json(req.reportData.value()[1])
                              << "]";
    ASSERT_EQ(expected_report_data_json.str(), request[3]["reportData"].dump());
    ASSERT_EQ("false", request[3]["tbc"].dump());
    ASSERT_EQ("0", request[3]["seqNo"].dump());
    ASSERT_EQ(req_json["generatedAt"], request[3]["generatedAt"]);


}

// \brief Test a request that is one byte too long is split
TEST_F(NotifyReportRequestsSplitterTest, test_create_split_request) {

    NotifyReportRequest req{};
    req.requestId = 42;
    req.reportData = {ReportData{{"component_name"}, {"variable_name"}, {}, {}, {}},
                      ReportData{{"component_name2"}, {"variable_name2"}, {}, {}, {}}};
    req.tbc = false;
    json req_json = req;

    size_t full_size = json{2,"test_message_id","NotifyReport",req }.dump().size();

    NotifyReportRequestsSplitter splitter{
        req, full_size - 1,
        []() { return MessageId{"test_message_id"}; }};
    auto res = splitter.create_call_payloads();

    ASSERT_EQ(res.size(), 2);

    for (int i = 0; i < 2; i++) {
        auto request = res[i];
        check_valid_call_payload(request);
        ASSERT_EQ(request.size(), 4);
        ASSERT_EQ("2", request[0].dump());
        ASSERT_EQ("\"test_message_id\"", request[1].dump());
        ASSERT_EQ("\"NotifyReport\"", request[2].dump());

        std::stringstream expected_report_data_json;
        expected_report_data_json << "[" << json(req.reportData.value()[i]).dump() << "]";
        ASSERT_EQ(expected_report_data_json.str(), request[3]["reportData"].dump());

        ASSERT_EQ(req_json["generatedAt"], request[3]["generatedAt"]);
        if (i == 0) {
            ASSERT_EQ(request[3]["tbc"].dump(), "true");
            ASSERT_EQ(request[3]["seqNo"].dump(), "0");
        } else {
            ASSERT_EQ(request[3]["tbc"].dump(), "false");
            ASSERT_EQ(request[3]["seqNo"].dump(), "1");
        }
    }
}

//  \brief Test that each split contains at least one report data object, even if it exceeds the size bound.
TEST_F(NotifyReportRequestsSplitterTest, test_splits_contains_at_least_one_report) {

    NotifyReportRequest req{};
    req.requestId = 42;
    req.reportData = {ReportData{{"component_name"}, {"variable_name"}, {}, {}, {}},
                      ReportData{{"component_name2"}, {"variable_name2"}, {}, {}, {}}};
    req.tbc = false;
    json req_json = req;

    NotifyReportRequestsSplitter splitter{req, 1, []() { return MessageId{"test_message_id"}; }};
    auto res = splitter.create_call_payloads();

    ASSERT_EQ(res.size(), 2);

    for (int i = 0; i < 2; i++) {
        auto request = res[i];
        check_valid_call_payload(request);
        ASSERT_EQ(request.size(), 4);
        ASSERT_EQ("2", request[0].dump());
        ASSERT_EQ("\"test_message_id\"", request[1].dump());
        ASSERT_EQ("\"NotifyReport\"", request[2].dump());

        std::stringstream expected_report_data_json;
        expected_report_data_json << "[" << json(req.reportData.value()[i]).dump() << "]";
        ASSERT_EQ(expected_report_data_json.str(), request[3]["reportData"].dump());

        ASSERT_EQ(req_json["generatedAt"], request[3]["generatedAt"]);
        if (i == 0) {
            ASSERT_EQ(request[3]["tbc"].dump(), "true");
            ASSERT_EQ(request[3]["seqNo"].dump(), "0");
        } else {
            ASSERT_EQ(request[3]["tbc"].dump(), "false");
            ASSERT_EQ(request[3]["seqNo"].dump(), "1");
        }
    }
}

} // namespace v201
} // namespace ocpp
