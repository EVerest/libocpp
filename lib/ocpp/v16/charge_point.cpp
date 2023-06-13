// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#include <thread>

#include <everest/logging.hpp>
#include <ocpp/v16/charge_point.hpp>
#include <ocpp/v16/charge_point_configuration.hpp>
#include <ocpp/v16/charge_point_impl.hpp>

namespace ocpp {
namespace v16 {

<<<<<<< HEAD
const auto ISO15118_PNC_VENDOR_ID = "org.openchargealliance.iso15118pnc";
const auto CLIENT_CERTIFICATE_TIMER_INTERVAL = std::chrono::hours(12);
const auto OCSP_REQUEST_TIMER_INTERVAL = std::chrono::hours(12);
const auto V2G_CERTIFICATE_TIMER_INTERVAL = std::chrono::hours(12);
const auto INITIAL_CERTIFICATE_REQUESTS_DELAY = std::chrono::seconds(60);

ChargePoint::ChargePoint(const json& config, const std::string& share_path, const std::string& user_config_path,
                         const std::string& database_path, const std::string& sql_init_path,
                         const std::string& message_log_path, const std::string& certs_path) :
    ocpp::ChargingStationBase(),
    initialized(false),
    connection_state(ChargePointConnectionState::Disconnected),
    registration_status(RegistrationStatus::Pending),
    diagnostics_status(DiagnosticsStatus::Idle),
    firmware_status(FirmwareStatus::Idle),
    log_status(UploadLogStatusEnumType::Idle),
    switch_security_profile_callback(nullptr),
    message_log_path(message_log_path) {
    this->configuration = std::make_shared<ocpp::v16::ChargePointConfiguration>(config, share_path, user_config_path);
    this->pki_handler = std::make_shared<ocpp::PkiHandler>(
        boost::filesystem::path(certs_path),
        this->configuration->getAdditionalRootCertificateCheck().get_value_or(false));
    this->heartbeat_timer = std::make_unique<Everest::SteadyTimer>(&this->io_service, [this]() { this->heartbeat(); });
    this->heartbeat_interval = this->configuration->getHeartbeatInterval();
    this->database_handler = std::make_shared<DatabaseHandler>(this->configuration->getChargePointId(),
                                                                     boost::filesystem::path(database_path),
                                                                     boost::filesystem::path(sql_init_path));
    this->database_handler->open_db_connection(this->configuration->getNumberOfConnectors());
    this->transaction_handler = std::make_unique<TransactionHandler>(this->configuration->getNumberOfConnectors());
    this->external_notify = {v16::MessageType::StartTransactionResponse};
    this->message_queue = std::make_unique<ocpp::MessageQueue<v16::MessageType>>(
        [this](json message) -> bool { return this->websocket->send(message.dump()); },
        this->configuration->getTransactionMessageAttempts(), this->configuration->getTransactionMessageRetryInterval(),
        this->external_notify);
    auto log_formats = this->configuration->getLogMessagesFormat();
    bool log_to_console = std::find(log_formats.begin(), log_formats.end(), "console") != log_formats.end();
    bool detailed_log_to_console =
        std::find(log_formats.begin(), log_formats.end(), "console_detailed") != log_formats.end();
    bool log_to_file = std::find(log_formats.begin(), log_formats.end(), "log") != log_formats.end();
    bool log_to_html = std::find(log_formats.begin(), log_formats.end(), "html") != log_formats.end();
    bool session_logging = std::find(log_formats.begin(), log_formats.end(), "session_logging") != log_formats.end();

    this->logging = std::make_shared<ocpp::MessageLogging>(
        this->configuration->getLogMessages(), message_log_path, DateTime().to_rfc3339(), log_to_console,
        detailed_log_to_console, log_to_file, log_to_html, session_logging);

    this->boot_notification_timer =
        std::make_unique<Everest::SteadyTimer>(&this->io_service, [this]() { this->boot_notification(); });

    for (int32_t connector = 0; connector < this->configuration->getNumberOfConnectors() + 1; connector++) {
        this->status_notification_timers.push_back(std::make_unique<Everest::SteadyTimer>(&this->io_service));
    }

    this->clock_aligned_meter_values_timer = std::make_unique<Everest::SystemTimer>(
        &this->io_service, [this]() { this->clock_aligned_meter_values_sample(); });

    this->client_certificate_timer = std::make_unique<Everest::SteadyTimer>(&this->io_service, [this]() {
        EVLOG_info << "Checking if CSMS client certificate has expired";
        int daysLeft =
            this->pki_handler->getDaysUntilLeafExpires(ocpp::CertificateSigningUseEnum::ChargingStationCertificate);
        if (daysLeft < 30) {
            EVLOG_info << "CSMS client certificate is invalid in " << daysLeft
                       << " days. Requesting new certificate with certificate signing request";
            this->sign_certificate(ocpp::CertificateSigningUseEnum::ChargingStationCertificate);
        } else {
            EVLOG_info << "CSMS client certificate is still valid.";
        }
        this->client_certificate_timer->interval(CLIENT_CERTIFICATE_TIMER_INTERVAL);
    });

    this->v2g_certificate_timer = std::make_unique<Everest::SteadyTimer>(&this->io_service, [this]() {
        EVLOG_info << "Checking if V2GCertificate has expired";
        int daysLeft = this->pki_handler->getDaysUntilLeafExpires(ocpp::CertificateSigningUseEnum::V2GCertificate);
        if (daysLeft < 30) {
            EVLOG_info << "V2GCertificate is invalid in " << daysLeft
                       << " days. Requesting new certificate with certificate signing request";
            this->data_transfer_pnc_sign_certificate();
        } else {
            EVLOG_info << "V2GCertificate is still valid.";
        }
        this->v2g_certificate_timer->interval(V2G_CERTIFICATE_TIMER_INTERVAL);
    });

    this->status = std::make_unique<ChargePointStates>(
        [this](int32_t connector, ChargePointErrorCode errorCode, ChargePointStatus status) {
            this->status_notification_timers.at(connector)->stop();
            this->status_notification_timers.at(connector)->timeout(
                [this, connector, errorCode, status]() { this->status_notification(connector, errorCode, status); },
                std::chrono::seconds(this->configuration->getMinimumStatusDuration().get_value_or(0)));
        });

    for (int id = 0; id <= this->configuration->getNumberOfConnectors(); id++) {
        this->connectors.insert(std::make_pair(id, std::make_shared<Connector>(id)));
    }

    this->smart_charging_handler = std::make_unique<SmartChargingHandler>(this->connectors, this->database_handler);

    // ISO15118 PnC handlers
    if (this->configuration->getSupportedFeatureProfilesSet().count(SupportedFeatureProfiles::PnC)) {
        this->data_transfer_pnc_callbacks[conversions::messagetype_to_string(MessageType::TriggerMessage)] =
            [this](ocpp::Call<ocpp::v16::DataTransferRequest> call) {
                this->handle_data_transfer_pnc_trigger_message(call);
            };
        this->data_transfer_pnc_callbacks[conversions::messagetype_to_string(MessageType::CertificateSigned)] =
            [this](ocpp::Call<ocpp::v16::DataTransferRequest> call) {
                this->handle_data_transfer_pnc_certificate_signed(call);
            };
        this->data_transfer_pnc_callbacks[conversions::messagetype_to_string(MessageType::GetInstalledCertificateIds)] =
            [this](ocpp::Call<ocpp::v16::DataTransferRequest> call) {
                this->handle_data_transfer_pnc_get_installed_certificates(call);
            };
        this->data_transfer_pnc_callbacks[conversions::messagetype_to_string(MessageType::DeleteCertificate)] =
            [this](ocpp::Call<ocpp::v16::DataTransferRequest> call) {
                this->handle_data_transfer_delete_certificate(call);
            };
        this->data_transfer_pnc_callbacks[conversions::messagetype_to_string(MessageType::InstallCertificate)] =
            [this](ocpp::Call<ocpp::v16::DataTransferRequest> call) {
                this->handle_data_transfer_install_certificate(call);
            };
        this->ocsp_request_timer = std::make_unique<Everest::SteadyTimer>(&this->io_service, [this]() {
            this->update_ocsp_cache();
            this->ocsp_request_timer->interval(OCSP_REQUEST_TIMER_INTERVAL);
        });
    }
}

void ChargePoint::init_websocket(int32_t security_profile) {
    WebsocketConnectionOptions connection_options{OcppProtocolVersion::v16,
                                                  this->configuration->getCentralSystemURI(),
                                                  security_profile,
                                                  this->configuration->getChargePointId(),
                                                  this->configuration->getAuthorizationKey(),
                                                  this->configuration->getWebsocketReconnectInterval(),
                                                  this->configuration->getSupportedCiphers12(),
                                                  this->configuration->getSupportedCiphers13(),
                                                  this->configuration->getWebsocketPingInterval().value_or(0),
                                                  this->configuration->getWebsocketPingPayload(),
                                                  this->configuration->getUseSslDefaultVerifyPaths(),
                                                  this->configuration->getAdditionalRootCertificateCheck()};

    this->websocket = std::make_unique<Websocket>(connection_options, this->pki_handler, this->logging);
    this->websocket->register_connected_callback([this](const int security_profile) {
        if (this->connection_state_changed_callback != nullptr) {
            this->connection_state_changed_callback(true);
        }
        this->message_queue->resume(); //
        this->connected_callback();    //
    });
    this->websocket->register_disconnected_callback([this]() {
        if (this->connection_state_changed_callback != nullptr) {
            this->connection_state_changed_callback(false);
        }
        this->message_queue->pause(); //
        if (this->ocsp_request_timer != nullptr) {
            this->ocsp_request_timer->stop();
        }
        if (this->client_certificate_timer != nullptr) {
            this->client_certificate_timer->stop();
        }
        if (this->v2g_certificate_timer != nullptr) {
            this->v2g_certificate_timer->stop();
        }
        if (this->switch_security_profile_callback != nullptr) {
            this->switch_security_profile_callback();
        }
    });

    this->websocket->register_message_callback([this](const std::string& message) { this->message_callback(message); });
}

void ChargePoint::connect_websocket() {
    if (!this->websocket->is_connected()) {
        this->init_websocket(this->configuration->getSecurityProfile());
        this->websocket->connect(this->configuration->getSecurityProfile());
    }
}

void ChargePoint::disconnect_websocket() {
    if (this->websocket->is_connected()) {
        this->websocket->disconnect(websocketpp::close::status::going_away);
    }
}

void ChargePoint::call_set_connection_timeout() {
    if (this->set_connection_timeout_callback != nullptr) {
        this->set_connection_timeout_callback(this->configuration->getConnectionTimeOut());
    }
}

void ChargePoint::heartbeat() {
    EVLOG_debug << "Sending heartbeat";
    HeartbeatRequest req;

    ocpp::Call<HeartbeatRequest> call(req, this->message_queue->createMessageId());
    this->send<HeartbeatRequest>(call);
}

void ChargePoint::boot_notification() {
    EVLOG_debug << "Sending BootNotification";
    BootNotificationRequest req;
    req.chargeBoxSerialNumber.emplace(this->configuration->getChargeBoxSerialNumber());
    req.chargePointModel = this->configuration->getChargePointModel();
    req.chargePointSerialNumber = this->configuration->getChargePointSerialNumber();
    req.chargePointVendor = this->configuration->getChargePointVendor();
    req.firmwareVersion.emplace(this->configuration->getFirmwareVersion());
    req.iccid = this->configuration->getICCID();
    req.imsi = this->configuration->getIMSI();
    req.meterSerialNumber = this->configuration->getMeterSerialNumber();
    req.meterType = this->configuration->getMeterType();

    ocpp::Call<BootNotificationRequest> call(req, this->message_queue->createMessageId());
    this->send<BootNotificationRequest>(call);
}

void ChargePoint::clock_aligned_meter_values_sample() {
    if (this->initialized) {
        EVLOG_debug << "Sending clock aligned meter values";
        for (int32_t connector = 1; connector < this->configuration->getNumberOfConnectors() + 1; connector++) {
            auto meter_value = this->get_latest_meter_value(
                connector, this->configuration->getMeterValuesAlignedDataVector(), ReadingContext::Sample_Clock);
            if (meter_value.has_value()) {
                if (this->transaction_handler->transaction_active(connector)) {
                    this->transaction_handler->get_transaction(connector)->add_meter_value(meter_value.value());
                }
                this->send_meter_value(connector, meter_value.value());
            } else {
                EVLOG_warning << "Could not send clock aligned meter value for uninitialized powermeter at connector#"
                              << connector;
            }
        }
        this->update_clock_aligned_meter_values_interval();
    }
}

void ChargePoint::update_heartbeat_interval() {
    this->heartbeat_timer->interval(std::chrono::seconds(this->configuration->getHeartbeatInterval()));
}

void ChargePoint::update_meter_values_sample_interval() {
    // TODO(kai): should we update the meter values for continuous monitoring here too?
    int32_t interval = this->configuration->getMeterValueSampleInterval();
    this->transaction_handler->change_meter_values_sample_intervals(interval);
}

void ChargePoint::update_clock_aligned_meter_values_interval() {
    const auto clock_aligned_data_interval = this->configuration->getClockAlignedDataInterval();
    const auto next_timestamp = this->get_next_clock_aligned_meter_value_timestamp(clock_aligned_data_interval);
    if (next_timestamp.has_value()) {
        this->clock_aligned_meter_values_timer->at(next_timestamp.value().to_time_point());
    }
}

void ChargePoint::stop_pending_transactions() {
    const auto transactions = this->database_handler->get_transactions(true);

    if (!transactions.empty()) {
        EVLOG_info << "Sending StopTransaction.req for " << transactions.size()
                   << " open transactions that haven't been acknowledged by CSMS.";
    }
    for (const auto& transaction_entry : transactions) {
        std::shared_ptr<Transaction> transaction = std::make_shared<Transaction>(
            transaction_entry.connector, transaction_entry.session_id, CiString<20>(transaction_entry.id_tag_start),
            transaction_entry.meter_start, transaction_entry.reservation_id,
            ocpp::DateTime(transaction_entry.time_start), nullptr);
        ocpp::DateTime timestamp;
        int meter_stop = 0;
        if (transaction_entry.time_end.has_value() and transaction_entry.meter_stop.has_value()) {
            timestamp = ocpp::DateTime(transaction_entry.time_end.value());
            meter_stop = transaction_entry.meter_stop.value();
        } else {
            timestamp = ocpp::DateTime(transaction_entry.meter_last_time);
            meter_stop = transaction_entry.meter_last;
        }

        const auto stop_energy_wh = std::make_shared<StampedEnergyWh>(timestamp, meter_stop);
        transaction->add_stop_energy_wh(stop_energy_wh);
        transaction->set_transaction_id(transaction_entry.transaction_id);
        this->transaction_handler->add_transaction(transaction);

        this->stop_transaction(transaction_entry.connector, Reason::PowerLoss, boost::none);
        this->database_handler->update_transaction(transaction_entry.session_id, meter_stop, timestamp.to_rfc3339(),
                                                   boost::none, Reason::PowerLoss);
    }
}

void ChargePoint::load_charging_profiles() {
    auto profiles = this->database_handler->get_charging_profiles();
    EVLOG_info << "Found " << profiles.size() << " charging profile(s) in the database";
    const auto supported_purpose_types = this->configuration->getSupportedChargingProfilePurposeTypes();
    for (auto& profile : profiles) {
        const auto connector_id = this->database_handler->get_connector_id(profile.chargingProfileId);
        if (this->smart_charging_handler->validate_profile(
                profile, connector_id, false, this->configuration->getChargeProfileMaxStackLevel(),
                this->configuration->getMaxChargingProfilesInstalled(),
                this->configuration->getChargingScheduleMaxPeriods(),
                this->configuration->getChargingScheduleAllowedChargingRateUnitVector()) and
            std::find(supported_purpose_types.begin(), supported_purpose_types.end(), profile.chargingProfilePurpose) !=
                supported_purpose_types.end()) {

            if (profile.chargingProfilePurpose == ChargingProfilePurposeType::ChargePointMaxProfile) {
                this->smart_charging_handler->add_charge_point_max_profile(profile);
            } else if (profile.chargingProfilePurpose == ChargingProfilePurposeType::TxDefaultProfile) {
                this->smart_charging_handler->add_tx_default_profile(profile, connector_id);
            } else if (profile.chargingProfilePurpose == ChargingProfilePurposeType::TxProfile) {
                this->smart_charging_handler->add_tx_profile(profile, connector_id);
            }
        } else {
            // delete if not valid anymore
            this->database_handler->delete_charging_profile(profile.chargingProfileId);
        }
    }
}

boost::optional<MeterValue> ChargePoint::get_latest_meter_value(int32_t connector,
                                                                std::vector<MeasurandWithPhase> values_of_interest,
                                                                ReadingContext context) {
    std::lock_guard<std::mutex> lock(power_meters_mutex);
    boost::optional<MeterValue> filtered_meter_value_opt;
    // TODO(kai): also support readings from the charge point powermeter at "connector 0"
    if (this->connectors.find(connector) != this->connectors.end() &&
        this->connectors.at(connector)->powermeter.has_value()) {
        MeterValue filtered_meter_value;
        const auto power_meter = this->connectors.at(connector)->powermeter.value();
        const auto timestamp = power_meter.timestamp;
        filtered_meter_value.timestamp = ocpp::DateTime(timestamp);
        EVLOG_debug << "PowerMeter value for connector: " << connector << ": " << power_meter;

        for (auto configured_measurand : values_of_interest) {
            EVLOG_debug << "Value of interest: " << conversions::measurand_to_string(configured_measurand.measurand);
            // constructing sampled value
            SampledValue sample;

            sample.context.emplace(context);
            sample.format.emplace(ValueFormat::Raw); // TODO(kai): support signed data as well

            sample.measurand.emplace(configured_measurand.measurand);
            if (configured_measurand.phase) {
                EVLOG_debug << "  there is a phase configured: "
                            << conversions::phase_to_string(configured_measurand.phase.value());
            }
            switch (configured_measurand.measurand) {
            case Measurand::Energy_Active_Import_Register: {
                const auto energy_Wh_import = power_meter.energy_Wh_import;

                // Imported energy in Wh (from grid)
                sample.unit.emplace(UnitOfMeasure::Wh);
                sample.location.emplace(Location::Outlet);

                if (configured_measurand.phase) {
                    // phase available and it makes sense here
                    auto phase = configured_measurand.phase.value();
                    sample.phase.emplace(phase);
                    if (phase == Phase::L1) {
                        if (energy_Wh_import.L1) {
                            sample.value = ocpp::conversions::double_to_string((double)energy_Wh_import.L1.value());
                        } else {
                            EVLOG_debug
                                << "Power meter does not contain energy_Wh_import configured measurand for phase L1";
                        }
                    } else if (phase == Phase::L2) {
                        if (energy_Wh_import.L2) {
                            sample.value = ocpp::conversions::double_to_string((double)energy_Wh_import.L2.value());
                        } else {
                            EVLOG_debug
                                << "Power meter does not contain energy_Wh_import configured measurand for phase L2";
                        }
                    } else if (phase == Phase::L3) {
                        if (energy_Wh_import.L3) {
                            sample.value = ocpp::conversions::double_to_string((double)energy_Wh_import.L3.value());
                        } else {
                            EVLOG_debug
                                << "Power meter does not contain energy_Wh_import configured measurand for phase L3";
                        }
                    }
                } else {
                    // store total value
                    sample.value = ocpp::conversions::double_to_string((double)energy_Wh_import.total);
                }
                break;
            }
            case Measurand::Energy_Active_Export_Register: {
                const auto energy_Wh_export = power_meter.energy_Wh_export;
                // Exported energy in Wh (to grid)
                sample.unit.emplace(UnitOfMeasure::Wh);
                // TODO: which location is appropriate here? Inlet?
                // sample.location.emplace(Location::Outlet);
                if (energy_Wh_export) {
                    if (configured_measurand.phase) {
                        // phase available and it makes sense here
                        auto phase = configured_measurand.phase.value();
                        sample.phase.emplace(phase);
                        if (phase == Phase::L1) {
                            if (energy_Wh_export.value().L1) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)energy_Wh_export.value().L1.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain energy_Wh_export configured measurand "
                                               "for phase L1";
                            }
                        } else if (phase == Phase::L2) {
                            if (energy_Wh_export.value().L2) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)energy_Wh_export.value().L2.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain energy_Wh_export configured measurand "
                                               "for phase L2";
                            }
                        } else if (phase == Phase::L3) {
                            if (energy_Wh_export.value().L3) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)energy_Wh_export.value().L3.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain energy_Wh_export configured measurand "
                                               "for phase L3";
                            }
                        }
                    } else {
                        // store total value
                        sample.value = ocpp::conversions::double_to_string((double)energy_Wh_export.value().total);
                    }
                } else {
                    EVLOG_debug << "Power meter does not contain energy_Wh_export configured measurand";
                }
                break;
            }
            case Measurand::Power_Active_Import: {
                const auto power_W = power_meter.power_W;
                // power flow to EV, Instantaneous power in Watt
                sample.unit.emplace(UnitOfMeasure::W);
                sample.location.emplace(Location::Outlet);
                if (power_W) {
                    if (configured_measurand.phase) {
                        // phase available and it makes sense here
                        auto phase = configured_measurand.phase.value();
                        sample.phase.emplace(phase);
                        if (phase == Phase::L1) {
                            if (power_W.value().L1) {
                                sample.value = ocpp::conversions::double_to_string((double)power_W.value().L1.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain power_W configured measurand for phase L1";
                            }
                        } else if (phase == Phase::L2) {
                            if (power_W.value().L2) {
                                sample.value = ocpp::conversions::double_to_string((double)power_W.value().L2.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain power_W configured measurand for phase L2";
                            }
                        } else if (phase == Phase::L3) {
                            if (power_W.value().L3) {
                                sample.value = ocpp::conversions::double_to_string((double)power_W.value().L3.value());
                            } else {
                                EVLOG_debug << "Power meter does not contain power_W configured measurand for phase L3";
                            }
                        }
                    } else {
                        // store total value
                        sample.value = ocpp::conversions::double_to_string((double)power_W.value().total);
                    }
                } else {
                    EVLOG_debug << "Power meter does not contain power_W configured measurand";
                }
                break;
            }
            case Measurand::Voltage: {
                const auto voltage_V = power_meter.voltage_V;
                // AC supply voltage, Voltage in Volts
                sample.unit.emplace(UnitOfMeasure::V);
                sample.location.emplace(Location::Outlet);
                if (voltage_V) {
                    if (configured_measurand.phase) {
                        // phase available and it makes sense here
                        auto phase = configured_measurand.phase.value();
                        sample.phase.emplace(phase);
                        if (phase == Phase::L1) {
                            if (voltage_V.value().L1) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)voltage_V.value().L1.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain voltage_V configured measurand for phase L1";
                            }
                        } else if (phase == Phase::L2) {
                            if (voltage_V.value().L2) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)voltage_V.value().L2.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain voltage_V configured measurand for phase L2";
                            }
                        } else if (phase == Phase::L3) {
                            if (voltage_V.value().L3) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)voltage_V.value().L3.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain voltage_V configured measurand for phase L3";
                            }
                        }
                    }
                    // report DC value if set. This is a workaround for the fact that the power meter does not report
                    // AC (Dc charging)
                    else if (voltage_V.value().DC) {
                        sample.value = ocpp::conversions::double_to_string((double)voltage_V.value().DC.value());
                    }

                } else {
                    EVLOG_debug << "Power meter does not contain voltage_V configured measurand";
                }
                break;
            }
            case Measurand::Current_Import: {
                const auto current_A = power_meter.current_A;
                // current flow to EV in A
                sample.unit.emplace(UnitOfMeasure::A);
                sample.location.emplace(Location::Outlet);
                if (current_A) {
                    if (configured_measurand.phase) {
                        // phase available and it makes sense here
                        auto phase = configured_measurand.phase.value();
                        sample.phase.emplace(phase);
                        if (phase == Phase::L1) {
                            if (current_A.value().L1) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)current_A.value().L1.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain current_A configured measurand for phase L1";
                            }
                        } else if (phase == Phase::L2) {
                            if (current_A.value().L2) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)current_A.value().L2.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain current_A configured measurand for phase L2";
                            }
                        } else if (phase == Phase::L3) {
                            if (current_A.value().L3) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)current_A.value().L3.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain current_A configured measurand for phase L3";
                            }
                        }
                    }
                    // report DC value if set. This is a workaround for the fact that the power meter does not report
                    // AC (DC charging)
                    else if (current_A.value().DC) {
                        sample.value = ocpp::conversions::double_to_string((double)current_A.value().DC.value());
                    }
                } else {
                    EVLOG_debug << "Power meter does not contain current_A configured measurand";
                }

                break;
            }
            case Measurand::Frequency: {
                const auto frequency_Hz = power_meter.frequency_Hz;
                // Grid frequency in Hertz
                // TODO: which location is appropriate here? Inlet?
                // sample.location.emplace(Location::Outlet);
                if (frequency_Hz) {
                    if (configured_measurand.phase) {
                        // phase available and it makes sense here
                        auto phase = configured_measurand.phase.value();
                        sample.phase.emplace(phase);
                        if (phase == Phase::L1) {
                            sample.value = ocpp::conversions::double_to_string((double)frequency_Hz.value().L1);
                        } else if (phase == Phase::L2) {
                            if (frequency_Hz.value().L2) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)frequency_Hz.value().L2.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain frequency_Hz configured measurand for phase L2";
                            }
                        } else if (phase == Phase::L3) {
                            if (frequency_Hz.value().L3) {
                                sample.value =
                                    ocpp::conversions::double_to_string((double)frequency_Hz.value().L3.value());
                            } else {
                                EVLOG_debug
                                    << "Power meter does not contain frequency_Hz configured measurand for phase L3";
                            }
                        }
                    }
                } else {
                    EVLOG_debug << "Power meter does not contain frequency_Hz configured measurand";
                }
                break;
            }
            case Measurand::Current_Offered: {
                // current offered to EV
                sample.unit.emplace(UnitOfMeasure::A);
                sample.location.emplace(Location::Outlet);

                sample.value = ocpp::conversions::double_to_string(this->connectors.at(connector)->max_current_offered);
                break;
            }
            default:
                break;
            }
            // only add if value is set
            if (!sample.value.empty()) {
                filtered_meter_value.sampledValue.push_back(sample);
            }
        }
        filtered_meter_value_opt.emplace(filtered_meter_value);
    }
    return filtered_meter_value_opt;
}

