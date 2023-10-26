// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <utility>

#include <everest/logging.hpp>
#include <ocpp/v201/ctrlr_component_variables.hpp>
#include <ocpp/v201/evse.hpp>
#include <ocpp/v201/utils.hpp>

namespace ocpp {
namespace v201 {

// Convert an energy value into Wh
static float get_normalized_energy_value(SampledValue sampled_value) {
    float value = sampled_value.value;
    // If no unit of measure is present the unit is in Wh so nothing to do
    if (sampled_value.unitOfMeasure.has_value()) {
        const auto& unit_of_measure = sampled_value.unitOfMeasure.value();
        if (unit_of_measure.unit.has_value()) {
            if (unit_of_measure.unit.value() == "kWh") {
                value *= 1000.0f;
            } else if (unit_of_measure.unit.value() == "Wh") {
                // do nothing
            } else {
                EVLOG_AND_THROW(
                    std::runtime_error("Attempt to convert an energy value which does not have a correct unit"));
            }
        }

        if (unit_of_measure.multiplier.has_value()) {
            if (unit_of_measure.multiplier.value() != 0) {
                value *= powf(10, unit_of_measure.multiplier.value());
            }
        }
    }
    return value;
}

Evse::Evse(const int32_t evse_id, const int32_t number_of_connectors, DeviceModel& device_model,
           const std::function<void(const int32_t connector_id, const ConnectorStatusEnum& status)>&
               status_notification_callback,
           const std::function<void(const MeterValue& meter_value, const Transaction& transaction, const int32_t seq_no,
                                    const std::optional<int32_t> reservation_id)>& transaction_meter_value_req,
           const std::function<void()> pause_charging_callback) :
    evse_id(evse_id),
    device_model(device_model),
    status_notification_callback(status_notification_callback),
    transaction_meter_value_req(transaction_meter_value_req),
    pause_charging_callback(pause_charging_callback),
    transaction(nullptr) {
    for (int connector_id = 1; connector_id <= number_of_connectors; connector_id++) {
        this->id_connector_map.insert(std::make_pair(
            connector_id,
            std::make_unique<Connector>(connector_id, [this, connector_id](const ConnectorStatusEnum& status) {
                this->status_notification_callback(connector_id, status);
            })));
    }
}

EVSE Evse::get_evse_info() {
    EVSE evse{evse_id};
    return evse;
}

uint32_t Evse::get_number_of_connectors() {
    return static_cast<uint32_t>(this->id_connector_map.size());
}

Everest::SteadyTimer& Evse::get_sampled_meter_values_timer() {
    return this->sampled_meter_values_timer;
}

void Evse::open_transaction(const std::string& transaction_id, const int32_t connector_id, const DateTime& timestamp,
                            const MeterValue& meter_start, const IdToken& id_token,
                            const std::optional<IdToken>& group_id_token, const std::optional<int32_t> reservation_id,
                            const int32_t sampled_data_tx_updated_interval) {
    if (!this->id_connector_map.count(connector_id)) {
        EVLOG_AND_THROW(std::runtime_error("Attempt to start transaction at invalid connector_id"));
    }
    this->transaction = std::make_unique<EnhancedTransaction>();
    this->transaction->transactionId = transaction_id;
    this->transaction->reservation_id = reservation_id;
    this->transaction->id_token = id_token;
    this->transaction->group_id_token = group_id_token;

    auto start_value = this->get_active_import_register_meter_value();
    if (start_value.has_value()) {
        this->transaction->active_energy_import_start_value = start_value.value();
    }

    transaction->meter_values.push_back(meter_start);

    if (sampled_data_tx_updated_interval > 0) {
        this->sampled_meter_values_timer.interval(
            [this] {
                const auto meter_value = this->get_meter_value();
                this->transaction->meter_values.push_back(meter_value);
                this->transaction_meter_value_req(meter_value, this->transaction->get_transaction(),
                                                  transaction->get_seq_no(), this->transaction->reservation_id);
            },
            std::chrono::seconds(sampled_data_tx_updated_interval));
    }
}

void Evse::close_transaction(const DateTime& timestamp, const MeterValue& meter_stop, const ReasonEnum& reason) {
    if (this->transaction == nullptr) {
        EVLOG_warning << "Received attempt to stop a transaction without an active transaction";
        return;
    }

    this->transaction->stoppedReason.emplace(reason);
    this->transaction->meter_values.push_back(meter_stop);
    this->sampled_meter_values_timer.stop();
}

void Evse::start_checking_max_energy_on_invalid_id() {
    if (this->transaction != nullptr) {
        this->transaction->check_max_active_import_energy = true;
        this->check_max_energy_on_invalid_id();
    } else {
        EVLOG_error << "Trying to start \"MaxEnergyOnInvalidId\" checking without an active transaction";
    }
}

bool Evse::has_active_transaction() {
    return this->transaction != nullptr;
}

void Evse::release_transaction() {
    this->transaction = nullptr;
}

std::unique_ptr<EnhancedTransaction>& Evse::get_transaction() {
    return this->transaction;
}

ConnectorStatusEnum Evse::get_state(const int32_t connector_id) {
    return this->id_connector_map.at(connector_id)->get_state();
}

void Evse::submit_event(const int32_t connector_id, ConnectorEvent event) {
    return this->id_connector_map.at(connector_id)->submit_event(event);
}

void Evse::trigger_status_notification_callbacks() {
    for (auto const& [connector_id, connector] : this->id_connector_map) {
        this->status_notification_callback(connector_id, connector->get_state());
    }
}

void Evse::trigger_status_notification_callback(const int32_t connector_id) {
    this->status_notification_callback(connector_id, this->id_connector_map.at(connector_id)->get_state());
}

void Evse::on_meter_value(const MeterValue& meter_value) {
    std::lock_guard<std::recursive_mutex> lk(this->meter_value_mutex);
    this->meter_value = meter_value;
    this->check_max_energy_on_invalid_id();
}

MeterValue Evse::get_meter_value() {
    std::lock_guard<std::recursive_mutex> lk(this->meter_value_mutex);
    return this->meter_value;
}

std::optional<float> Evse::get_active_import_register_meter_value() {
    std::lock_guard<std::recursive_mutex> lk(this->meter_value_mutex);
    auto it = std::find_if(
        this->meter_value.sampledValue.begin(), this->meter_value.sampledValue.end(), [](const SampledValue& value) {
            return value.measurand == MeasurandEnum::Energy_Active_Import_Register and !value.phase.has_value();
        });
    if (it != this->meter_value.sampledValue.end()) {
        return get_normalized_energy_value(*it);
    }
    return std::nullopt;
}

void Evse::check_max_energy_on_invalid_id() {
    // Handle E05.02
    auto max_energy_on_invalid_id =
        this->device_model.get_optional_value<int32_t>(ControllerComponentVariables::MaxEnergyOnInvalidId);
    auto& transaction = this->transaction;
    if (transaction != nullptr and max_energy_on_invalid_id.has_value() and
        transaction->active_energy_import_start_value.has_value() and transaction->check_max_active_import_energy) {
        const auto opt_energy_value = this->get_active_import_register_meter_value();

        if (opt_energy_value.has_value()) {
            auto charged_energy = opt_energy_value.value() - transaction->active_energy_import_start_value.value();

            if (charged_energy > static_cast<float>(max_energy_on_invalid_id.value())) {
                this->pause_charging_callback();
                transaction->check_max_active_import_energy = false; // No need to check anymore
            }
        }
    }
}

} // namespace v201
} // namespace ocpp
