// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <ocpp/v201/types.hpp>

namespace ocpp {
namespace v201 {

namespace conversions {

std::string messagetype_to_string(MessageType m) {
    switch (m) {
    case MessageType::Authorize:
        return "Authorize";
    case MessageType::AuthorizeResponse:
        return "AuthorizeResponse";
    case MessageType::BootNotification:
        return "BootNotification";
    case MessageType::BootNotificationResponse:
        return "BootNotificationResponse";
    case MessageType::CancelReservation:
        return "CancelReservation";
    case MessageType::CancelReservationResponse:
        return "CancelReservationResponse";
    case MessageType::CertificateSigned:
        return "CertificateSigned";
    case MessageType::CertificateSignedResponse:
        return "CertificateSignedResponse";
    case MessageType::ChangeAvailability:
        return "ChangeAvailability";
    case MessageType::ChangeAvailabilityResponse:
        return "ChangeAvailabilityResponse";
    case MessageType::ClearCache:
        return "ClearCache";
    case MessageType::ClearCacheResponse:
        return "ClearCacheResponse";
    case MessageType::ClearChargingProfile:
        return "ClearChargingProfile";
    case MessageType::ClearChargingProfileResponse:
        return "ClearChargingProfileResponse";
    case MessageType::ClearDisplayMessage:
        return "ClearDisplayMessage";
    case MessageType::ClearDisplayMessageResponse:
        return "ClearDisplayMessageResponse";
    case MessageType::ClearedChargingLimit:
        return "ClearedChargingLimit";
    case MessageType::ClearedChargingLimitResponse:
        return "ClearedChargingLimitResponse";
    case MessageType::ClearVariableMonitoring:
        return "ClearVariableMonitoring";
    case MessageType::ClearVariableMonitoringResponse:
        return "ClearVariableMonitoringResponse";
    case MessageType::CostUpdated:
        return "CostUpdated";
    case MessageType::CostUpdatedResponse:
        return "CostUpdatedResponse";
    case MessageType::CustomerInformation:
        return "CustomerInformation";
    case MessageType::CustomerInformationResponse:
        return "CustomerInformationResponse";
    case MessageType::DataTransfer:
        return "DataTransfer";
    case MessageType::DataTransferResponse:
        return "DataTransferResponse";
    case MessageType::DeleteCertificate:
        return "DeleteCertificate";
    case MessageType::DeleteCertificateResponse:
        return "DeleteCertificateResponse";
    case MessageType::FirmwareStatusNotification:
        return "FirmwareStatusNotification";
    case MessageType::FirmwareStatusNotificationResponse:
        return "FirmwareStatusNotificationResponse";
    case MessageType::Get15118EVCertificate:
        return "Get15118EVCertificate";
    case MessageType::Get15118EVCertificateResponse:
        return "Get15118EVCertificateResponse";
    case MessageType::GetBaseReport:
        return "GetBaseReport";
    case MessageType::GetBaseReportResponse:
        return "GetBaseReportResponse";
    case MessageType::GetCertificateStatus:
        return "GetCertificateStatus";
    case MessageType::GetCertificateStatusResponse:
        return "GetCertificateStatusResponse";
    case MessageType::GetChargingProfiles:
        return "GetChargingProfiles";
    case MessageType::GetChargingProfilesResponse:
        return "GetChargingProfilesResponse";
    case MessageType::GetCompositeSchedule:
        return "GetCompositeSchedule";
    case MessageType::GetCompositeScheduleResponse:
        return "GetCompositeScheduleResponse";
    case MessageType::GetDisplayMessages:
        return "GetDisplayMessages";
    case MessageType::GetDisplayMessagesResponse:
        return "GetDisplayMessagesResponse";
    case MessageType::GetInstalledCertificateIds:
        return "GetInstalledCertificateIds";
    case MessageType::GetInstalledCertificateIdsResponse:
        return "GetInstalledCertificateIdsResponse";
    case MessageType::GetLocalListVersion:
        return "GetLocalListVersion";
    case MessageType::GetLocalListVersionResponse:
        return "GetLocalListVersionResponse";
    case MessageType::GetLog:
        return "GetLog";
    case MessageType::GetLogResponse:
        return "GetLogResponse";
    case MessageType::GetMonitoringReport:
        return "GetMonitoringReport";
    case MessageType::GetMonitoringReportResponse:
        return "GetMonitoringReportResponse";
    case MessageType::GetReport:
        return "GetReport";
    case MessageType::GetReportResponse:
        return "GetReportResponse";
    case MessageType::GetTransactionStatus:
        return "GetTransactionStatus";
    case MessageType::GetTransactionStatusResponse:
        return "GetTransactionStatusResponse";
    case MessageType::GetVariables:
        return "GetVariables";
    case MessageType::GetVariablesResponse:
        return "GetVariablesResponse";
    case MessageType::Heartbeat:
        return "Heartbeat";
    case MessageType::HeartbeatResponse:
        return "HeartbeatResponse";
    case MessageType::InstallCertificate:
        return "InstallCertificate";
    case MessageType::InstallCertificateResponse:
        return "InstallCertificateResponse";
    case MessageType::LogStatusNotification:
        return "LogStatusNotification";
    case MessageType::LogStatusNotificationResponse:
        return "LogStatusNotificationResponse";
    case MessageType::MeterValues:
        return "MeterValues";
    case MessageType::MeterValuesResponse:
        return "MeterValuesResponse";
    case MessageType::NotifyChargingLimit:
        return "NotifyChargingLimit";
    case MessageType::NotifyChargingLimitResponse:
        return "NotifyChargingLimitResponse";
    case MessageType::NotifyCustomerInformation:
        return "NotifyCustomerInformation";
    case MessageType::NotifyCustomerInformationResponse:
        return "NotifyCustomerInformationResponse";
    case MessageType::NotifyDisplayMessages:
        return "NotifyDisplayMessages";
    case MessageType::NotifyDisplayMessagesResponse:
        return "NotifyDisplayMessagesResponse";
    case MessageType::NotifyEVChargingNeeds:
        return "NotifyEVChargingNeeds";
    case MessageType::NotifyEVChargingNeedsResponse:
        return "NotifyEVChargingNeedsResponse";
    case MessageType::NotifyEVChargingSchedule:
        return "NotifyEVChargingSchedule";
    case MessageType::NotifyEVChargingScheduleResponse:
        return "NotifyEVChargingScheduleResponse";
    case MessageType::NotifyEvent:
        return "NotifyEvent";
    case MessageType::NotifyEventResponse:
        return "NotifyEventResponse";
    case MessageType::NotifyMonitoringReport:
        return "NotifyMonitoringReport";
    case MessageType::NotifyMonitoringReportResponse:
        return "NotifyMonitoringReportResponse";
    case MessageType::NotifyReport:
        return "NotifyReport";
    case MessageType::NotifyReportResponse:
        return "NotifyReportResponse";
    case MessageType::PublishFirmware:
        return "PublishFirmware";
    case MessageType::PublishFirmwareResponse:
        return "PublishFirmwareResponse";
    case MessageType::PublishFirmwareStatusNotification:
        return "PublishFirmwareStatusNotification";
    case MessageType::PublishFirmwareStatusNotificationResponse:
        return "PublishFirmwareStatusNotificationResponse";
    case MessageType::ReportChargingProfiles:
        return "ReportChargingProfiles";
    case MessageType::ReportChargingProfilesResponse:
        return "ReportChargingProfilesResponse";
    case MessageType::RequestStartTransaction:
        return "RequestStartTransaction";
    case MessageType::RequestStartTransactionResponse:
        return "RequestStartTransactionResponse";
    case MessageType::RequestStopTransaction:
        return "RequestStopTransaction";
    case MessageType::RequestStopTransactionResponse:
        return "RequestStopTransactionResponse";
    case MessageType::ReservationStatusUpdate:
        return "ReservationStatusUpdate";
    case MessageType::ReservationStatusUpdateResponse:
        return "ReservationStatusUpdateResponse";
    case MessageType::ReserveNow:
        return "ReserveNow";
    case MessageType::ReserveNowResponse:
        return "ReserveNowResponse";
    case MessageType::Reset:
        return "Reset";
    case MessageType::ResetResponse:
        return "ResetResponse";
    case MessageType::SecurityEventNotification:
        return "SecurityEventNotification";
    case MessageType::SecurityEventNotificationResponse:
        return "SecurityEventNotificationResponse";
    case MessageType::SendLocalList:
        return "SendLocalList";
    case MessageType::SendLocalListResponse:
        return "SendLocalListResponse";
    case MessageType::SetChargingProfile:
        return "SetChargingProfile";
    case MessageType::SetChargingProfileResponse:
        return "SetChargingProfileResponse";
    case MessageType::SetDisplayMessage:
        return "SetDisplayMessage";
    case MessageType::SetDisplayMessageResponse:
        return "SetDisplayMessageResponse";
    case MessageType::SetMonitoringBase:
        return "SetMonitoringBase";
    case MessageType::SetMonitoringBaseResponse:
        return "SetMonitoringBaseResponse";
    case MessageType::SetMonitoringLevel:
        return "SetMonitoringLevel";
    case MessageType::SetMonitoringLevelResponse:
        return "SetMonitoringLevelResponse";
    case MessageType::SetNetworkProfile:
        return "SetNetworkProfile";
    case MessageType::SetNetworkProfileResponse:
        return "SetNetworkProfileResponse";
    case MessageType::SetVariableMonitoring:
        return "SetVariableMonitoring";
    case MessageType::SetVariableMonitoringResponse:
        return "SetVariableMonitoringResponse";
    case MessageType::SetVariables:
        return "SetVariables";
    case MessageType::SetVariablesResponse:
        return "SetVariablesResponse";
    case MessageType::SignCertificate:
        return "SignCertificate";
    case MessageType::SignCertificateResponse:
        return "SignCertificateResponse";
    case MessageType::StatusNotification:
        return "StatusNotification";
    case MessageType::StatusNotificationResponse:
        return "StatusNotificationResponse";
    case MessageType::TransactionEvent:
        return "TransactionEvent";
    case MessageType::TransactionEventResponse:
        return "TransactionEventResponse";
    case MessageType::TriggerMessage:
        return "TriggerMessage";
    case MessageType::TriggerMessageResponse:
        return "TriggerMessageResponse";
    case MessageType::UnlockConnector:
        return "UnlockConnector";
    case MessageType::UnlockConnectorResponse:
        return "UnlockConnectorResponse";
    case MessageType::UnpublishFirmware:
        return "UnpublishFirmware";
    case MessageType::UnpublishFirmwareResponse:
        return "UnpublishFirmwareResponse";
    case MessageType::UpdateFirmware:
        return "UpdateFirmware";
    case MessageType::UpdateFirmwareResponse:
        return "UpdateFirmwareResponse";
    case MessageType::InternalError:
        EVLOG_error << "No known string conversion for InternalError MessageType";
        return "InternalError";
    }
    throw std::out_of_range("No known string conversion for provided enum of type MessageType");
}

MessageType string_to_messagetype(const std::string& s) {
    if (s == "Authorize") {
        return MessageType::Authorize;
    } else if (s == "AuthorizeResponse") {
        return MessageType::AuthorizeResponse;
    } else if (s == "BootNotification") {
        return MessageType::BootNotification;
    } else if (s == "BootNotificationResponse") {
        return MessageType::BootNotificationResponse;
    } else if (s == "CancelReservation") {
        return MessageType::CancelReservation;
    } else if (s == "CancelReservationResponse") {
        return MessageType::CancelReservationResponse;
    } else if (s == "CertificateSigned") {
        return MessageType::CertificateSigned;
    } else if (s == "CertificateSignedResponse") {
        return MessageType::CertificateSignedResponse;
    } else if (s == "ChangeAvailability") {
        return MessageType::ChangeAvailability;
    } else if (s == "ChangeAvailabilityResponse") {
        return MessageType::ChangeAvailabilityResponse;
    } else if (s == "ClearCache") {
        return MessageType::ClearCache;
    } else if (s == "ClearCacheResponse") {
        return MessageType::ClearCacheResponse;
    } else if (s == "ClearChargingProfile") {
        return MessageType::ClearChargingProfile;
    } else if (s == "ClearChargingProfileResponse") {
        return MessageType::ClearChargingProfileResponse;
    } else if (s == "ClearDisplayMessage") {
        return MessageType::ClearDisplayMessage;
    } else if (s == "ClearDisplayMessageResponse") {
        return MessageType::ClearDisplayMessageResponse;
    } else if (s == "ClearedChargingLimit") {
        return MessageType::ClearedChargingLimit;
    } else if (s == "ClearedChargingLimitResponse") {
        return MessageType::ClearedChargingLimitResponse;
    } else if (s == "ClearVariableMonitoring") {
        return MessageType::ClearVariableMonitoring;
    } else if (s == "ClearVariableMonitoringResponse") {
        return MessageType::ClearVariableMonitoringResponse;
    } else if (s == "CostUpdated") {
        return MessageType::CostUpdated;
    } else if (s == "CostUpdatedResponse") {
        return MessageType::CostUpdatedResponse;
    } else if (s == "CustomerInformation") {
        return MessageType::CustomerInformation;
    } else if (s == "CustomerInformationResponse") {
        return MessageType::CustomerInformationResponse;
    } else if (s == "DataTransfer") {
        return MessageType::DataTransfer;
    } else if (s == "DataTransferResponse") {
        return MessageType::DataTransferResponse;
    } else if (s == "DeleteCertificate") {
        return MessageType::DeleteCertificate;
    } else if (s == "DeleteCertificateResponse") {
        return MessageType::DeleteCertificateResponse;
    } else if (s == "FirmwareStatusNotification") {
        return MessageType::FirmwareStatusNotification;
    } else if (s == "FirmwareStatusNotificationResponse") {
        return MessageType::FirmwareStatusNotificationResponse;
    } else if (s == "Get15118EVCertificate") {
        return MessageType::Get15118EVCertificate;
    } else if (s == "Get15118EVCertificateResponse") {
        return MessageType::Get15118EVCertificateResponse;
    } else if (s == "GetBaseReport") {
        return MessageType::GetBaseReport;
    } else if (s == "GetBaseReportResponse") {
        return MessageType::GetBaseReportResponse;
    } else if (s == "GetCertificateStatus") {
        return MessageType::GetCertificateStatus;
    } else if (s == "GetCertificateStatusResponse") {
        return MessageType::GetCertificateStatusResponse;
    } else if (s == "GetChargingProfiles") {
        return MessageType::GetChargingProfiles;
    } else if (s == "GetChargingProfilesResponse") {
        return MessageType::GetChargingProfilesResponse;
    } else if (s == "GetCompositeSchedule") {
        return MessageType::GetCompositeSchedule;
    } else if (s == "GetCompositeScheduleResponse") {
        return MessageType::GetCompositeScheduleResponse;
    } else if (s == "GetDisplayMessages") {
        return MessageType::GetDisplayMessages;
    } else if (s == "GetDisplayMessagesResponse") {
        return MessageType::GetDisplayMessagesResponse;
    } else if (s == "GetInstalledCertificateIds") {
        return MessageType::GetInstalledCertificateIds;
    } else if (s == "GetInstalledCertificateIdsResponse") {
        return MessageType::GetInstalledCertificateIdsResponse;
    } else if (s == "GetLocalListVersion") {
        return MessageType::GetLocalListVersion;
    } else if (s == "GetLocalListVersionResponse") {
        return MessageType::GetLocalListVersionResponse;
    } else if (s == "GetLog") {
        return MessageType::GetLog;
    } else if (s == "GetLogResponse") {
        return MessageType::GetLogResponse;
    } else if (s == "GetMonitoringReport") {
        return MessageType::GetMonitoringReport;
    } else if (s == "GetMonitoringReportResponse") {
        return MessageType::GetMonitoringReportResponse;
    } else if (s == "GetReport") {
        return MessageType::GetReport;
    } else if (s == "GetReportResponse") {
        return MessageType::GetReportResponse;
    } else if (s == "GetTransactionStatus") {
        return MessageType::GetTransactionStatus;
    } else if (s == "GetTransactionStatusResponse") {
        return MessageType::GetTransactionStatusResponse;
    } else if (s == "GetVariables") {
        return MessageType::GetVariables;
    } else if (s == "GetVariablesResponse") {
        return MessageType::GetVariablesResponse;
    } else if (s == "Heartbeat") {
        return MessageType::Heartbeat;
    } else if (s == "HeartbeatResponse") {
        return MessageType::HeartbeatResponse;
    } else if (s == "InstallCertificate") {
        return MessageType::InstallCertificate;
    } else if (s == "InstallCertificateResponse") {
        return MessageType::InstallCertificateResponse;
    } else if (s == "LogStatusNotification") {
        return MessageType::LogStatusNotification;
    } else if (s == "LogStatusNotificationResponse") {
        return MessageType::LogStatusNotificationResponse;
    } else if (s == "MeterValues") {
        return MessageType::MeterValues;
    } else if (s == "MeterValuesResponse") {
        return MessageType::MeterValuesResponse;
    } else if (s == "NotifyChargingLimit") {
        return MessageType::NotifyChargingLimit;
    } else if (s == "NotifyChargingLimitResponse") {
        return MessageType::NotifyChargingLimitResponse;
    } else if (s == "NotifyCustomerInformation") {
        return MessageType::NotifyCustomerInformation;
    } else if (s == "NotifyCustomerInformationResponse") {
        return MessageType::NotifyCustomerInformationResponse;
    } else if (s == "NotifyDisplayMessages") {
        return MessageType::NotifyDisplayMessages;
    } else if (s == "NotifyDisplayMessagesResponse") {
        return MessageType::NotifyDisplayMessagesResponse;
    } else if (s == "NotifyEVChargingNeeds") {
        return MessageType::NotifyEVChargingNeeds;
    } else if (s == "NotifyEVChargingNeedsResponse") {
        return MessageType::NotifyEVChargingNeedsResponse;
    } else if (s == "NotifyEVChargingSchedule") {
        return MessageType::NotifyEVChargingSchedule;
    } else if (s == "NotifyEVChargingScheduleResponse") {
        return MessageType::NotifyEVChargingScheduleResponse;
    } else if (s == "NotifyEvent") {
        return MessageType::NotifyEvent;
    } else if (s == "NotifyEventResponse") {
        return MessageType::NotifyEventResponse;
    } else if (s == "NotifyMonitoringReport") {
        return MessageType::NotifyMonitoringReport;
    } else if (s == "NotifyMonitoringReportResponse") {
        return MessageType::NotifyMonitoringReportResponse;
    } else if (s == "NotifyReport") {
        return MessageType::NotifyReport;
    } else if (s == "NotifyReportResponse") {
        return MessageType::NotifyReportResponse;
    } else if (s == "PublishFirmware") {
        return MessageType::PublishFirmware;
    } else if (s == "PublishFirmwareResponse") {
        return MessageType::PublishFirmwareResponse;
    } else if (s == "PublishFirmwareStatusNotification") {
        return MessageType::PublishFirmwareStatusNotification;
    } else if (s == "PublishFirmwareStatusNotificationResponse") {
        return MessageType::PublishFirmwareStatusNotificationResponse;
    } else if (s == "ReportChargingProfiles") {
        return MessageType::ReportChargingProfiles;
    } else if (s == "ReportChargingProfilesResponse") {
        return MessageType::ReportChargingProfilesResponse;
    } else if (s == "RequestStartTransaction") {
        return MessageType::RequestStartTransaction;
    } else if (s == "RequestStartTransactionResponse") {
        return MessageType::RequestStartTransactionResponse;
    } else if (s == "RequestStopTransaction") {
        return MessageType::RequestStopTransaction;
    } else if (s == "RequestStopTransactionResponse") {
        return MessageType::RequestStopTransactionResponse;
    } else if (s == "ReservationStatusUpdate") {
        return MessageType::ReservationStatusUpdate;
    } else if (s == "ReservationStatusUpdateResponse") {
        return MessageType::ReservationStatusUpdateResponse;
    } else if (s == "ReserveNow") {
        return MessageType::ReserveNow;
    } else if (s == "ReserveNowResponse") {
        return MessageType::ReserveNowResponse;
    } else if (s == "Reset") {
        return MessageType::Reset;
    } else if (s == "ResetResponse") {
        return MessageType::ResetResponse;
    } else if (s == "SecurityEventNotification") {
        return MessageType::SecurityEventNotification;
    } else if (s == "SecurityEventNotificationResponse") {
        return MessageType::SecurityEventNotificationResponse;
    } else if (s == "SendLocalList") {
        return MessageType::SendLocalList;
    } else if (s == "SendLocalListResponse") {
        return MessageType::SendLocalListResponse;
    } else if (s == "SetChargingProfile") {
        return MessageType::SetChargingProfile;
    } else if (s == "SetChargingProfileResponse") {
        return MessageType::SetChargingProfileResponse;
    } else if (s == "SetDisplayMessage") {
        return MessageType::SetDisplayMessage;
    } else if (s == "SetDisplayMessageResponse") {
        return MessageType::SetDisplayMessageResponse;
    } else if (s == "SetMonitoringBase") {
        return MessageType::SetMonitoringBase;
    } else if (s == "SetMonitoringBaseResponse") {
        return MessageType::SetMonitoringBaseResponse;
    } else if (s == "SetMonitoringLevel") {
        return MessageType::SetMonitoringLevel;
    } else if (s == "SetMonitoringLevelResponse") {
        return MessageType::SetMonitoringLevelResponse;
    } else if (s == "SetNetworkProfile") {
        return MessageType::SetNetworkProfile;
    } else if (s == "SetNetworkProfileResponse") {
        return MessageType::SetNetworkProfileResponse;
    } else if (s == "SetVariableMonitoring") {
        return MessageType::SetVariableMonitoring;
    } else if (s == "SetVariableMonitoringResponse") {
        return MessageType::SetVariableMonitoringResponse;
    } else if (s == "SetVariables") {
        return MessageType::SetVariables;
    } else if (s == "SetVariablesResponse") {
        return MessageType::SetVariablesResponse;
    } else if (s == "SignCertificate") {
        return MessageType::SignCertificate;
    } else if (s == "SignCertificateResponse") {
        return MessageType::SignCertificateResponse;
    } else if (s == "StatusNotification") {
        return MessageType::StatusNotification;
    } else if (s == "StatusNotificationResponse") {
        return MessageType::StatusNotificationResponse;
    } else if (s == "TransactionEvent") {
        return MessageType::TransactionEvent;
    } else if (s == "TransactionEventResponse") {
        return MessageType::TransactionEventResponse;
    } else if (s == "TriggerMessage") {
        return MessageType::TriggerMessage;
    } else if (s == "TriggerMessageResponse") {
        return MessageType::TriggerMessageResponse;
    } else if (s == "UnlockConnector") {
        return MessageType::UnlockConnector;
    } else if (s == "UnlockConnectorResponse") {
        return MessageType::UnlockConnectorResponse;
    } else if (s == "UnpublishFirmware") {
        return MessageType::UnpublishFirmware;
    } else if (s == "UnpublishFirmwareResponse") {
        return MessageType::UnpublishFirmwareResponse;
    } else if (s == "UpdateFirmware") {
        return MessageType::UpdateFirmware;
    } else if (s == "UpdateFirmwareResponse") {
        return MessageType::UpdateFirmwareResponse;
    } else if (s == "InternalError") {
        return MessageType::InternalError;
    }
    throw std::out_of_range("Provided string " + s + " could not be converted to enum of type MessageType");
}
} // namespace conversions

std::ostream& operator<<(std::ostream& os, const MessageType& message_type) {
    os << conversions::messagetype_to_string(message_type);
    return os;
}

} // namespace v201
} // namespace ocpp