MeterValue ChargePoint::get_signed_meter_value(const std::string& signed_value, const ReadingContext& context,
                                               const ocpp::DateTime& timestamp) {
    MeterValue meter_value;
    meter_value.timestamp = timestamp;
    SampledValue sampled_value;
    sampled_value.context = context;
    sampled_value.value = signed_value;
    sampled_value.format = ValueFormat::SignedData;

    meter_value.sampledValue.push_back(sampled_value);
    return meter_value;
}

void ChargePoint::send_meter_value(int32_t connector, MeterValue meter_value) {

    if (meter_value.sampledValue.size() == 0) {
        return;
    }

    MeterValuesRequest req;
    const auto message_id = this->message_queue->createMessageId();
    // connector = 0 designates the main powermeter
    // connector > 0 designates a connector of the charge point
    req.connectorId = connector;
    std::ostringstream oss;
    oss << "Gathering measurands of connector: " << connector;
    if (connector > 0) {
        auto transaction = this->transaction_handler->get_transaction(connector);
        if (transaction != nullptr && transaction->get_transaction_id() != -1) {
            auto transaction_id = transaction->get_transaction_id();
            req.transactionId.emplace(transaction_id);
        } else if (transaction != nullptr and transaction->get_transaction_id() == -1) {
            // this means a transaction is active but we have not received a transaction_id from CSMS yet
            this->message_queue->add_meter_value_message_id(transaction->get_start_transaction_message_id(),
                                                            message_id.get());
        }
    }

    EVLOG_debug << oss.str();

    req.meterValue.push_back(meter_value);

    ocpp::Call<MeterValuesRequest> call(req, message_id);
    this->send<MeterValuesRequest>(call);
}
=======
ChargePoint::ChargePoint(const std::string& config, const std::filesystem::path& share_path,
                         const std::filesystem::path& user_config_path, const std::filesystem::path& database_path,
                         const std::filesystem::path& sql_init_path, const std::filesystem::path& message_log_path,
                         const std::filesystem::path& certs_path) {
    this->charge_point = std::make_unique<ChargePointImpl>(config, share_path, user_config_path, database_path,
                                                           sql_init_path, message_log_path, certs_path);
}

