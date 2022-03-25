// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <ocpp1_6/charging_session.hpp>

namespace ocpp1_6 {
Transaction::Transaction(int32_t transactionId, std::unique_ptr<Everest::SteadyTimer> meter_values_sample_timer) :
    transactionId(transactionId), active(true), meter_values_sample_timer(std::move(meter_values_sample_timer)) {
}

void Transaction::add_sampled_meter_value(MeterValue meter_value) {
    if (this->active) {
        std::lock_guard<std::mutex> lock(this->sampled_meter_values_mutex);
        this->sampled_meter_values.push_back(meter_value);
    }
}

std::vector<MeterValue> Transaction::get_sampled_meter_values() {
    std::lock_guard<std::mutex> lock(this->sampled_meter_values_mutex);
    return this->sampled_meter_values;
}

bool Transaction::change_meter_values_sample_interval(int32_t interval) {
    this->meter_values_sample_timer->interval(std::chrono::seconds(interval));
    return true;
}

void Transaction::add_clock_aligned_meter_value(MeterValue meter_value) {
    if (this->active) {
        std::lock_guard<std::mutex> lock(this->clock_aligned_meter_values_mutex);
        this->clock_aligned_meter_values.push_back(meter_value);
    }
}

std::vector<MeterValue> Transaction::get_clock_aligned_meter_values() {
    std::lock_guard<std::mutex> lock(this->clock_aligned_meter_values_mutex);
    return this->clock_aligned_meter_values;
}

int32_t Transaction::get_transaction_id() {
    return this->transactionId;
}

std::vector<TransactionData> Transaction::get_transaction_data() {
    std::vector<TransactionData> transaction_data_vec;
    for (auto meter_value : this->get_sampled_meter_values()) {
        TransactionData transaction_data;
        transaction_data.timestamp = meter_value.timestamp;
        transaction_data.sampledValue = meter_value.sampledValue;
        transaction_data_vec.push_back(transaction_data);
    }
    for (auto meter_value : this->get_clock_aligned_meter_values()) {
        TransactionData transaction_data;
        transaction_data.timestamp = meter_value.timestamp;
        transaction_data.sampledValue = meter_value.sampledValue;
        transaction_data_vec.push_back(transaction_data);
    }
    return transaction_data_vec;
}

void Transaction::stop() {
    this->active = false;
}

ChargingSession::ChargingSession() : authorized_token(nullptr), plug_connected(false), transaction(nullptr) {
}

ChargingSession::ChargingSession(std::unique_ptr<AuthorizedToken> authorized_token) :
    authorized_token(std::move(authorized_token)), plug_connected(false), transaction(nullptr) {
}

void ChargingSession::connect_plug() {
    this->plug_connected = true;
}

void ChargingSession::disconnect_plug() {
    this->plug_connected = false;
}

bool ChargingSession::authorized_token_available() {
    return this->authorized_token != nullptr;
}

bool ChargingSession::add_authorized_token(std::unique_ptr<AuthorizedToken> authorized_token) {
    this->authorized_token = std::move(authorized_token);
    return true;
}

bool ChargingSession::add_start_energy_wh(std::shared_ptr<StampedEnergyWh> start_energy_wh) {
    if (this->start_energy_wh != nullptr) {
        return false;
    }
    this->start_energy_wh = start_energy_wh;
    return true;
}

std::shared_ptr<StampedEnergyWh> ChargingSession::get_start_energy_wh() {
    return this->start_energy_wh;
}

bool ChargingSession::add_stop_energy_wh(std::shared_ptr<StampedEnergyWh> stop_energy_wh) {
    if (this->stop_energy_wh != nullptr) {
        return false;
    }
    if (this->transaction != nullptr) {
        this->transaction->stop();
    }
    this->stop_energy_wh = stop_energy_wh;
    return true;
}

std::shared_ptr<StampedEnergyWh> ChargingSession::get_stop_energy_wh() {
    return this->stop_energy_wh;
}

bool ChargingSession::ready() {
    return this->plug_connected && this->authorized_token_available() && this->start_energy_wh != nullptr;
}

bool ChargingSession::running() {
    return this->transaction != nullptr;
}

boost::optional<CiString20Type> ChargingSession::get_authorized_id_tag() {
    if (this->authorized_token == nullptr) {
        return boost::none;
    }
    return this->authorized_token->idTag;
}

bool ChargingSession::add_transaction(std::shared_ptr<Transaction> transaction) {
    if (this->transaction != nullptr) {
        return false;
    }
    this->transaction = transaction;
    return true;
}

std::shared_ptr<Transaction> ChargingSession::get_transaction() {
    return this->transaction;
}

bool ChargingSession::change_meter_values_sample_interval(int32_t interval) {
    if (this->transaction == nullptr) {
        return false;
    }
    return this->transaction->change_meter_values_sample_interval(interval);
}

void ChargingSession::add_sampled_meter_value(MeterValue meter_value) {
    if (transaction != nullptr) {
        this->transaction->add_sampled_meter_value(meter_value);
    }
}

std::vector<MeterValue> ChargingSession::get_sampled_meter_values() {
    if (this->transaction == nullptr) {
        return {};
    }
    return this->transaction->get_sampled_meter_values();
}

void ChargingSession::add_clock_aligned_meter_value(MeterValue meter_value) {
    if (transaction != nullptr) {
        this->transaction->add_clock_aligned_meter_value(meter_value);
    }
}

std::vector<MeterValue> ChargingSession::get_clock_aligned_meter_values() {
    if (this->transaction == nullptr) {
        return {};
    }
    return this->transaction->get_clock_aligned_meter_values();
}

bool ChargingSessions::valid_connector(int32_t connector) {
    if (connector < 0 || connector > this->charging_sessions.size()) {
        return false;
    }
    return true;
}

ChargingSessions::ChargingSessions(int32_t number_of_connectors) : number_of_connectors(number_of_connectors) {
    for (size_t i = 0; i < number_of_connectors + 1; i++) {
        this->charging_sessions.push_back(nullptr);
    }
}

int32_t ChargingSessions::add_authorized_token(CiString20Type idTag, IdTagInfo idTagInfo) {
    // EVLOG(error) << "add token thingy";
    return this->add_authorized_token(0, idTag, idTagInfo);
}

int32_t ChargingSessions::add_authorized_token(int32_t connector, CiString20Type idTag, IdTagInfo idTagInfo) {
    EVLOG(critical) << "add_authorized_token to connector " << connector;

    if (!this->valid_connector(connector)) {
        return -1;
    }

    auto authorized_token = std::make_unique<AuthorizedToken>(idTag, idTagInfo);

    if (connector == 0) {
        std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
        // check if a charging session is missing an authorized token
        bool moved = false;
        int32_t index = 0;
        for (auto& charging_session : this->charging_sessions) {
            if (charging_session != nullptr && charging_session->get_authorized_id_tag() == boost::none) {
                charging_session->add_authorized_token(std::move(authorized_token));
                moved = true;
                connector = index;
                break;
            }
            index += 1;
        }
        if (!moved) {
            this->charging_sessions.at(0) = std::make_unique<ChargingSession>(std::move(authorized_token));
        }
    } else {
        std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
        if (this->charging_sessions.at(connector) == nullptr) {
            this->charging_sessions.at(connector) = std::make_unique<ChargingSession>(std::move(authorized_token));
        } else {
            if (this->charging_sessions.at(connector)->running()) {
                // do not add a authorized token to a running charging session
                return -1;
            }
            EVLOG(info) << "Adding authorized token " << authorized_token->idTag;
            this->charging_sessions.at(connector)->add_authorized_token(std::move(authorized_token));
        }
    }
    return connector;
}

bool ChargingSessions::add_start_energy_wh(int32_t connector, std::shared_ptr<StampedEnergyWh> start_energy_wh) {
    if (!this->valid_connector(connector)) {
        return false;
    }
    if (connector == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return false;
    }
    this->charging_sessions.at(connector)->add_start_energy_wh(start_energy_wh);
    return true;
}

std::shared_ptr<StampedEnergyWh> ChargingSessions::get_start_energy_wh(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return nullptr;
    }
    if (connector == 0) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return nullptr;
    }
    return this->charging_sessions.at(connector)->get_start_energy_wh();
}

