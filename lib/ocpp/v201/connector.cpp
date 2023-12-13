// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <utility>

#include <everest/logging.hpp>
#include <ocpp/v201/connector.hpp>

namespace ocpp {
namespace v201 {

namespace conversions {

std::string connector_event_to_string(ConnectorEvent e) {
    switch (e) {
    case ConnectorEvent::PlugIn:
        return "PlugIn";
    case ConnectorEvent::PlugOut:
        return "PlugOut";
    case ConnectorEvent::Reserve:
        return "Reserve";
    case ConnectorEvent::ReservationCleared:
        return "ReservationCleared";
    case ConnectorEvent::Error:
        return "Error";
    case ConnectorEvent::ErrorCleared:
        return "ErrorCleared";
    }

    throw std::out_of_range("No known string conversion for provided enum of type ConnectorEvent");
}

} // namespace conversions

Connector::Connector(
    const int32_t evse_id, const int32_t connector_id, std::shared_ptr<DatabaseHandler> database_handler,
    OperationalStatusEnum evse_effective_status,
    const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback,
    const std::function<void(const OperationalStatusEnum new_status)> change_effective_availability_callback) :
    evse_id(evse_id),
    connector_id(connector_id),
    database_handler(database_handler),
    operational(database_handler->get_availability(evse_id, connector_id)),
    reserved(false),
    plugged_in(false),
    faulted(false),
    status_notification_callback(status_notification_callback),
    change_effective_availability_callback(change_effective_availability_callback) {
    this->effective_status = this->determine_effective_status(evse_effective_status);
}

ConnectorStatusEnum Connector::determine_effective_status(OperationalStatusEnum evse_status) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    if (evse_status != OperationalStatusEnum::Operative) {
        return ConnectorStatusEnum::Unavailable;
    }
    if (this->operational == OperationalStatusEnum::Inoperative) {
        return ConnectorStatusEnum::Unavailable;
    }
    if (this->faulted) {
        return ConnectorStatusEnum::Faulted;
    }
    if (this->plugged_in) {
        return ConnectorStatusEnum::Occupied;
    }
    if (this->reserved) {
        return ConnectorStatusEnum::Reserved;
    }
    return ConnectorStatusEnum::Available;
}

void Connector::submit_event(ConnectorEvent event, OperationalStatusEnum evse_status) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    switch (event) {
    case ConnectorEvent::PlugIn:
        this->plugged_in = true;
        break;
    case ConnectorEvent::PlugOut:
        this->plugged_in = false;
        break;
    case ConnectorEvent::Reserve:
        this->reserved = true;
        break;
    case ConnectorEvent::ReservationCleared:
        this->reserved = false;
        break;
    case ConnectorEvent::Error:
        this->faulted = true;
        break;
    case ConnectorEvent::ErrorCleared:
        this->faulted = false;
        break;
    }
    // Update the effective status of the connector
    ConnectorStatusEnum new_effective_status = this->determine_effective_status(evse_status);
    this->set_effective_status(new_effective_status);
}

ConnectorStatusEnum Connector::get_effective_status() {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    return this->effective_status;
}

void Connector::set_effective_status(ConnectorStatusEnum new_effective_status) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    bool changed = (new_effective_status != this->effective_status);
    this->effective_status = new_effective_status;
    if (changed) {
        this->status_notification_callback(new_effective_status);
    }
}

static OperationalStatusEnum to_operative(ConnectorStatusEnum connector_status) {
    if (connector_status == ConnectorStatusEnum::Unavailable || connector_status == ConnectorStatusEnum::Faulted) {
        return OperationalStatusEnum::Inoperative;
    } else {
        return OperationalStatusEnum::Operative;
    }
}

void Connector::set_operative_status(std::optional<OperationalStatusEnum> new_status, OperationalStatusEnum evse_status,
                                     bool persist, bool is_boot) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);

    OperationalStatusEnum old_op_status = this->get_operative_status();
    OperationalStatusEnum old_eff_status = to_operative(this->get_effective_status());

    if (new_status.has_value()) {
        this->operational = new_status.value();
    }
    this->set_effective_status(this->determine_effective_status(evse_status));

    OperationalStatusEnum new_eff_status = to_operative(this->get_effective_status());

    if (old_op_status != this->operational && persist) {
        this->database_handler->insert_availability(this->evse_id, this->connector_id, this->operational, true);
    }
    if (is_boot || old_eff_status != new_eff_status) {
        this->change_effective_availability_callback(new_eff_status);
    }
}
OperationalStatusEnum Connector::get_operative_status() {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    return this->operational;
}

} // namespace v201
} // namespace ocpp