ChargePoint::~ChargePoint() = default;
>>>>>>> 158e9e7208760f1ead8bffbdb331a1743b260b73

bool ChargePoint::start() {
    return this->charge_point->start();
}

bool ChargePoint::restart() {
    return this->charge_point->restart();
}

bool ChargePoint::stop() {
    return this->charge_point->stop();
}

void ChargePoint::connect_websocket() {
    this->charge_point->connect_websocket();
}

void ChargePoint::disconnect_websocket() {
    this->charge_point->disconnect_websocket();
}

void ChargePoint::call_set_connection_timeout() {
    this->charge_point->call_set_connection_timeout();
}

IdTagInfo ChargePoint::authorize_id_token(CiString<20> id_token) {
    return this->charge_point->authorize_id_token(id_token);
}

ocpp::v201::AuthorizeResponse ChargePoint::data_transfer_pnc_authorize(

    const std::string& emaid, const std::optional<std::string>& certificate,
    const std::optional<std::vector<ocpp::v201::OCSPRequestData>>& iso15118_certificate_hash_data) {
    return this->charge_point->data_transfer_pnc_authorize(emaid, certificate, iso15118_certificate_hash_data);
}

void ChargePoint::data_transfer_pnc_get_15118_ev_certificate(
    const int32_t connector_id, const std::string& exi_request, const std::string& iso15118_schema_version,
    const ocpp::v201::CertificateActionEnum& certificate_action) {

    this->charge_point->data_transfer_pnc_get_15118_ev_certificate(connector_id, exi_request, iso15118_schema_version,
                                                                   certificate_action);
}