bool ChargingSessions::add_stop_energy_wh(int32_t connector, std::shared_ptr<StampedEnergyWh> stop_energy_wh) {
    if (!this->valid_connector(connector)) {
        return false;
    }
    if (connector == 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return false;
    }
    this->charging_sessions.at(connector)->add_stop_energy_wh(stop_energy_wh);
    return true;
}

std::shared_ptr<StampedEnergyWh> ChargingSessions::get_stop_energy_wh(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return nullptr;
    }
    if (connector == 0) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return nullptr;
    }
    return this->charging_sessions.at(connector)->get_stop_energy_wh();
}

bool ChargingSessions::initiate_session(int32_t connector) {
    // TODO(kai): think about supporting connector 0 here, meaning "any connector"
    if (connector == 0) {
        EVLOG(warning) << "Attempting to start a charging session on connector 0, this is not supported at the moment";
        return false;
    }
    if (!this->valid_connector(connector)) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
        if (this->charging_sessions.at(connector) == nullptr) {
            if (this->charging_sessions.at(0) != nullptr) {
                this->charging_sessions.at(connector) = std::move(this->charging_sessions.at(0));
            } else {
                this->charging_sessions.at(connector) = std::make_unique<ChargingSession>();
            }
        }
        if (this->charging_sessions.at(connector)->running()) {
            return false;
        }
        this->charging_sessions.at(connector)->connect_plug();
    }
    return true;
}

