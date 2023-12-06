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

Connector::Connector(const int32_t connector_id,
                     const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback) :
    connector_id(connector_id),
    // TODO verify init BEGIN
    enabled(true),
    reserved(false),
    plugged_in(false),
    faulted(false),
    effective_status(ConnectorStatusEnum::Available),
    // TODO verify init END
    status_notification_callback(status_notification_callback) {
}

ConnectorStatusEnum Connector::determine_effective_status(OperationalStatusEnum evse_status) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    if (evse_status != OperationalStatusEnum::Operative) {
        return ConnectorStatusEnum::Unavailable;
    }
    if (!this->enabled) {
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
    switch(event) {
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
    // TODO persist the new state if needed
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
void Connector::set_operative_status(OperationalStatusEnum new_status,
                                     OperationalStatusEnum evse_status,
                                     bool persist) {
    std::lock_guard<std::recursive_mutex> lk(this->state_mutex);
    this->enabled = (new_status == OperationalStatusEnum::Operative);
    // TODO persist the new state if needed
    // Update the effective status of the connector
    ConnectorStatusEnum new_effective_status = this->determine_effective_status(evse_status);
    this->set_effective_status(new_effective_status);
}

void Connector::update_effective_status(OperationalStatusEnum evse_status) {
    // Update the effective status of the connector
    ConnectorStatusEnum new_effective_status = this->determine_effective_status(evse_status);
    this->set_effective_status(new_effective_status);
}

} // namespace v201
} // namespace ocpp
