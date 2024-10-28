// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/message_dispatcher.hpp>
#include <ocpp/v201/messages/DataTransfer.hpp>

namespace ocpp {
namespace v201 {

class DataTransferInterface {

public:
    /// \brief Data transfer mechanism initiated by charger
    /// \param vendorId
    /// \param messageId
    /// \param data
    /// \return DataTransferResponse containing the result from CSMS
    virtual std::optional<DataTransferResponse> data_transfer_req(const CiString<255>& vendorId,
                                                                  const std::optional<CiString<50>>& messageId,
                                                                  const std::optional<json>& data) = 0;

    /// \brief Data transfer mechanism initiated by charger
    /// \param request message shall be sent to the CSMS
    /// \return DataTransferResponse containing the result from CSMS. In case no response is received from the CSMS
    /// because the message timed out or the charging station is offline, std::nullopt is returned
    virtual std::optional<DataTransferResponse> data_transfer_req(const DataTransferRequest& request) = 0;

    /// \brief Handles the given DataTransfer.req \p call by the CSMS by responding with a CallResult
    virtual void handle_data_transfer_req(Call<DataTransferRequest> call) = 0;
};

class DataTransfer : public DataTransferInterface {

private:
    MessageDispatcherInterface<MessageType>& message_dispatcher;
    std::optional<std::function<DataTransferResponse(const DataTransferRequest& request)>> data_transfer_callback;

public:
    DataTransfer(MessageDispatcherInterface<MessageType>& message_dispatcher,
                 const std::optional<std::function<DataTransferResponse(const DataTransferRequest& request)>>&
                     data_transfer_callback) :
        message_dispatcher(message_dispatcher), data_transfer_callback(data_transfer_callback){};

    void handle_data_transfer_req(Call<DataTransferRequest> call) override;

    std::optional<DataTransferResponse> data_transfer_req(const CiString<255>& vendorId,
                                                          const std::optional<CiString<50>>& messageId,
                                                          const std::optional<json>& data) override;

    std::optional<DataTransferResponse> data_transfer_req(const DataTransferRequest& request) override;
};

} // namespace v201
} // namespace ocpp