bool ChargingSessions::remove_session(int32_t connector) {
    if (connector == 0) {
        EVLOG(warning) << "Attempting to remove a charging session on connector 0, this is not supported.";
        return false;
    }
    if (!this->valid_connector(connector)) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
        this->charging_sessions.at(connector) = nullptr;
    }
    return true;
}

bool ChargingSessions::ready(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) != nullptr) {
        return this->charging_sessions.at(connector)->ready();
    }
    return false;
}

bool ChargingSessions::add_transaction(int32_t connector, std::shared_ptr<Transaction> transaction) {
    if (!this->valid_connector(connector)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return false;
    }
    return this->charging_sessions.at(connector)->add_transaction(transaction);
}

std::shared_ptr<Transaction> ChargingSessions::get_transaction(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return nullptr;
    }
    return this->charging_sessions.at(connector)->get_transaction();
}

bool ChargingSessions::transaction_active(int32_t connector) {
    return this->get_transaction(connector) != nullptr;
}

int32_t ChargingSessions::get_connector_from_transaction_id(int32_t transaction_id) {
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    int32_t index = 0;
    for (auto& charging_session : this->charging_sessions) {
        if (charging_session != nullptr) {
            auto transaction = charging_session->get_transaction();
            if (transaction != nullptr) {
                if (transaction->get_transaction_id() == transaction_id) {
                    return index;
                }
            }
        }
        index += 1;
    }
    return -1;
}

bool ChargingSessions::change_meter_values_sample_interval(int32_t connector, int32_t interval) {
    if (!this->valid_connector(connector)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return false;
    }
    return this->charging_sessions.at(connector)->change_meter_values_sample_interval(interval);
}

bool ChargingSessions::change_meter_values_sample_intervals(int32_t interval) {
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    bool success = true;
    for (auto& charging_session : this->charging_sessions) {
        if (charging_session != nullptr && charging_session->running()) {
            if (!charging_session->change_meter_values_sample_interval(interval)) {
                success = false;
            }
        }
    }
    return success;
}

boost::optional<CiString20Type> ChargingSessions::get_authorized_id_tag(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return boost::none;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return boost::none;
    }
    return this->charging_sessions.at(connector)->get_authorized_id_tag();
}

void ChargingSessions::add_sampled_meter_value(int32_t connector, MeterValue meter_value) {
    if (!this->valid_connector(connector)) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return;
    }
    this->charging_sessions.at(connector)->add_sampled_meter_value(meter_value);
}

std::vector<MeterValue> ChargingSessions::get_sampled_meter_values(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return {};
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return {};
    }
    return this->charging_sessions.at(connector)->get_sampled_meter_values();
}

void ChargingSessions::add_clock_aligned_meter_value(int32_t connector, MeterValue meter_value) {
    if (!this->valid_connector(connector)) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return;
    }
    this->charging_sessions.at(connector)->add_clock_aligned_meter_value(meter_value);
}

std::vector<MeterValue> ChargingSessions::get_clock_aligned_meter_values(int32_t connector) {
    if (!this->valid_connector(connector)) {
        return {};
    }
    std::lock_guard<std::mutex> lock(this->charging_sessions_mutex);
    if (this->charging_sessions.at(connector) == nullptr) {
        return {};
    }
    return this->charging_sessions.at(connector)->get_clock_aligned_meter_values();
}



Reservations::Reservations() {

}

