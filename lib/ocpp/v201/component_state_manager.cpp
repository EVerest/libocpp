// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <fmt/format.h>
#include <ocpp/v201/component_state_manager.hpp>

namespace ocpp::v201 {

ComponentStateManager::ComponentStateManager(const std::map<int32_t, int32_t>& evse_connector_structure,
                                             std::shared_ptr<DatabaseHandler> db_handler) :
    database(std::move(db_handler)) {
    this->database->insert_cs_availability(OperationalStatusEnum::Operative, false);
    this->cs_individual_status = this->database->get_cs_availability();

    int num_evses = evse_connector_structure.size();
    for (int evse_id = 1; evse_id <= num_evses; evse_id++) {
        if (evse_connector_structure.count(evse_id) == 0) {
            throw std::logic_error("evse_connector_structure should contain EVSE ids counting from 1 upwards.");
        }
        int num_connectors = evse_connector_structure.at(evse_id);
        std::vector<FullConnectorStatus> connector_statuses;

        this->database->insert_evse_availability(evse_id, OperationalStatusEnum::Operative, false);
        OperationalStatusEnum evse_operational = this->database->get_evse_availability(evse_id);

        for (int connector_id = 1; connector_id <= num_connectors; connector_id++) {
            this->database->insert_connector_availability(evse_id, connector_id, OperationalStatusEnum::Operative, false);
            OperationalStatusEnum connector_operational = this->database->get_connector_availability(evse_id, connector_id);
            FullConnectorStatus full_connector_status{connector_operational, false, false, false};
            connector_statuses.push_back(full_connector_status);
        }

        this->evse_and_connector_individual_statuses.push_back(std::make_pair(evse_operational, connector_statuses));
    }

    // Initialize the cached effective statuses (after everything else is done)
    this->last_cs_effective_status = this->get_cs_individual_operational_status();
    for (int evse_id = 1; evse_id <= num_evses; evse_id++) {
        int num_connectors = evse_connector_structure.at(evse_id);
        std::vector<ConnectorStatusEnum> connector_statuses;

        OperationalStatusEnum evse_effective = this->get_evse_effective_operational_status(evse_id);
        for (int connector_id = 1; connector_id <= num_connectors; connector_id++) {
            ConnectorStatusEnum connector_effective = this->get_connector_effective_status(evse_id, connector_id);
            connector_statuses.push_back(connector_effective);
        }

        this->last_evse_and_connector_effective_statuses.push_back(std::make_pair(evse_effective, connector_statuses));
    }
}

void ComponentStateManager::check_evse_id(int32_t evse_id) {
    if (evse_id <= 0 || evse_id > this->evse_and_connector_individual_statuses.size()) {
        throw std::logic_error(fmt::format("EVSE ID {} out of bounds.", evse_id));
    }
}

void ComponentStateManager::check_evse_and_connector_id(int32_t evse_id, int32_t connector_id) {
    this->check_evse_id(evse_id);
    if (connector_id <= 0 ||
        connector_id > this->evse_and_connector_individual_statuses.at(evse_id - 1).second.size()) {
        throw std::logic_error(fmt::format("Connector ID {} out of bounds.", connector_id));
    }
}

OperationalStatusEnum ComponentStateManager::get_cs_individual_operational_status() {
    return this->cs_individual_status;
}
OperationalStatusEnum ComponentStateManager::get_evse_individual_operational_status(int32_t evse_id) {
    this->check_evse_id(evse_id);
    return this->evse_and_connector_individual_statuses[evse_id - 1].first;
}
OperationalStatusEnum ComponentStateManager::get_connector_individual_operational_status(int32_t evse_id,
                                                                                         int32_t connector_id) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    return this->evse_and_connector_individual_statuses[evse_id - 1]
        .second[connector_id - 1]
        .individual_operational_status;
}

void ComponentStateManager::set_cs_individual_operational_status(OperationalStatusEnum new_status, bool persist) {
    this->cs_individual_status = new_status;
    if (persist) {
        this->database->insert_cs_availability(new_status, true);
    }
}
void ComponentStateManager::set_evse_individual_operational_status(int32_t evse_id, OperationalStatusEnum new_status,
                                                                   bool persist) {
    this->check_evse_id(evse_id);
    this->evse_and_connector_individual_statuses[evse_id - 1].first = new_status;
    if (persist) {
        this->database->insert_evse_availability(evse_id, new_status, true);
    }
}
void ComponentStateManager::set_connector_individual_operational_status(int32_t evse_id, int32_t connector_id,
                                                                        OperationalStatusEnum new_status,
                                                                        bool persist) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    this->evse_and_connector_individual_statuses[evse_id - 1].second[connector_id - 1].individual_operational_status =
        new_status;
    if (persist) {
        this->database->insert_connector_availability(evse_id, connector_id, new_status, true);
    }
}