DataTransferResponse ChargePoint::data_transfer(const CiString<255>& vendorId, const CiString<50>& messageId,
                                                const std::string& data) {
    return this->charge_point->data_transfer(vendorId, messageId, data);
}

std::map<int32_t, ChargingSchedule> ChargePoint::get_all_composite_charging_schedules(const int32_t duration_s) {

    return this->charge_point->get_all_composite_charging_schedules(duration_s);
}

void ChargePoint::on_meter_values(int32_t connector, const Powermeter& power_meter) {
    this->charge_point->on_meter_values(connector, power_meter);
}

void ChargePoint::on_max_current_offered(int32_t connector, int32_t max_current) {
    this->charge_point->on_max_current_offered(connector, max_current);
}

void ChargePoint::on_session_started(int32_t connector, const std::string& session_id, const std::string& reason,
                                     const std::optional<std::string>& session_logging_path) {

    this->charge_point->on_session_started(connector, session_id, reason, session_logging_path);
}

void ChargePoint::on_session_stopped(const int32_t connector, const std::string& session_id) {
    this->charge_point->on_session_stopped(connector, session_id);
}

void ChargePoint::on_transaction_started(const int32_t& connector, const std::string& session_id,
                                         const std::string& id_token, const int32_t& meter_start,
                                         std::optional<int32_t> reservation_id, const ocpp::DateTime& timestamp,
                                         std::optional<std::string> signed_meter_value) {
    this->charge_point->on_transaction_started(connector, session_id, id_token, meter_start, reservation_id, timestamp,
                                               signed_meter_value);
}

