// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/message_handler.hpp>

#include <ocpp/common/message_dispatcher.hpp>

#include <ocpp/v201/messages/UpdateFirmware.hpp>

namespace ocpp {

// Forward declarations.
class EvseSecurity;

namespace v201 {

// Formward declarations.
class DeviceModel;
class EvseManagerInterface;
class Availability;
class Security;

// Typedef
typedef std::function<UpdateFirmwareResponse(const UpdateFirmwareRequest& request)> UpdateFirmwareRequestCallback;
typedef std::function<void()> AllConnectorsUnavailableCallback;

class FirmwareUpdateInterface : public MessageHandlerInterface {
public:
    virtual ~FirmwareUpdateInterface() {
    }

    virtual void on_firmware_update_status_notification(int32_t request_id,
                                                        const FirmwareStatusEnum& firmware_update_status) = 0;
    virtual void on_firmware_status_notification_request() = 0;
};

class FirmwareUpdate : public FirmwareUpdateInterface {
private: // Members
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    DeviceModel& device_model;
    EvseManagerInterface& evse_manager;
    EvseSecurity& evse_security;
    Availability& availability;
    Security& security;

    UpdateFirmwareRequestCallback update_firmware_request_callback;
    std::optional<AllConnectorsUnavailableCallback> all_connectors_unavailable_callback;

    FirmwareStatusEnum firmware_status;
    // The request ID in the last firmware update status received
    std::optional<int32_t> firmware_status_id;
    // The last firmware status which will be posted before the firmware is installed.
    FirmwareStatusEnum firmware_status_before_installing = FirmwareStatusEnum::SignatureVerified;

public:
    FirmwareUpdate(MessageDispatcherInterface<MessageType>& message_dispatcher, DeviceModel& device_model,
                   EvseManagerInterface& evse_manager, EvseSecurity& evse_security, Availability& availability,
                   Security& security, UpdateFirmwareRequestCallback update_firmware_request_callback,
                   AllConnectorsUnavailableCallback all_connectors_unavailable_callback);
    void on_firmware_update_status_notification(int32_t request_id,
                                                const FirmwareStatusEnum& firmware_update_status) override;
    void on_firmware_status_notification_request() override;

private: // Functions
    // Functional Block L: Firmware management
    void handle_firmware_update_req(Call<UpdateFirmwareRequest> call);

    /// \brief Changes all unoccupied connectors to unavailable. If a transaction is running schedule an availabilty
    /// change
    /// If all connectors are unavailable signal to the firmware updater that installation of the firmware update can
    /// proceed
    void change_all_connectors_to_unavailable_for_firmware_update();

    /// \brief Restores all connectors to their persisted state
    void restore_all_connector_states();
};
} // namespace v201
} // namespace ocpp
