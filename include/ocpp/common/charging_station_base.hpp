// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_CHARGE_POINT_HPP
#define OCPP_COMMON_CHARGE_POINT_HPP

#include <boost/shared_ptr.hpp>

#include <ocpp/common/evse_security.hpp>
#include <ocpp/common/evse_security_impl.hpp>
#include <ocpp/common/message_queue.hpp>
#include <ocpp/common/websocket/websocket.hpp>

namespace ocpp {

/// \brief Common base class for OCPP1.6 and OCPP2.0.1 charging stations
class ChargingStationBase {

protected:
    std::unique_ptr<Websocket> websocket;
    std::shared_ptr<EvseSecurity> evse_security;
    std::shared_ptr<MessageLogging> logging;
    Everest::SteadyTimer websocket_timer;

    boost::shared_ptr<boost::asio::io_service::work> work;
    boost::asio::io_service io_service;
    std::thread io_service_thread;

    boost::uuids::random_generator uuid_generator;

    /// \brief Identifies the next timestamp at which a clock aligned meter value should be send
    /// \param interval the configured AlignedDataInterval
    /// \return std::optional<DateTime> If \param interval > 0 it returns the next timestamp at which a clock aligned
    /// meter value should be sent, else std::nullopt
    std::optional<DateTime> get_next_clock_aligned_meter_value_timestamp(const int32_t interval);

    /// \brief Generates a uuid
    /// \return uuid
    std::string uuid();

public:
    /// \brief Constructor for ChargingStationBase
    /// \param evse_security Pointer to evse_security that manages security related operations; if nullptr
    /// security_configuration must be set
    /// \param security_configuration specifies the file paths that are required to set up the internal evse_security
    /// implementation
    explicit ChargingStationBase(const std::shared_ptr<EvseSecurity> evse_security,
                                 const std::optional<SecurityConfiguration> security_configuration = std::nullopt);
    virtual ~ChargingStationBase(){};
};

} // namespace ocpp

#endif // OCPP_COMMON