void ChargePoint::on_transaction_stopped(const int32_t connector, const std::string& session_id, const Reason& reason,
                                         ocpp::DateTime timestamp, float energy_wh_import,
                                         std::optional<CiString<20>> id_tag_end,
                                         std::optional<std::string> signed_meter_value) {
    this->charge_point->on_transaction_stopped(connector, session_id, reason, timestamp, energy_wh_import, id_tag_end,
                                               signed_meter_value);
}

void ChargePoint::on_suspend_charging_ev(int32_t connector) {
    this->charge_point->on_suspend_charging_ev(connector);
}

void ChargePoint::on_suspend_charging_evse(int32_t connector) {
    this->charge_point->on_suspend_charging_evse(connector);
}

void ChargePoint::on_resume_charging(int32_t connector) {
    this->charge_point->on_resume_charging(connector);
}

void ChargePoint::on_error(int32_t connector, const ChargePointErrorCode& error) {
    this->charge_point->on_error(connector, error);
}

void ChargePoint::on_log_status_notification(int32_t request_id, std::string log_status) {
    this->charge_point->on_log_status_notification(request_id, log_status);
}

void ChargePoint::on_firmware_update_status_notification(int32_t request_id, std::string firmware_update_status) {
    this->charge_point->on_firmware_update_status_notification(request_id, firmware_update_status);
}