std::set<int32_t> Reservations::get_reserved_ids() {
    std::set<int32_t> ids;
    for(std::map<int32_t, std::tuple<int32_t, DateTime, CiString20Type>>::iterator it = this->reservations.begin(); it != this->reservations.end(); ++it) {
        ids.insert(it->first);
    }
    return ids;
}

int32_t Reservations::get_unreserved_connector(int32_t query_connector, std::map<int32_t, ocpp1_6::AvailabilityType> availability) {
    int32_t current_reservations = 0;
    // std::set<int32_t> reserved_connectors = get_reserved_values(TupleElement::connector_id);
    std::set<int32_t> reserved_connectors = get_reserved_connectors();
    

    if (query_connector == 0) {
        int32_t unreserved = 0;
        for(std::map<int32_t, ocpp1_6::AvailabilityType>::iterator it = availability.begin(); it != availability.end(); ++it) {
            int32_t current_connector = it->first;
            ocpp1_6::AvailabilityType current_availability = it->second;

            current_reservations = reserved_connectors.count(current_connector);
            if (current_reservations == 0 && current_availability == AvailabilityType::Operative) {
                unreserved += 1;
            } else if ((current_connector != 0 && current_reservations > 1) || current_reservations < 0) {
                return this->error_unexpected_state;
            }
        }

        // Account for flexible reservations with connector set to zero
        unreserved -= reserved_connectors.count(query_connector);

        if (unreserved > 0) {
            return query_connector;
        } else if (unreserved == 0) {
            return this->no_connectors_available;
        } else {
            return this->error_unexpected_state;
        }
    } else if (query_connector > 0) {
        current_reservations = get_reserved_connectors().count(query_connector);
        if (availability[query_connector] == AvailabilityType::Operative && current_reservations == 0) {
            return query_connector;
        } else {
            return this->no_connectors_available;
        }
    }

    return this->error_unexpected_state;
}




ReservationStatus Reservations::try_reserve_now(int32_t reservationId, int32_t connectorId, DateTime expiryDate, CiString20Type idTag, std::map<int32_t, ocpp1_6::AvailabilityType> availability) {


    auto properties = std::make_tuple(connectorId, expiryDate, idTag);    
    auto pair = std::pair<int32_t, std::tuple<int32_t, DateTime, CiString20Type>>(reservationId, properties);

    if (this->get_reserved_ids().count(reservationId)) {
        // Overwrite existing reservation
        this->reservations[reservationId] = properties;
        // TODO: Change the availabilityType and check the status
        return ReservationStatus::Accepted;
    } else {
        int32_t to_be_reserved = this->get_unreserved_connector(connectorId, availability);

        // Sanity check - perhaps this can be turned into a test case.
        bool valid_unexpected_output = (to_be_reserved >= 0 && to_be_reserved != connectorId);
        bool invalid_output = to_be_reserved < -1;
        if (valid_unexpected_output || invalid_output) {
            // Unexpected state - faulted
        }

        if (to_be_reserved > 0) {
            // Create new reservation for connectorId
            
        } else if (to_be_reserved == 0) {
            // Create new flexible reservation for connectorId (0)

        } else if (to_be_reserved == -1) {
            // No connectors found - unavailable

        } else {
            // Unexpected state - faulted
        }
    }

    // unavailable = faulted



    // Else:
    // Return rejected if configured not to accept reservations

    // If reserved: refuse charging unless if the incoming idTag or parent idTag match that of the reservation.


    // if transaction starts for reserved idTag or reserved connector or any connector when connid == 0:
    //     terminate charge point
    //
    // if expiryDate time is reached:
    //     terminate charge point

    // if chargepoint or connector are faulted / unavailable:
    //     terminate charge point

    // If reserved idTag is started:
    //     the chargepoint sends reservationId in the StartTransaction.req PDU (see start Transaction)
    //     to notify the central system that the reservation is terminated.

    // when reservation expires:
    //     terminate the reservation and make connector available
    //     notify the central system that the reserved connector is now available

    // if authorization cache exists:
    //     update the cache entry upon receiving reserveNow.conf
    //     incase it is not already in the Local Authorization List (Authorization Cache)

    // Before starting the transaction:
    //     Validate the identifier with authorize.req after receiving ReserveNow.req.

    // Evsim_manager

        // This point should never be reached
        return ReservationStatus::Faulted;
    }

} // namespace ocpp1_6
