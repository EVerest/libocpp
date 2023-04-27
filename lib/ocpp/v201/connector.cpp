// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <utility>

#include <everest/logging.hpp>
#include <ocpp/v201/connector.hpp>

namespace ocpp {
namespace v201 {

Connector::Connector(const int32_t connector_id,
                     const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback) :
    connector_id(connector_id),
    state(ConnectorStatusEnum::Available),
    last_state(ConnectorStatusEnum::Available),
    operational_state(OperationalStatusEnum::Operative),
    status_notification_callback(status_notification_callback) {
}

void Connector::set_state(const ConnectorStatusEnum new_state) {
    std::lock_guard<std::mutex> lg(this->state_mutex);
    this->last_state = this->state;
    this->state = new_state;
}

ConnectorStatusEnum Connector::get_state() {
    std::lock_guard<std::mutex> lg(this->state_mutex);
    return this->state;
}

OperationalStatusEnum Connector::get_operational_state() {
    return this->operational_state;
}

void Connector::set_operational_state(const OperationalStatusEnum& operational_state) {
    this->operational_state = operational_state;
    if (this->operational_state == OperationalStatusEnum::Inoperative) {
        this->submit_event(ConnectorEvent::Unavailable);
    }
}

void Connector::submit_event(ConnectorEvent event) {

    // FIXME(piet): This state machine implementation is a first draft
    const auto current_state = this->get_state();

    switch (current_state) {
    case ConnectorStatusEnum::Available:
        switch (event) {
        case ConnectorEvent::PlugIn:
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        case ConnectorEvent::Reserve:
            this->set_state(ConnectorStatusEnum::Reserved);
            break;
        case ConnectorEvent::Error:
            this->set_state(ConnectorStatusEnum::Faulted);
            break;
        case ConnectorEvent::Unavailable:
            this->set_state(ConnectorStatusEnum::Unavailable);
            break;
        default:
            EVLOG_warning << "Invalid connector event in state Available.";
            return;
        }
        break;
    case ConnectorStatusEnum::Occupied:
        switch (event) {
        case ConnectorEvent::PlugOut:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        case ConnectorEvent::Error:
            this->set_state(ConnectorStatusEnum::Faulted);
            break;
        case ConnectorEvent::Unavailable:
            this->set_state(ConnectorStatusEnum::Unavailable);
            break;
        default:
            EVLOG_warning << "Invalid connector event in state Occupied.";
            return;
        }
        break;
    case ConnectorStatusEnum::Reserved:
        switch (event) {
        case ConnectorEvent::ReservationFinished:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        case ConnectorEvent::PlugInAndTokenValid:
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        case ConnectorEvent::Error:
            this->set_state(ConnectorStatusEnum::Faulted);
            break;
        case ConnectorEvent::Unavailable:
            this->set_state(ConnectorStatusEnum::Unavailable);
            break;
        default:
            EVLOG_warning << "Invalid connector event in state Reserved.";
            return;
        }
        break;
    case ConnectorStatusEnum::Unavailable:
        switch (event) {
        case ConnectorEvent::UnavailableToAvailable:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        case ConnectorEvent::UnavailableToOccupied:
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        case ConnectorEvent::UnavailableToReserved:
            this->set_state(ConnectorStatusEnum::Reserved);
            break;
        case ConnectorEvent::UnavailableFaulted:
            this->set_state(ConnectorStatusEnum::Faulted);
            break;
        case ConnectorEvent::PlugOut:
            this->last_state = ConnectorStatusEnum::Available;
            break;
        case ConnectorEvent::PlugIn:
            this->last_state = ConnectorStatusEnum::Occupied;
            break;
        case ConnectorEvent::ReturnToOperativeState:
            if (this->operational_state == OperationalStatusEnum::Operative) {
                this->set_state(this->last_state);
            }
            break;
        default:
            EVLOG_warning << "Invalid connector event in state Unavailable.";
            return;
        }
        break;
    case ConnectorStatusEnum::Faulted:
        switch (event) {
        case ConnectorEvent::ErrorCleared:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        case ConnectorEvent::ErrorCleardOnOccupied:
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        case ConnectorEvent::ErrorCleardOnReserved:
            this->set_state(ConnectorStatusEnum::Reserved);
            break;
        case ConnectorEvent::Unavailable:
            this->set_state(ConnectorStatusEnum::Unavailable);
            break;
        default:
            EVLOG_warning << "Invalid connector event in state Faulted.";
            return;
        }
        break;
    }
    if (current_state != this->state) {
        this->status_notification_callback(this->state);
    }
}

} // namespace v201
} // namespace ocpp