void ChargePoint::on_reservation_start(int32_t connector) {
    this->charge_point->on_reservation_start(connector);
}

void ChargePoint::on_reservation_end(int32_t connector) {
    this->charge_point->on_reservation_end(connector);
}

void ChargePoint::on_enabled(int32_t connector) {
    this->charge_point->on_enabled(connector);
}

void ChargePoint::on_disabled(int32_t connector) {
    this->charge_point->on_disabled(connector);
}

void ChargePoint::register_data_transfer_callback(
    const CiString<255>& vendorId, const CiString<50>& messageId,
    const std::function<DataTransferResponse(const std::optional<std::string>& msg)>& callback) {
    this->charge_point->register_data_transfer_callback(vendorId, messageId, callback);
}

void ChargePoint::register_enable_evse_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_enable_evse_callback(callback);
}

void ChargePoint::register_disable_evse_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_disable_evse_callback(callback);
}

void ChargePoint::register_pause_charging_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_pause_charging_callback(callback);
}

void ChargePoint::register_resume_charging_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_resume_charging_callback(callback);
}

void ChargePoint::register_provide_token_callback(
    const std::function<void(const std::string& id_token, std::vector<int32_t> referenced_connectors,
                             bool prevalidated)>& callback) {
    this->charge_point->register_provide_token_callback(callback);
}