ConnectorStatusEnum FullConnectorStatus::to_effective_status() {
    if (this->individual_operational_status == OperationalStatusEnum::Inoperative) {
        return ConnectorStatusEnum::Unavailable;
    }
    if (this->faulted) {
        return ConnectorStatusEnum::Faulted;
    }
    if (this->occupied) {
        return ConnectorStatusEnum::Occupied;
    }
    if (this->reserved) {
        return ConnectorStatusEnum::Reserved;
    }
    return ConnectorStatusEnum::Available;
}

OperationalStatusEnum ComponentStateManager::get_evse_effective_operational_status(int32_t evse_id) {
    this->check_evse_id(evse_id);
    if (this->cs_individual_status == OperationalStatusEnum::Inoperative) {
        return OperationalStatusEnum::Inoperative;
    }
    return this->evse_and_connector_individual_statuses[evse_id - 1].first;
}
ConnectorStatusEnum ComponentStateManager::get_connector_effective_status(int32_t evse_id, int32_t connector_id) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    if (this->cs_individual_status == OperationalStatusEnum::Inoperative) {
        return ConnectorStatusEnum::Unavailable;
    }
    if (this->evse_and_connector_individual_statuses[evse_id - 1].first == OperationalStatusEnum::Inoperative) {
        return ConnectorStatusEnum::Unavailable;
    }

    return this->evse_and_connector_individual_statuses[evse_id - 1].second[connector_id - 1].to_effective_status();
}
OperationalStatusEnum ComponentStateManager::get_connector_effective_operational_status(int32_t evse_id,
                                                                                        int32_t connector_id) {
    ConnectorStatusEnum eff_status = this->get_connector_effective_status(evse_id, connector_id);
    if (eff_status == ConnectorStatusEnum::Unavailable || eff_status == ConnectorStatusEnum::Faulted) {
        return OperationalStatusEnum::Inoperative;
    } else {
        return OperationalStatusEnum::Operative;
    }
}

OperationalStatusEnum ComponentStateManager::get_cs_persisted_operational_status() {
    return this->database->get_cs_availability();
}
OperationalStatusEnum ComponentStateManager::get_evse_persisted_operational_status(int32_t evse_id) {
    this->check_evse_id(evse_id);
    return this->database->get_evse_availability(evse_id);
}
OperationalStatusEnum ComponentStateManager::get_connector_persisted_operational_status(int32_t evse_id,
                                                                                        int32_t connector_id) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    return this->database->get_connector_availability(evse_id, connector_id);
}

void ComponentStateManager::set_connector_occupied(int32_t evse_id, int32_t connector_id, bool is_occupied) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    this->evse_and_connector_individual_statuses[evse_id - 1].second[connector_id - 1].occupied = is_occupied;
}
void ComponentStateManager::set_connector_reserved(int32_t evse_id, int32_t connector_id, bool is_reserved) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    this->evse_and_connector_individual_statuses[evse_id - 1].second[connector_id - 1].reserved = is_reserved;
}
void ComponentStateManager::set_connector_faulted(int32_t evse_id, int32_t connector_id, bool is_faulted) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    this->evse_and_connector_individual_statuses[evse_id - 1].second[connector_id - 1].faulted = is_faulted;
}

bool ComponentStateManager::cs_effective_status_changed() {
    return this->last_cs_effective_status != this->get_cs_individual_operational_status();
}
bool ComponentStateManager::evse_effective_status_changed(int32_t evse_id) {
    this->check_evse_id(evse_id);
    return this->get_evse_effective_operational_status(evse_id)
           != this->last_evse_and_connector_effective_statuses[evse_id-1].first;
}
bool ComponentStateManager::connector_effective_status_changed(int32_t evse_id, int32_t connector_id) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    return this->get_connector_effective_status(evse_id, connector_id)
           != this->last_evse_and_connector_effective_statuses[evse_id-1].second[connector_id-1];
}

void ComponentStateManager::clear_cs_effective_status_changed() {
    this->last_cs_effective_status = this->get_cs_individual_operational_status();
}
void ComponentStateManager::clear_evse_effective_status_changed(int32_t evse_id) {
    this->check_evse_id(evse_id);
    this->last_evse_and_connector_effective_statuses[evse_id-1].first =
        this->get_evse_effective_operational_status(evse_id);
}
void ComponentStateManager::clear_connector_effective_status_changed(int32_t evse_id, int32_t connector_id) {
    this->check_evse_and_connector_id(evse_id, connector_id);
    this->last_evse_and_connector_effective_statuses[evse_id-1].second[connector_id-1] =
        this->get_connector_effective_status(evse_id, connector_id);
}

} // namespace ocpp::v201