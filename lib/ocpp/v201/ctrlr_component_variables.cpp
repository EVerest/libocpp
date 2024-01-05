// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 -  Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/ctrlr_component_variables.hpp>

namespace ocpp {
namespace v201 {

namespace ControllerComponents {
const Component& InternalCtrlr = {"InternalCtrlr"};
const Component& AlignedDataCtrlr = {"AlignedDataCtrlr"};
const Component& AuthCacheCtrlr = {"AuthCacheCtrlr"};
const Component& AuthCtrlr = {"AuthCtrlr"};
const Component& ChargingStation = {"ChargingStation"};
const Component& ClockCtrlr = {"ClockCtrlr"};
const Component& Connector = {"Connector"};
const Component& CustomizationCtrlr = {"CustomizationCtrlr"};
const Component& DeviceDataCtrlr = {"DeviceDataCtrlr"};
const Component& DisplayMessageCtrlr = {"DisplayMessageCtrlr"};
const Component& EVSE = {"EVSE"};
const Component& ISO15118Ctrlr = {"ISO15118Ctrlr"};
const Component& LocalAuthListCtrlr = {"LocalAuthListCtrlr"};
const Component& MonitoringCtrlr = {"MonitoringCtrlr"};
const Component& OCPPCommCtrlr = {"OCPPCommCtrlr"};
const Component& ReservationCtrlr = {"ReservationCtrlr"};
const Component& SampledDataCtrlr = {"SampledDataCtrlr"};
const Component& SecurityCtrlr = {"SecurityCtrlr"};
const Component& SmartChargingCtrlr = {"SmartChargingCtrlr"};
const Component& TariffCostCtrlr = {"TariffCostCtrlr"};
const Component& TxCtrlr = {"TxCtrlr"};
} // namespace ControllerComponents

namespace ControllerComponentVariables {

const ComponentVariable& InternalCtrlrEnabled = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const RequiredComponentVariable& ChargePointId = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ChargePointId",
    }),
};
const RequiredComponentVariable& NetworkConnectionProfiles = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NetworkConnectionProfiles",
    }),
};
const RequiredComponentVariable& ChargeBoxSerialNumber = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ChargeBoxSerialNumber",
    }),
};
const RequiredComponentVariable& ChargePointModel = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ChargePointModel",
    }),
};
const ComponentVariable& ChargePointSerialNumber = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ChargePointSerialNumber",
    }),
};
const RequiredComponentVariable& ChargePointVendor = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ChargePointVendor",
    }),
};
const RequiredComponentVariable& FirmwareVersion = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "FirmwareVersion",
    }),
};
const ComponentVariable& ICCID = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ICCID",
    }),
};
const ComponentVariable& IMSI = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "IMSI",
    }),
};
const ComponentVariable& MeterSerialNumber = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MeterSerialNumber",
    }),
};
const ComponentVariable& MeterType = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MeterType",
    }),
};
const RequiredComponentVariable& SupportedCiphers12 = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SupportedCiphers12",
    }),
};
const RequiredComponentVariable& SupportedCiphers13 = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SupportedCiphers13",
    }),
};
const ComponentVariable& AuthorizeConnectorZeroOnConnectorOne = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "AuthorizeConnectorZeroOnConnectorOne",
    }),
};
const ComponentVariable& LogMessages = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LogMessages",
    }),
};
const RequiredComponentVariable& LogMessagesFormat = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LogMessagesFormat",
    }),
};
const ComponentVariable& SupportedChargingProfilePurposeTypes = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SupportedChargingProfilePurposeTypes",
    }),
};
const ComponentVariable& MaxCompositeScheduleDuration = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MaxCompositeScheduleDuration",
    }),
};
const RequiredComponentVariable& NumberOfConnectors = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NumberOfConnectors",
    }),
};
const ComponentVariable& UseSslDefaultVerifyPaths = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "UseSslDefaultVerifyPaths",
    }),
};
const ComponentVariable& OcspRequestInterval = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "OcspRequestInterval",
    }),
};
const ComponentVariable& WebsocketPingPayload = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "WebsocketPingPayload",
    }),
};
const ComponentVariable& WebsocketPongTimeout = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "WebsocketPongTimeout",
    }),
};
const ComponentVariable& MaxCustomerInformationDataLength = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MaxCustomerInformationDataLength",
    }),
};
const ComponentVariable& V2GCertificateExpireCheckInitialDelaySeconds = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "V2GCertificateExpireCheckInitialDelaySeconds",
    }),
};
const ComponentVariable& V2GCertificateExpireCheckIntervalSeconds = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "V2GCertificateExpireCheckIntervalSeconds",
    }),
};
const ComponentVariable& ClientCertificateExpireCheckInitialDelaySeconds = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ClientCertificateExpireCheckInitialDelaySeconds",
    }),
};
const ComponentVariable& ClientCertificateExpireCheckIntervalSeconds = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ClientCertificateExpireCheckIntervalSeconds",
    }),
};
const ComponentVariable& MessageQueueSizeThreshold = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MessageQueueSizeThreshold",
    }),
};
const ComponentVariable& MaxMessageSize = {
    ControllerComponents::InternalCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MaxMessageSize",
    }),
};
const ComponentVariable& AlignedDataCtrlrEnabled = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& AlignedDataCtrlrAvailable = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& AlignedDataInterval = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Interval",
    }),
};
const RequiredComponentVariable& AlignedDataMeasurands = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Measurands",
    }),
};
const ComponentVariable& AlignedDataSendDuringIdle = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SendDuringIdle",
    }),
};
const ComponentVariable& AlignedDataSignReadings = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SignReadings",
    }),
};
const RequiredComponentVariable& AlignedDataTxEndedInterval = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxEndedInterval",
    }),
};
const RequiredComponentVariable& AlignedDataTxEndedMeasurands = {
    ControllerComponents::AlignedDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxEndedMeasurands",
    }),
};
const ComponentVariable& AuthCacheCtrlrAvailable = {
    ControllerComponents::AuthCacheCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const ComponentVariable& AuthCacheCtrlrEnabled = {
    ControllerComponents::AuthCacheCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& AuthCacheLifeTime = {
    ControllerComponents::AuthCacheCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LifeTime",
    }),
};
const ComponentVariable& AuthCachePolicy = {
    ControllerComponents::AuthCacheCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Policy",
    }),
};
const ComponentVariable& AuthCacheStorage = {
    ControllerComponents::AuthCacheCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Storage",
    }),
};
const ComponentVariable& AuthCtrlrEnabled = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& AdditionalInfoItemsPerMessage = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "AdditionalInfoItemsPerMessage",
    }),
};
const RequiredComponentVariable& AuthorizeRemoteStart = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "AuthorizeRemoteStart",
    }),
};
const RequiredComponentVariable& LocalAuthorizeOffline = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LocalAuthorizeOffline",
    }),
};
const RequiredComponentVariable& LocalPreAuthorize = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LocalPreAuthorize",
    }),
};
const ComponentVariable& MasterPassGroupId = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MasterPassGroupId",
    }),
};
const ComponentVariable& OfflineTxForUnknownIdEnabled = {
    ControllerComponents::AuthCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "OfflineTxForUnknownIdEnabled",
    }),
};
const ComponentVariable& AllowNewSessionsPendingFirmwareUpdate = {
    ControllerComponents::ChargingStation,
    std::nullopt,
    std::optional<Variable>({"AllowNewSessionsPendingFirmwareUpdate", std::nullopt, "BytesPerMessage"}),
};
const RequiredComponentVariable& ChargingStationAvailabilityState = {
    ControllerComponents::ChargingStation,
    std::nullopt,
    std::optional<Variable>({
        "AvailabilityState",
    }),
};
const RequiredComponentVariable& ChargingStationAvailable = {
    ControllerComponents::ChargingStation,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& ChargingStationSupplyPhases = {
    ControllerComponents::ChargingStation,
    std::nullopt,
    std::optional<Variable>({
        "SupplyPhases",
    }),
};
const RequiredComponentVariable& ClockCtrlrDateTime = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "DateTime",
    }),
};
const ComponentVariable& NextTimeOffsetTransitionDateTime = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NextTimeOffsetTransitionDateTime",
    }),
};
const ComponentVariable& NtpServerUri = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NtpServerUri",
    }),
};
const ComponentVariable& NtpSource = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NtpSource",
    }),
};
const ComponentVariable& TimeAdjustmentReportingThreshold = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TimeAdjustmentReportingThreshold",
    }),
};
const ComponentVariable& TimeOffset = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TimeOffset",
    }),
};
const ComponentVariable& TimeOffsetNextTransition = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({"TimeOffset", std::nullopt, "NextTransition"}),
};
const RequiredComponentVariable& TimeSource = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TimeSource",
    }),
};
const ComponentVariable& TimeZone = {
    ControllerComponents::ClockCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TimeZone",
    }),
};
const RequiredComponentVariable& ConnectorAvailable = {
    ControllerComponents::Connector,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& ConnectorType = {
    ControllerComponents::Connector,
    std::nullopt,
    std::optional<Variable>({
        "ConnectorType",
    }),
};
const RequiredComponentVariable& ConnectorSupplyPhases = {
    ControllerComponents::Connector,
    std::nullopt,
    std::optional<Variable>({
        "SupplyPhases",
    }),
};
const ComponentVariable& CustomImplementationEnabled = {
    ControllerComponents::CustomizationCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "CustomImplementationEnabled",
    }),
};
const RequiredComponentVariable& BytesPerMessageGetReport = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"BytesPerMessage", std::nullopt, "GetReport"}),
};
const RequiredComponentVariable& BytesPerMessageGetVariables = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"BytesPerMessage", std::nullopt, "GetVariables"}),
};
const RequiredComponentVariable& BytesPerMessageSetVariables = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"BytesPerMessage", std::nullopt, "SetVariables"}),
};
const ComponentVariable& ConfigurationValueSize = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ConfigurationValueSize",
    }),
};
const RequiredComponentVariable& ItemsPerMessageGetReport = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"ItemsPerMessage", std::nullopt, "GetReport"}),
};
const RequiredComponentVariable& ItemsPerMessageGetVariables = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"ItemsPerMessage", std::nullopt, "GetVariables"}),
};
const RequiredComponentVariable& ItemsPerMessageSetVariables = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({"ItemsPerMessage", std::nullopt, "SetVariables"}),
};
const ComponentVariable& ReportingValueSize = {
    ControllerComponents::DeviceDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ReportingValueSize",
    }),
};
const ComponentVariable& DisplayMessageCtrlrAvailable = {
    ControllerComponents::DisplayMessageCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& NumberOfDisplayMessages = {
    ControllerComponents::DisplayMessageCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NumberOfDisplayMessages",
    }),
};
const RequiredComponentVariable& DisplayMessageSupportedFormats = {
    ControllerComponents::DisplayMessageCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SupportedFormats",
    }),
};
const RequiredComponentVariable& DisplayMessageSupportedPriorities = {
    ControllerComponents::DisplayMessageCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SupportedPriorities",
    }),
};
const ComponentVariable& EVSEAllowReset = {
    ControllerComponents::EVSE,
    std::nullopt,
    std::optional<Variable>({
        "AllowReset",
    }),
};
const RequiredComponentVariable& EVSEAvailable = {
    ControllerComponents::EVSE,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& EVSEPower = {
    ControllerComponents::EVSE,
    std::nullopt,
    std::optional<Variable>({
        "Power",
    }),
};
const RequiredComponentVariable& EVSESupplyPhases = {
    ControllerComponents::EVSE,
    std::nullopt,
    std::optional<Variable>({
        "SupplyPhases",
    }),
};
const ComponentVariable& CentralContractValidationAllowed = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "CentralContractValidationAllowed",
    }),
};
const RequiredComponentVariable& ContractValidationOffline = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "ContractValidationOffline",
    }),
};
const ComponentVariable& RequestMeteringReceipt = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "RequestMeteringReceipt",
    }),
};
const ComponentVariable& ISO15118CtrlrSeccId = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "SeccId",
    }),
};
const ComponentVariable& ISO15118CtrlrCountryName = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "CountryName",
    }),
};
const ComponentVariable& ISO15118CtrlrOrganizationName = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "OrganizationName",
    }),
};
const ComponentVariable& PnCEnabled = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "PnCEnabled",
    }),
};
const ComponentVariable& V2GCertificateInstallationEnabled = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "V2GCertificateInstallationEnabled",
    }),
};
const ComponentVariable& ContractCertificateInstallationEnabled = {
    ControllerComponents::ISO15118Ctrlr,
    std::nullopt,
    std::optional<Variable>({
        "ContractCertificateInstallationEnabled",
    }),
};
const ComponentVariable& LocalAuthListCtrlrAvailable = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const RequiredComponentVariable& BytesPerMessageSendLocalList = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "BytesPerMessage",
    }),
};
const ComponentVariable& LocalAuthListCtrlrEnabled = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const RequiredComponentVariable& LocalAuthListCtrlrEntries = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Entries",
    }),
};
const RequiredComponentVariable& ItemsPerMessageSendLocalList = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ItemsPerMessage",
    }),
};
const ComponentVariable& LocalAuthListCtrlrStorage = {
    ControllerComponents::LocalAuthListCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Storage",
    }),
};
const ComponentVariable& MonitoringCtrlrAvailable = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const ComponentVariable& BytesPerMessageClearVariableMonitoring = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({"BytesPerMessage", std::nullopt, "ClearVariableMonitoring"}),
};
const RequiredComponentVariable& BytesPerMessageSetVariableMonitoring = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({"BytesPerMessage", std::nullopt, "SetVariableMonitoring"}),
};
const ComponentVariable& MonitoringCtrlrEnabled = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& ItemsPerMessageClearVariableMonitoring = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({"ItemsPerMessage", std::nullopt, "ClearVariableMonitoring"}),
};
const RequiredComponentVariable& ItemsPerMessageSetVariableMonitoring = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({"ItemsPerMessage", std::nullopt, "SetVariableMonitoring"}),
};
const ComponentVariable& OfflineQueuingSeverity = {
    ControllerComponents::MonitoringCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "OfflineQueuingSeverity",
    }),
};
const ComponentVariable& ActiveNetworkProfile = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ActiveNetworkProfile",
    }),
};
const RequiredComponentVariable& FileTransferProtocols = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "FileTransferProtocols",
    }),
};
const ComponentVariable& HeartbeatInterval = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "HeartbeatInterval",
    }),
};
const RequiredComponentVariable& MessageTimeout = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({"MessageTimeout", std::nullopt, "Default"}),
};
const RequiredComponentVariable& MessageAttemptInterval = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({"MessageAttemptInterval", std::nullopt, "TransactionEvent"}),
};
const RequiredComponentVariable& MessageAttempts = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({"MessageAttempts", std::nullopt, "TransactionEvent"}),
};
const RequiredComponentVariable& NetworkConfigurationPriority = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NetworkConfigurationPriority",
    }),
};
const RequiredComponentVariable& NetworkProfileConnectionAttempts = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NetworkProfileConnectionAttempts",
    }),
};
const RequiredComponentVariable& OfflineThreshold = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "OfflineThreshold",
    }),
};
const ComponentVariable& QueueAllMessages = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "QueueAllMessages",
    }),
};
const RequiredComponentVariable& ResetRetries = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ResetRetries",
    }),
};
const RequiredComponentVariable& RetryBackOffRandomRange = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "RetryBackOffRandomRange",
    }),
};
const RequiredComponentVariable& RetryBackOffRepeatTimes = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "RetryBackOffRepeatTimes",
    }),
};
const RequiredComponentVariable& RetryBackOffWaitMinimum = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "RetryBackOffWaitMinimum",
    }),
};
const RequiredComponentVariable& UnlockOnEVSideDisconnect = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "UnlockOnEVSideDisconnect",
    }),
};
const RequiredComponentVariable& WebSocketPingInterval = {
    ControllerComponents::OCPPCommCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "WebSocketPingInterval",
    }),
};
const ComponentVariable& ReservationCtrlrAvailable = {
    ControllerComponents::ReservationCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const ComponentVariable& ReservationCtrlrEnabled = {
    ControllerComponents::ReservationCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& ReservationCtrlrNonEvseSpecific = {
    ControllerComponents::ReservationCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NonEvseSpecific",
    }),
};
const ComponentVariable& SampledDataCtrlrAvailable = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const ComponentVariable& SampledDataCtrlrEnabled = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const ComponentVariable& SampledDataSignReadings = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SignReadings",
    }),
};
const RequiredComponentVariable& SampledDataTxEndedInterval = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxEndedInterval",
    }),
};
const RequiredComponentVariable& SampledDataTxEndedMeasurands = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxEndedMeasurands",
    }),
};
const RequiredComponentVariable& SampledDataTxStartedMeasurands = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxStartedMeasurands",
    }),
};
const RequiredComponentVariable& SampledDataTxUpdatedInterval = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxUpdatedInterval",
    }),
};
const RequiredComponentVariable& SampledDataTxUpdatedMeasurands = {
    ControllerComponents::SampledDataCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxUpdatedMeasurands",
    }),
};
const ComponentVariable& AdditionalRootCertificateCheck = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "AdditionalRootCertificateCheck",
    }),
};
const ComponentVariable& BasicAuthPassword = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "BasicAuthPassword",
    }),
};
const RequiredComponentVariable& CertificateEntries = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "CertificateEntries",
    }),
};
const ComponentVariable& CertSigningRepeatTimes = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "CertSigningRepeatTimes",
    }),
};
const ComponentVariable& CertSigningWaitMinimum = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "CertSigningWaitMinimum",
    }),
};
const RequiredComponentVariable& SecurityCtrlrIdentity = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Identity",
    }),
};
const ComponentVariable& MaxCertificateChainSize = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MaxCertificateChainSize",
    }),
};
const RequiredComponentVariable& OrganizationName = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "OrganizationName",
    }),
};
const RequiredComponentVariable& SecurityProfile = {
    ControllerComponents::SecurityCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "SecurityProfile",
    }),
};
const ComponentVariable& ACPhaseSwitchingSupported = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ACPhaseSwitchingSupported",
    }),
};
const ComponentVariable& SmartChargingCtrlrAvailable = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Available",
    }),
};
const ComponentVariable& SmartChargingCtrlrAvailableEnabled = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Enabled",
    }),
};
const RequiredComponentVariable& EntriesChargingProfiles = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({"Entries", std::nullopt, "ChargingProfiles"}),
};
const ComponentVariable& ExternalControlSignalsEnabled = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ExternalControlSignalsEnabled",
    }),
};
const RequiredComponentVariable& LimitChangeSignificance = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "LimitChangeSignificance",
    }),
};
const ComponentVariable& NotifyChargingLimitWithSchedules = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "NotifyChargingLimitWithSchedules",
    }),
};
const RequiredComponentVariable& PeriodsPerSchedule = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "PeriodsPerSchedule",
    }),
};
const ComponentVariable& Phases3to1 = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Phases3to1",
    }),
};
const RequiredComponentVariable& ChargingProfileMaxStackLevel = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "ProfileStackLevel",
    }),
};
const RequiredComponentVariable& ChargingScheduleChargingRateUnit = {
    ControllerComponents::SmartChargingCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "RateUnit",
    }),
};
const ComponentVariable& TariffCostCtrlrAvailableTariff = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({"Available", std::nullopt, "Tariff"}),
};
const ComponentVariable& TariffCostCtrlrAvailableCost = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({"Available", std::nullopt, "Cost"}),
};
const RequiredComponentVariable& TariffCostCtrlrCurrency = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "Currency",
    }),
};
const ComponentVariable& TariffCostCtrlrEnabledTariff = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({"Enabled", std::nullopt, "Tariff"}),
};
const ComponentVariable& TariffCostCtrlrEnabledCost = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({"Enabled", std::nullopt, "Cost"}),
};
const RequiredComponentVariable& TariffFallbackMessage = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TariffFallbackMessage",
    }),
};
const RequiredComponentVariable& TotalCostFallbackMessage = {
    ControllerComponents::TariffCostCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TotalCostFallbackMessage",
    }),
};
const RequiredComponentVariable& EVConnectionTimeOut = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "EVConnectionTimeOut",
    }),
};
const ComponentVariable& MaxEnergyOnInvalidId = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "MaxEnergyOnInvalidId",
    }),
};
const RequiredComponentVariable& StopTxOnEVSideDisconnect = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "StopTxOnEVSideDisconnect",
    }),
};
const RequiredComponentVariable& StopTxOnInvalidId = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "StopTxOnInvalidId",
    }),
};
const ComponentVariable& TxBeforeAcceptedEnabled = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxBeforeAcceptedEnabled",
    }),
};
const RequiredComponentVariable& TxStartPoint = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxStartPoint",
    }),
};
const RequiredComponentVariable& TxStopPoint = {
    ControllerComponents::TxCtrlr,
    std::nullopt,
    std::optional<Variable>({
        "TxStopPoint",
    }),
};

} // namespace ControllerComponentVariables
} // namespace v201
} // namespace ocpp