void ChargePoint::register_stop_transaction_callback(
    const std::function<bool(int32_t connector, Reason reason)>& callback) {
    this->charge_point->register_stop_transaction_callback(callback);
}

void ChargePoint::register_reserve_now_callback(
    const std::function<ReservationStatus(int32_t reservation_id, int32_t connector, ocpp::DateTime expiryDate,
                                          CiString<20> idTag, std::optional<CiString<20>> parent_id)>& callback) {
    this->charge_point->register_reserve_now_callback(callback);
}

void ChargePoint::register_cancel_reservation_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_cancel_reservation_callback(callback);
}

void ChargePoint::register_unlock_connector_callback(const std::function<bool(int32_t connector)>& callback) {
    this->charge_point->register_unlock_connector_callback(callback);
}

void ChargePoint::register_upload_diagnostics_callback(
    const std::function<GetLogResponse(const GetDiagnosticsRequest& request)>& callback) {
    this->charge_point->register_upload_diagnostics_callback(callback);
}

void ChargePoint::register_update_firmware_callback(
    const std::function<void(const UpdateFirmwareRequest msg)>& callback) {
    this->charge_point->register_update_firmware_callback(callback);
}

void ChargePoint::register_signed_update_firmware_callback(
    const std::function<UpdateFirmwareStatusEnumType(const SignedUpdateFirmwareRequest msg)>& callback) {
    this->charge_point->register_signed_update_firmware_callback(callback);
}

