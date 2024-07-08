// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/common/custom_iterators.hpp>
#include <ocpp/v201/evse.hpp>

namespace ocpp {
namespace v201 {

/// \brief Exception used when an evse that does not exist is accessed.
class EvseOutOfRangeException : public std::exception {
public:
    explicit EvseOutOfRangeException(int32_t id) : msg{"Evse with id " + std::to_string(id) + " does not exist"} {
    }

    ~EvseOutOfRangeException() noexcept override = default;

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

/// \brief Class used to access the Evse instances
class EvseManagerInterface {
public:
    using EvseIterator = ForwardIterator<EvseInterface>;

    /// \brief Default destructor
    virtual ~EvseManagerInterface() = default;

    /// \brief Get a reference to the evse with \p id
    /// \note If \p id is not present this could throw an EvseOutOfRangeException
    virtual EvseInterface& get_evse(int32_t id) = 0;

    /// \brief Get a const reference to the evse with \p id
    /// \note If \p id is not present this could throw an EvseOutOfRangeException
    virtual const EvseInterface& get_evse(int32_t id) const = 0;

    /// \brief Check if an evse with \p id exists
    virtual bool does_evse_exist(int32_t id) const = 0;

    /// \brief Gets an iterator pointing to the first evse
    virtual EvseIterator begin() = 0;
    /// \brief Gets an iterator pointing past the last evse
    virtual EvseIterator end() = 0;
};

class EvseManager : public EvseManagerInterface {
private:
    std::vector<std::unique_ptr<EvseInterface>> evses;

public:
    EvseManager(const std::map<int32_t, int32_t>& evse_connector_structure, DeviceModel& device_model,
                std::shared_ptr<DatabaseHandler> database_handler,
                std::shared_ptr<ComponentStateManagerInterface> component_state_manager,
                const std::function<void(const MeterValue& meter_value, EnhancedTransaction& transaction)>&
                    transaction_meter_value_req,
                const std::function<void(int32_t evse_id)>& pause_charging_callback);

    EvseInterface& get_evse(int32_t id) override;
    const EvseInterface& get_evse(int32_t id) const override;

    bool does_evse_exist(int32_t id) const override;

    EvseIterator begin() override;
    EvseIterator end() override;
};

} // namespace v201
} // namespace ocpp