// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef V201_TYPES_HPP
#define V201_TYPES_HPP

#include <ocpp/v201/ocpp_types.hpp>

#include <ostream>
#include <string>

namespace ocpp {
namespace v201 {

/// \brief Contains all supported OCPP 2.0.1 message types
enum class MessageType {
    Authorize,
    AuthorizeResponse,
    BootNotification,
    BootNotificationResponse,
    CancelReservation,
    CancelReservationResponse,
    CertificateSigned,
    CertificateSignedResponse,
    ChangeAvailability,
    ChangeAvailabilityResponse,
    ClearCache,
    ClearCacheResponse,
    ClearChargingProfile,
    ClearChargingProfileResponse,
    ClearDisplayMessage,
    ClearDisplayMessageResponse,
    ClearedChargingLimit,
    ClearedChargingLimitResponse,
    ClearVariableMonitoring,
    ClearVariableMonitoringResponse,
    CostUpdated,
    CostUpdatedResponse,
    CustomerInformation,
    CustomerInformationResponse,
    DataTransfer,
    DataTransferResponse,
    DeleteCertificate,
    DeleteCertificateResponse,
    FirmwareStatusNotification,
    FirmwareStatusNotificationResponse,
    Get15118EVCertificate,
    Get15118EVCertificateResponse,
    GetBaseReport,
    GetBaseReportResponse,
    GetCertificateStatus,
    GetCertificateStatusResponse,
    GetChargingProfiles,
    GetChargingProfilesResponse,
    GetCompositeSchedule,
    GetCompositeScheduleResponse,
    GetDisplayMessages,
    GetDisplayMessagesResponse,
    GetInstalledCertificateIds,
    GetInstalledCertificateIdsResponse,
    GetLocalListVersion,
    GetLocalListVersionResponse,
    GetLog,
    GetLogResponse,
    GetMonitoringReport,
    GetMonitoringReportResponse,
    GetReport,
    GetReportResponse,
    GetTransactionStatus,
    GetTransactionStatusResponse,
    GetVariables,
    GetVariablesResponse,
    Heartbeat,
    HeartbeatResponse,
    InstallCertificate,
    InstallCertificateResponse,
    LogStatusNotification,
    LogStatusNotificationResponse,
    MeterValues,
    MeterValuesResponse,
    NotifyChargingLimit,
    NotifyChargingLimitResponse,
    NotifyCustomerInformation,
    NotifyCustomerInformationResponse,
    NotifyDisplayMessages,
    NotifyDisplayMessagesResponse,
    NotifyEVChargingNeeds,
    NotifyEVChargingNeedsResponse,
    NotifyEVChargingSchedule,
    NotifyEVChargingScheduleResponse,
    NotifyEvent,
    NotifyEventResponse,
    NotifyMonitoringReport,
    NotifyMonitoringReportResponse,
    NotifyReport,
    NotifyReportResponse,
    PublishFirmware,
    PublishFirmwareResponse,
    PublishFirmwareStatusNotification,
    PublishFirmwareStatusNotificationResponse,
    ReportChargingProfiles,
    ReportChargingProfilesResponse,
    RequestStartTransaction,
    RequestStartTransactionResponse,
    RequestStopTransaction,
    RequestStopTransactionResponse,
    ReservationStatusUpdate,
    ReservationStatusUpdateResponse,
    ReserveNow,
    ReserveNowResponse,
    Reset,
    ResetResponse,
    SecurityEventNotification,
    SecurityEventNotificationResponse,
    SendLocalList,
    SendLocalListResponse,
    SetChargingProfile,
    SetChargingProfileResponse,
    SetDisplayMessage,
    SetDisplayMessageResponse,
    SetMonitoringBase,
    SetMonitoringBaseResponse,
    SetMonitoringLevel,
    SetMonitoringLevelResponse,
    SetNetworkProfile,
    SetNetworkProfileResponse,
    SetVariableMonitoring,
    SetVariableMonitoringResponse,
    SetVariables,
    SetVariablesResponse,
    SignCertificate,
    SignCertificateResponse,
    StatusNotification,
    StatusNotificationResponse,
    TransactionEvent,
    TransactionEventResponse,
    TriggerMessage,
    TriggerMessageResponse,
    UnlockConnector,
    UnlockConnectorResponse,
    UnpublishFirmware,
    UnpublishFirmwareResponse,
    UpdateFirmware,
    UpdateFirmwareResponse,
    InternalError, // not in spec, for internal use
};

/// \brief This enhances the ChargingProfile type by additional paramaters that are required in the
/// ReportChargingProfilesRequest (EvseId, ChargingLimitSource)
struct ReportedChargingProfile {
    ChargingProfile profile;
    int32_t evse_id;
    CiString<20> source;

    ReportedChargingProfile(const ChargingProfile& profile, const int32_t evse_id, const CiString<20> source) :
        profile(profile), evse_id(evse_id), source(source) {
    }
};

namespace conversions {
/// \brief Converts the given MessageType \p m to std::string
/// \returns a string representation of the MessageType
std::string messagetype_to_string(MessageType m);

/// \brief Converts the given std::string \p s to MessageType
/// \returns a MessageType from a string representation
MessageType string_to_messagetype(const std::string& s);

} // namespace conversions

/// \brief Writes the string representation of the given \p message_type to the given output stream \p os
/// \returns an output stream with the MessageType written to
std::ostream& operator<<(std::ostream& os, const MessageType& message_type);

namespace ChargingLimitSourceEnumStringType {
inline const CiString<20> EMS = "EMS";
inline const CiString<20> OTHER = "Other";
inline const CiString<20> SO = "SO";
inline const CiString<20> CSO = "CSO";
} // namespace ChargingLimitSourceEnumStringType

namespace IdTokenEnumStringType {
inline const CiString<20> Value = "Value";
inline const CiString<20> Central = "Central";
inline const CiString<20> DirectPayment = "DirectPayment";
inline const CiString<20> eMAID = "eMAID";
inline const CiString<20> EVCCID = "EVCCID";
inline const CiString<20> ISO14443 = "ISO14443";
inline const CiString<20> ISO15693 = "ISO15693";
inline const CiString<20> KeyCode = "KeyCode";
inline const CiString<20> Local = "Local";
inline const CiString<20> MacAddress = "MacAddress";
inline const CiString<20> NEMA = "NEMA";
inline const CiString<20> NoAuthorization = "NoAuthorization";
inline const CiString<20> VIN = "VIN";
} // namespace IdTokenEnumStringType
} // namespace v201
} // namespace ocpp

#endif