void ChargePoint::register_upload_logs_callback(const std::function<GetLogResponse(GetLogRequest req)>& callback) {
    this->charge_point->register_upload_logs_callback(callback);
}

void ChargePoint::register_set_connection_timeout_callback(
    const std::function<void(int32_t connection_timeout)>& callback) {
    this->charge_point->register_set_connection_timeout_callback(callback);
}

void ChargePoint::register_is_reset_allowed_callback(const std::function<bool(const ResetType& reset_type)>& callback) {
    this->charge_point->register_is_reset_allowed_callback(callback);
}

void ChargePoint::register_reset_callback(const std::function<void(const ResetType& reset_type)>& callback) {
    this->charge_point->register_reset_callback(callback);
}

void ChargePoint::register_set_system_time_callback(
    const std::function<void(const std::string& system_time)>& callback) {
    this->charge_point->register_set_system_time_callback(callback);
}

void ChargePoint::register_signal_set_charging_profiles_callback(const std::function<void()>& callback) {
    this->charge_point->register_signal_set_charging_profiles_callback(callback);
}

void ChargePoint::register_connection_state_changed_callback(const std::function<void(bool is_connected)>& callback) {
    this->charge_point->register_connection_state_changed_callback(callback);
}

void ChargePoint::register_get_15118_ev_certificate_response_callback(
    const std::function<void(const int32_t connector,
                             const ocpp::v201::Get15118EVCertificateResponse& certificate_response,
                             const ocpp::v201::CertificateActionEnum& certificate_action)>& callback) {
    this->charge_point->register_get_15118_ev_certificate_response_callback(callback);
}

} // namespace v16
} // namespace ocpp
