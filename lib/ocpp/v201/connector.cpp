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
    case ConnectorEvent::Error:
        return "Error";
    case ConnectorEvent::Unavailable:
        return "Unavailable";
    case ConnectorEvent::ReservationFinished:
        return "ReservationFinished";
    case ConnectorEvent::PlugInAndTokenValid:
        return "PlugInAndTokenValid";
    case ConnectorEvent::ErrorCleared:
        return "ErrorCleared";
    case ConnectorEvent::ErrorClearedOnOccupied:
        return "ErrorCleardOnOccupied";
    case ConnectorEvent::ErrorClearedOnReserved:
        return "ErrorCleardOnReserved";
    case ConnectorEvent::UnavailableToAvailable:
        return "UnavailableToAvailable";
    case ConnectorEvent::UnavailableToOccupied:
        return "UnavailableToOccupied";
    case ConnectorEvent::UnavailableToReserved:
        return "UnavailableToReserved";
    case ConnectorEvent::UnavailableFaulted:
        return "UnavailableFaulted";
    case ConnectorEvent::ReturnToOperativeState:
        return "ReturnToOperativeState";
    }

    throw std::out_of_range("No known string conversion for provided enum of type ConnectorEvent");
}

ConnectorEvent string_to_connector_event(const std::string& s) {
    if (s == "PlugIn") {
        return ConnectorEvent::PlugIn;
    }
    if (s == "PlugOut") {
        return ConnectorEvent::PlugOut;
    }
    if (s == "Reserve") {
        return ConnectorEvent::Reserve;
    }
    if (s == "Error") {
        return ConnectorEvent::Error;
    }
    if (s == "Unavailable") {
        return ConnectorEvent::Unavailable;
    }
    if (s == "ReservationFinished") {
        return ConnectorEvent::ReservationFinished;
    }
    if (s == "PlugInAndTokenValid") {
        return ConnectorEvent::PlugInAndTokenValid;
    }
    if (s == "ErrorCleared") {
        return ConnectorEvent::ErrorCleared;
    }
    if (s == "ErrorCleardOnOccupied") {
        return ConnectorEvent::ErrorClearedOnOccupied;
    }
    if (s == "ErrorCleardOnReserved") {
        return ConnectorEvent::ErrorClearedOnReserved;
    }
    if (s == "UnavailableToAvailable") {
        return ConnectorEvent::UnavailableToAvailable;
    }
    if (s == "UnavailableToOccupied") {
        return ConnectorEvent::UnavailableToOccupied;
    }
    if (s == "UnavailableToReserved") {
        return ConnectorEvent::UnavailableToReserved;
    }
    if (s == "UnavailableFaulted") {
        return ConnectorEvent::UnavailableFaulted;
    }
    if (s == "ReturnToOperativeState") {
        return ConnectorEvent::ReturnToOperativeState;
    }

    throw std::out_of_range("Provided string " + s + " could not be converted to enum of type ConnectorEvent");
}

} // namespace conversions

Connector::Connector(const int32_t connector_id,
                     const std::function<void(const ConnectorStatusEnum& status)>& status_notification_callback) :
    connector_id(connector_id),
    state(ConnectorStatusEnum::Available), // TODO verify this is correct init
    state_if_operative(ConnectorStatusEnum::Available), // TODO verify this is correct init
    effective_state(ConnectorStatusEnum::Available), // TODO verify this is correct init
    status_notification_callback(status_notification_callback) {
}

void Connector::set_state(const ConnectorStatusEnum new_state) {
    std::lock_guard<std::mutex> lg(this->state_mutex);
    this->state = new_state;
    if (this->state == ConnectorStatusEnum::Available
        || this->state == ConnectorStatusEnum::Occupied
        || this->state == ConnectorStatusEnum::Reserved) {
        this->state_if_operative = new_state;
    }
}

ConnectorStatusEnum Connector::get_effective_state() {
    std::lock_guard<std::mutex> lg(this->state_mutex);
    return this->effective_state;
}

ConnectorStatusEnum Connector::get_state() {
    std::lock_guard<std::mutex> lg(this->state_mutex);
    return this->state;
}

void Connector::submit_event(ConnectorEvent event, OperationalStatusEnum evse_status) {

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
        case ConnectorEvent::ReturnToOperativeState:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        default:
            EVLOG_warning << "Invalid connector event: " << conversions::connector_event_to_string(event)
                          << " in state Available.";
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
        case ConnectorEvent::ReturnToOperativeState: // TODO: Is this valid?
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        default:
            EVLOG_warning << "Invalid connector event: " << conversions::connector_event_to_string(event)
                          << " in state Occupied.";
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
        case ConnectorEvent::ReturnToOperativeState: // TODO: Is this valid?
            this->set_state(ConnectorStatusEnum::Reserved);
            break;
        default:
            EVLOG_warning << "Invalid connector event: " << conversions::connector_event_to_string(event)
                          << " in state Reserved.";
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
            this->state_if_operative = ConnectorStatusEnum::Available;
            break;
        case ConnectorEvent::PlugIn:
            this->state_if_operative = ConnectorStatusEnum::Occupied;
            break;
        case ConnectorEvent::ReturnToOperativeState:
            this->set_state(this->state_if_operative);
            break;
        case ConnectorEvent::Unavailable:
            break;
        default:
            EVLOG_warning << "Invalid connector event: " << conversions::connector_event_to_string(event)
                          << " in state Unavailable.";
            return;
        }
        break;
    case ConnectorStatusEnum::Faulted:
        switch (event) {
        case ConnectorEvent::ErrorCleared:
            this->set_state(ConnectorStatusEnum::Available);
            break;
        case ConnectorEvent::ErrorClearedOnOccupied:
            this->set_state(ConnectorStatusEnum::Occupied);
            break;
        case ConnectorEvent::ErrorClearedOnReserved:
            this->set_state(ConnectorStatusEnum::Reserved);
            break;
        case ConnectorEvent::Unavailable: // TODO: Is this a sensible state transition?
            this->set_state(ConnectorStatusEnum::Unavailable);
            break;
        default:
            EVLOG_warning << "Invalid connector event: " << conversions::connector_event_to_string(event)
                          << " in state Faulted.";
            return;
        }
        break;
    }

    // Recompute the effective state of the connector
    ConnectorStatusEnum prev_effective_state = this->effective_state;
    // It's Unavailable if the evse is unavailable, otherwise equal to the individual status
    this->effective_state = ConnectorStatusEnum::Unavailable;
    if (evse_status == OperationalStatusEnum::Operative) {
        this->effective_state = this->state;
    }
    if (prev_effective_state != this->effective_state) {
        this->status_notification_callback(this->effective_state);
    }
}

void Connector::change_availability(std::optional<OperationalStatusEnum> new_status,
                                    OperationalStatusEnum evse_status) {
    if (new_status.has_value()) {
        if (evse_status == OperationalStatusEnum::Inoperative) {
            // Disable the connector
            this->submit_event(ConnectorEvent::Unavailable, evse_status);
        } else {
            // Enable the connector
            this->submit_event(ConnectorEvent::ReturnToOperativeState, evse_status);
        }
        // TODO persist new status
    }

}

} // namespace v201
} // namespace ocpp
