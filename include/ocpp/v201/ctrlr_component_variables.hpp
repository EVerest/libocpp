// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 -  Pionix GmbH and Contributors to EVerest

#ifndef OCPP_V201_CTRLR_COMPONENT_VARIABLES
#define OCPP_V201_CTRLR_COMPONENT_VARIABLES

#include <ocpp/v201/ocpp_types.hpp>

namespace ocpp {
namespace v201 {

namespace ControllerComponents {
extern const Component& InternalCtrlr;
extern const Component& AlignedDataCtrlr;
extern const Component& AuthCacheCtrlr;
extern const Component& AuthCtrlr;
extern const Component& ChargingStation;
extern const Component& ClockCtrlr;
extern const Component& Connector;
extern const Component& CustomizationCtrlr;
extern const Component& DeviceDataCtrlr;
extern const Component& DisplayMessageCtrlr;
extern const Component& EVSE;
extern const Component& ISO15118Ctrlr;
extern const Component& LocalAuthListCtrlr;
extern const Component& MonitoringCtrlr;
extern const Component& OCPPCommCtrlr;
extern const Component& ReservationCtrlr;
extern const Component& SampledDataCtrlr;
extern const Component& SecurityCtrlr;
extern const Component& SmartChargingCtrlr;
extern const Component& TariffCostCtrlr;
extern const Component& TxCtrlr;
} // namespace ControllerComponents

// Provides access to standardized variables of OCPP2.0.1 spec
namespace ControllerComponentVariables {
extern const ComponentVariable& InternalCtrlrEnabled;
extern const ComponentVariable& ChargePointId;
extern const ComponentVariable& NetworkConnectionProfiles;
extern const ComponentVariable& ChargeBoxSerialNumber;
extern const ComponentVariable& ChargePointModel;
extern const ComponentVariable& ChargePointSerialNumber;
extern const ComponentVariable& ChargePointVendor;
extern const ComponentVariable& FirmwareVersion;
extern const ComponentVariable& ICCID;
extern const ComponentVariable& IMSI;
extern const ComponentVariable& MeterSerialNumber;
extern const ComponentVariable& MeterType;
extern const ComponentVariable& SupportedCiphers12;
extern const ComponentVariable& SupportedCiphers13;
extern const ComponentVariable& AuthorizeConnectorZeroOnConnectorOne;
extern const ComponentVariable& LogMessages;
extern const ComponentVariable& LogMessagesFormat;
extern const ComponentVariable& SupportedChargingProfilePurposeTypes;
extern const ComponentVariable& MaxCompositeScheduleDuration;
extern const ComponentVariable& NumberOfConnectors;
extern const ComponentVariable& UseSslDefaultVerifyPaths;
extern const ComponentVariable& OcspRequestInterval;
extern const ComponentVariable& WebsocketPingPayload;
extern const ComponentVariable& WebsocketPongTimeout;
extern const ComponentVariable& AlignedDataCtrlrEnabled;
extern const ComponentVariable& AlignedDataCtrlrAvailable;
extern const ComponentVariable& AlignedDataInterval;
extern const ComponentVariable& AlignedDataMeasurands;
extern const ComponentVariable& AlignedDataSendDuringIdle;
extern const ComponentVariable& AlignedDataSignReadings;
extern const ComponentVariable& AlignedDataTxEndedInterval;
extern const ComponentVariable& AlignedDataTxEndedMeasurands;
extern const ComponentVariable& AuthCacheCtrlrAvailable;
extern const ComponentVariable& AuthCacheCtrlrEnabled;
extern const ComponentVariable& AuthCacheLifeTime;
extern const ComponentVariable& AuthCachePolicy;
extern const ComponentVariable& AuthCacheStorage;
extern const ComponentVariable& AuthCtrlrEnabled;
extern const ComponentVariable& AdditionalInfoItemsPerMessage;
extern const ComponentVariable& AuthorizeRemoteStart;
extern const ComponentVariable& LocalAuthorizeOffline;
extern const ComponentVariable& LocalPreAuthorize;
extern const ComponentVariable& MasterPassGroupId;
extern const ComponentVariable& OfflineTxForUnknownIdEnabled;
extern const ComponentVariable& AllowNewSessionsPendingFirmwareUpdate;
extern const ComponentVariable& ChargingStationAvailabilityState;
extern const ComponentVariable& ChargingStationAvailable;
extern const ComponentVariable& ChargingStationSupplyPhases;
extern const ComponentVariable& ClockCtrlrDateTime;
extern const ComponentVariable& NextTimeOffsetTransitionDateTime;
extern const ComponentVariable& NtpServerUri;
extern const ComponentVariable& NtpSource;
extern const ComponentVariable& TimeAdjustmentReportingThreshold;
extern const ComponentVariable& TimeOffset;
extern const ComponentVariable& TimeOffsetNextTransition;
extern const ComponentVariable& TimeSource;
extern const ComponentVariable& TimeZone;
extern const ComponentVariable& ConnectorAvailable;
extern const ComponentVariable& ConnectorType;
extern const ComponentVariable& ConnectorSupplyPhases;
extern const ComponentVariable& CustomImplementationEnabled;
extern const ComponentVariable& BytesPerMessageGetReport;
extern const ComponentVariable& BytesPerMessageGetVariables;
extern const ComponentVariable& BytesPerMessageSetVariables;
extern const ComponentVariable& ConfigurationValueSize;
extern const ComponentVariable& ItemsPerMessageGetReport;
extern const ComponentVariable& ItemsPerMessageGetVariables;
extern const ComponentVariable& ItemsPerMessageSetVariables;
extern const ComponentVariable& ReportingValueSize;
extern const ComponentVariable& DisplayMessageCtrlrAvailable;
extern const ComponentVariable& NumberOfDisplayMessages;
extern const ComponentVariable& DisplayMessageSupportedFormats;
extern const ComponentVariable& DisplayMessageSupportedPriorities;
extern const ComponentVariable& EVSEAllowReset;
extern const ComponentVariable& EVSEAvailable;
extern const ComponentVariable& EVSEPower;
extern const ComponentVariable& EVSESupplyPhases;
extern const ComponentVariable& CentralContractValidationAllowed;
extern const ComponentVariable& ContractValidationOffline;
extern const ComponentVariable& RequestMeteringReceipt;
extern const ComponentVariable& ISO15118CtrlrCountryName;
extern const ComponentVariable& ISO15118CtrlrOrganizationName;
extern const ComponentVariable& PnCEnabled;
extern const ComponentVariable& V2GCertificateInstallationEnabled;
extern const ComponentVariable& ContractCertificateInstallationEnabled;
extern const ComponentVariable& LocalAuthListCtrlrAvailable;
extern const ComponentVariable& BytesPerMessageSendLocalList;
extern const ComponentVariable& LocalAuthListCtrlrEnabled;
extern const ComponentVariable& LocalAuthListCtrlrEntries;
extern const ComponentVariable& ItemsPerMessageSendLocalList;
extern const ComponentVariable& LocalAuthListCtrlrStorage;
extern const ComponentVariable& MonitoringCtrlrAvailable;
extern const ComponentVariable& BytesPerMessageClearVariableMonitoring;
extern const ComponentVariable& BytesPerMessageSetVariableMonitoring;
extern const ComponentVariable& MonitoringCtrlrEnabled;
extern const ComponentVariable& ItemsPerMessageClearVariableMonitoring;
extern const ComponentVariable& ItemsPerMessageSetVariableMonitoring;
extern const ComponentVariable& OfflineQueuingSeverity;
extern const ComponentVariable& ActiveNetworkProfile;
extern const ComponentVariable& FileTransferProtocols;
extern const ComponentVariable& HeartbeatInterval;
extern const ComponentVariable& MessageTimeout;
extern const ComponentVariable& MessageAttemptInterval;
extern const ComponentVariable& MessageAttempts;
extern const ComponentVariable& NetworkConfigurationPriority;
extern const ComponentVariable& NetworkProfileConnectionAttempts;
extern const ComponentVariable& OfflineThreshold;
extern const ComponentVariable& QueueAllMessages;
extern const ComponentVariable& ResetRetries;
extern const ComponentVariable& RetryBackOffRandomRange;
extern const ComponentVariable& RetryBackOffRepeatTimes;
extern const ComponentVariable& RetryBackOffWaitMinimum;
extern const ComponentVariable& UnlockOnEVSideDisconnect;
extern const ComponentVariable& WebSocketPingInterval;
extern const ComponentVariable& ReservationCtrlrAvailable;
extern const ComponentVariable& ReservationCtrlrEnabled;
extern const ComponentVariable& ReservationCtrlrNonEvseSpecific;
extern const ComponentVariable& SampledDataCtrlrAvailable;
extern const ComponentVariable& SampledDataCtrlrEnabled;
extern const ComponentVariable& SampledDataSignReadings;
extern const ComponentVariable& SampledDataTxEndedInterval;
extern const ComponentVariable& SampledDataTxEndedMeasurands;
extern const ComponentVariable& SampledDataTxStartedMeasurands;
extern const ComponentVariable& SampledDataTxUpdatedInterval;
extern const ComponentVariable& SampledDataTxUpdatedMeasurands;
extern const ComponentVariable& AdditionalRootCertificateCheck;
extern const ComponentVariable& BasicAuthPassword;
extern const ComponentVariable& CertificateEntries;
extern const ComponentVariable& SecurityCtrlrIdentity;
extern const ComponentVariable& MaxCertificateChainSize;
extern const ComponentVariable& OrganizationName;
extern const ComponentVariable& SecurityProfile;
extern const ComponentVariable& ACPhaseSwitchingSupported;
extern const ComponentVariable& SmartChargingCtrlrAvailable;
extern const ComponentVariable& SmartChargingCtrlrAvailableEnabled;
extern const ComponentVariable& EntriesChargingProfiles;
extern const ComponentVariable& ExternalControlSignalsEnabled;
extern const ComponentVariable& LimitChangeSignificance;
extern const ComponentVariable& NotifyChargingLimitWithSchedules;
extern const ComponentVariable& PeriodsPerSchedule;
extern const ComponentVariable& Phases3to1;
extern const ComponentVariable& ChargingProfileMaxStackLevel;
extern const ComponentVariable& ChargingScheduleChargingRateUnit;
extern const ComponentVariable& TariffCostCtrlrAvailableTariff;
extern const ComponentVariable& TariffCostCtrlrAvailableCost;
extern const ComponentVariable& TariffCostCtrlrCurrency;
extern const ComponentVariable& TariffCostCtrlrEnabledTariff;
extern const ComponentVariable& TariffCostCtrlrEnabledCost;
extern const ComponentVariable& TariffFallbackMessage;
extern const ComponentVariable& TotalCostFallbackMessage;
extern const ComponentVariable& EVConnectionTimeOut;
extern const ComponentVariable& MaxEnergyOnInvalidId;
extern const ComponentVariable& StopTxOnEVSideDisconnect;
extern const ComponentVariable& StopTxOnInvalidId;
extern const ComponentVariable& TxBeforeAcceptedEnabled;
extern const ComponentVariable& TxStartPoint;
extern const ComponentVariable& TxStopPoint;

} // namespace ControllerComponentVariables
} // namespace v201
} // namespace ocpp

#endif // OCPP_V201_CTRLR_COMPONENT_VARIABLES
