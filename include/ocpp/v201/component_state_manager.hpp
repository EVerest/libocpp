// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#pragma once

#include <ocpp/v201/enums.hpp>
#include "database_handler.hpp"

namespace ocpp::v201 {

struct FullConnectorStatus {
    OperationalStatusEnum individual_operational_status;
    bool faulted;
    bool reserved;
    bool occupied;

    ConnectorStatusEnum to_effective_status();
};

class ComponentStateManager {
private:
    std::shared_ptr<DatabaseHandler> database;

    OperationalStatusEnum cs_individual_status;
    std::vector<std::pair<OperationalStatusEnum, std::vector<FullConnectorStatus>>>
        evse_and_connector_individual_statuses;

    void check_evse_id(int32_t evse_id);
    void check_evse_and_connector_id(int32_t evse_id, int32_t connector_id);

public:
    explicit ComponentStateManager(const std::map<int32_t, int32_t>& evse_connector_structure,
                                   std::shared_ptr<DatabaseHandler> db_handler);

    OperationalStatusEnum get_cs_individual_operational_status();
    OperationalStatusEnum get_evse_individual_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_individual_operational_status(int32_t evse_id, int32_t connector_id);

    OperationalStatusEnum get_cs_persisted_operational_status();
    OperationalStatusEnum get_evse_persisted_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_persisted_operational_status(int32_t evse_id, int32_t connector_id);

    void set_cs_individual_operational_status(OperationalStatusEnum new_status, bool persist);
    void set_evse_individual_operational_status(int32_t evse_id, OperationalStatusEnum new_status, bool persist);
    void set_connector_individual_operational_status(int32_t evse_id, int32_t connector_id,
                                                     OperationalStatusEnum new_status,bool persist);

    OperationalStatusEnum get_evse_effective_operational_status(int32_t evse_id);
    OperationalStatusEnum get_connector_effective_operational_status(int32_t evse_id, int32_t connector_id);
    ConnectorStatusEnum get_connector_effective_status(int32_t evse_id, int32_t connector_id);

    void set_connector_occupied(int32_t evse_id, int32_t connector_id, bool is_occupied);
    void set_connector_reserved(int32_t evse_id, int32_t connector_id, bool is_reserved);
    void set_connector_faulted(int32_t evse_id, int32_t connector_id, bool is_faulted);
};

} // namespace ocpp::v201
