// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <everest/logging.hpp>
#include <ocpp/common/database/sqlite_statement.hpp>
#include <ocpp/v201/device_model_storage_sqlite.hpp>

namespace ocpp {

using namespace common;

namespace v201 {

DeviceModelStorageSqlite::DeviceModelStorageSqlite(const fs::path& db_path) {
    db = std::make_unique<ocpp::common::DatabaseConnection>(db_path);

    if (!db->open_connection()) {
        EVLOG_error << "Could not open database at provided path: " << db_path;
        EVLOG_AND_THROW(std::runtime_error("Could not open device model database at provided path."));
    } else {
        EVLOG_info << "Established connection to device model database successfully: " << db_path;
    }
}

int DeviceModelStorageSqlite::get_component_id(const Component& component_id) {
    std::string select_query =
        "SELECT ID FROM COMPONENT WHERE NAME = ? AND INSTANCE IS ? AND EVSE_ID IS ? AND CONNECTOR_ID IS ?";

    auto select_stmt = this->db->new_statement(select_query);

    select_stmt->bind_text(1, component_id.name.get(), SQLiteString::Transient);
    if (component_id.instance.has_value()) {
        select_stmt->bind_text(2, component_id.instance.value().get(), SQLiteString::Transient);
    } else {
        select_stmt->bind_null(2);
    }
    if (component_id.evse.has_value()) {
        select_stmt->bind_int(3, component_id.evse.value().id);
        if (component_id.evse.value().connectorId.has_value()) {
            select_stmt->bind_int(4, component_id.evse.value().connectorId.value());
        } else {
            select_stmt->bind_null(4);
        }
    } else {
        select_stmt->bind_null(3);
    }

    if (select_stmt->step() == SQLITE_ROW) {
        return select_stmt->column_int(0);
    } else {
        return -1;
    }
}

int DeviceModelStorageSqlite::get_variable_id(const Component& component_id, const Variable& variable_id) {
    const auto _component_id = this->get_component_id(component_id);
    if (_component_id == -1) {
        return -1;
    }

    std::string select_query = "SELECT ID FROM VARIABLE WHERE COMPONENT_ID = ? AND NAME = ? AND INSTANCE IS ?";
    auto select_stmt = this->db->new_statement(select_query);

    select_stmt->bind_int(1, _component_id);
    select_stmt->bind_text(2, variable_id.name.get(), SQLiteString::Transient);
    if (variable_id.instance.has_value()) {
        select_stmt->bind_text(3, variable_id.instance.value().get(), SQLiteString::Transient);
    } else {
        select_stmt->bind_null(3);
    }
    if (select_stmt->step() == SQLITE_ROW) {
        return select_stmt->column_int(0);
    } else {
        return -1;
    }
}

DeviceModelMap DeviceModelStorageSqlite::get_device_model() {
    std::map<Component, std::map<Variable, VariableMetaData>> device_model;

    std::string select_query =
        "SELECT c.NAME, c.EVSE_ID, c.CONNECTOR_ID, c.INSTANCE, v.NAME, v.INSTANCE, vc.DATATYPE_ID, "
        "vc.SUPPORTS_MONITORING, vc.UNIT, vc.MIN_LIMIT, vc.MAX_LIMIT, vc.VALUES_LIST "
        "FROM COMPONENT c "
        "JOIN VARIABLE v ON c.ID = v.COMPONENT_ID "
        "JOIN VARIABLE_CHARACTERISTICS vc ON v.VARIABLE_CHARACTERISTICS_ID = vc.ID";

    auto select_stmt = this->db->new_statement(select_query);

    while (select_stmt->step() == SQLITE_ROW) {
        Component component;
        component.name = select_stmt->column_text(0);

        if (select_stmt->column_type(1) != SQLITE_NULL) {
            auto evse_id = select_stmt->column_int(1);
            EVSE evse;
            evse.id = evse_id;
            if (select_stmt->column_type(2) != SQLITE_NULL) {
                evse.connectorId = select_stmt->column_int(2);
            }
            component.evse = evse;
        }

        if (select_stmt->column_type(3) != SQLITE_NULL) {
            component.instance = select_stmt->column_text(3);
        }

        Variable variable;
        variable.name = select_stmt->column_text(4);

        if (select_stmt->column_type(5) != SQLITE_NULL) {
            variable.instance = select_stmt->column_text(5);
        }

        VariableCharacteristics characteristics;
        characteristics.dataType = static_cast<DataEnum>(select_stmt->column_int(6));
        characteristics.supportsMonitoring = select_stmt->column_int(7) != 0;

        if (select_stmt->column_type(8) != SQLITE_NULL) {
            characteristics.unit = select_stmt->column_text(8);
        }

        if (select_stmt->column_type(9) != SQLITE_NULL) {
            characteristics.minLimit = select_stmt->column_double(9);
        }

        if (select_stmt->column_type(10) != SQLITE_NULL) {
            characteristics.maxLimit = select_stmt->column_double(10);
        }

        if (select_stmt->column_type(11) != SQLITE_NULL) {
            characteristics.valuesList = select_stmt->column_text(11);
        }

        VariableMetaData meta_data;
        meta_data.characteristics = characteristics;

        // TODO(ioan): Query monitors for this variable

        device_model[component][variable] = meta_data;
    }

    EVLOG_info << "Successfully retrieved Device Model from DeviceModelStorage";
    return device_model;
}

std::optional<VariableAttribute> DeviceModelStorageSqlite::get_variable_attribute(const Component& component_id,
                                                                                  const Variable& variable_id,
                                                                                  const AttributeEnum& attribute_enum) {
    const auto attributes = this->get_variable_attributes(component_id, variable_id, attribute_enum);
    if (!attributes.empty()) {
        return attributes.at(0);
    } else {
        return std::nullopt;
    }
}

std::vector<VariableAttribute>
DeviceModelStorageSqlite::get_variable_attributes(const Component& component_id, const Variable& variable_id,
                                                  const std::optional<AttributeEnum>& attribute_enum) {
    std::vector<VariableAttribute> attributes;
    const auto _variable_id = this->get_variable_id(component_id, variable_id);
    if (_variable_id == -1) {
        return attributes;
    }

    std::string select_query = "SELECT va.VALUE, va.MUTABILITY_ID, va.PERSISTENT, va.CONSTANT, va.TYPE_ID "
                               "FROM VARIABLE_ATTRIBUTE va "
                               "WHERE va.VARIABLE_ID = @variable_id";

    if (attribute_enum.has_value()) {
        std::stringstream ss;
        ss << select_query << "  AND va.TYPE_ID = " << static_cast<int>(attribute_enum.value());
        select_query = ss.str();
    }

    auto select_stmt = this->db->new_statement(select_query);

    select_stmt->bind_int(1, _variable_id);

    while (select_stmt->step() == SQLITE_ROW) {
        VariableAttribute attribute;

        if (select_stmt->column_type(0) != SQLITE_NULL) {
            attribute.value = select_stmt->column_text(0);
        }
        attribute.mutability = static_cast<MutabilityEnum>(select_stmt->column_int(1));
        attribute.persistent = static_cast<bool>(select_stmt->column_int(2));
        attribute.constant = static_cast<bool>(select_stmt->column_int(3));
        attribute.type = static_cast<AttributeEnum>(select_stmt->column_int(4));
        attributes.push_back(attribute);
    }

    return attributes;
}

bool DeviceModelStorageSqlite::set_variable_attribute_value(const Component& component_id, const Variable& variable_id,
                                                            const AttributeEnum& attribute_enum,
                                                            const std::string& value) {
    std::string insert_query = "UPDATE VARIABLE_ATTRIBUTE SET VALUE = ? WHERE VARIABLE_ID = ? AND TYPE_ID = ?";
    auto insert_stmt = this->db->new_statement(insert_query);

    const auto _variable_id = this->get_variable_id(component_id, variable_id);

    if (_variable_id == -1) {
        return false;
    }

    insert_stmt->bind_text(1, value);
    insert_stmt->bind_int(2, _variable_id);
    insert_stmt->bind_int(3, static_cast<int>(attribute_enum));
    if (insert_stmt->step() != SQLITE_DONE) {
        EVLOG_error << this->db->get_error_message();
        return false;
    }
    return true;
}

bool DeviceModelStorageSqlite::set_monitoring_data(const SetMonitoringData& data) {
    const auto _variable_id = this->get_variable_id(data.component, data.variable);
    if (_variable_id == -1) {
        return false;
    }

    // TODO (ioan): see if we already have an existing monitor?

    std::string insert_query;

    if (data.id.has_value()) {
        insert_query =
            "INSERT OR REPLACE INTO VARIABLE_MONITORING (VARIABLE_ID, SEVERITY, 'TRANSACTION', TYPE_ID, VALUE) "
            "VALUES (?, ?, ?, ?, ?)";
    } else {
        insert_query =
            "INSERT OR REPLACE INTO VARIABLE_MONITORING (VARIABLE_ID, SEVERITY, 'TRANSACTION', TYPE_ID, VALUE, ID) "
            "VALUES (?, ?, ?, ?, ?, ?)";
    }

    SQLiteStatement insert_stmt(this->db, insert_query);

    insert_stmt.bind_int(1, _variable_id);
    insert_stmt.bind_int(2, data.severity);
    insert_stmt.bind_int(3, data.transaction.value_or(false));
    insert_stmt.bind_int(4, static_cast<int>(data.type));
    insert_stmt.bind_double(5, data.value);

    if (data.id.has_value()) {
        insert_stmt.bind_int(6, data.id.value());
    }

    if (insert_stmt.step() != SQLITE_DONE) {
        EVLOG_error << sqlite3_errmsg(this->db);
        return false;
    }
    return true;
}

std::optional<MonitoringData>
DeviceModelStorageSqlite::get_monitoring_data(const std::vector<MonitoringCriterionEnum>& criteria,
                                              const Component& component_id, const Variable& variable_id) {
    if (criteria.size() <= 0) {
        return std::nullopt;
    }

    const auto _variable_id = this->get_variable_id(component_id, variable_id);
    if (_variable_id == -1) {
        return std::nullopt;
    }

    // TODO (ioan): optimize select based on criterions
    std::string select_query = "SELECT vm.TYPE_ID, vm.ID, vm.SEVERITY, vm.'TRANSACTION', vm.VALUE "
                               "FROM VARIABLE_MONITORING vm "
                               "WHERE vm.VARIABLE_ID = @variable_id";

    SQLiteStatement select_stmt(this->db, select_query);
    select_stmt.bind_int(1, _variable_id);

    std::vector<VariableMonitoring> monitors;

    while (select_stmt.step() == SQLITE_ROW) {
        VariableMonitoring monitor;

        // Filter based on type first
        monitor.type = static_cast<MonitorEnum>(select_stmt.column_int(0));
        bool filter_match = true;

        for (auto& criterion : criteria) {
            switch (criterion) {
            case MonitoringCriterionEnum::DeltaMonitoring:
                filter_match = (monitor.type == MonitorEnum::Delta);
                break;
            case MonitoringCriterionEnum::ThresholdMonitoring:
                filter_match =
                    (monitor.type == MonitorEnum::LowerThreshold || monitor.type == MonitorEnum::UpperThreshold);
                break;
            case MonitoringCriterionEnum::PeriodicMonitoring:
                filter_match =
                    (monitor.type == MonitorEnum::Periodic || monitor.type == MonitorEnum::PeriodicClockAligned);
                break;
            }

            if (!filter_match) {
                break;
            }
        }

        if (filter_match) {
            monitor.id = select_stmt.column_int(1);
            monitor.severity = select_stmt.column_int(2);
            monitor.transaction = static_cast<bool>(select_stmt.column_int(3));
            monitor.value = static_cast<float>(select_stmt.column_double(4));

            monitors.push_back(monitor);
        }
    }

    if (monitors.size() > 0) {
        MonitoringData monitor;

        monitor.component = component_id;
        monitor.variable = variable_id;
        monitor.variableMonitoring = std::move(monitors);

        return monitor;
    }

    return std::nullopt;
}

bool DeviceModelStorageSqlite::clear_variable_monitor(int monitor_id) {
    std::string delete_query = "DELETE FROM VARIABLE_MONITORING WHERE ID = ?";

    SQLiteStatement delete_stmt(this->db, delete_query);

    delete_stmt.bind_int(1, monitor_id);
    if (delete_stmt.step() != SQLITE_DONE) {
        EVLOG_error << sqlite3_errmsg(this->db);
        return false;
    }

    // Ensure that we deleted 1 row
    return (delete_stmt.changes() == 1);
}

void DeviceModelStorageSqlite::check_integrity() {

    // Check for required variables without actual values
    std::stringstream query_stream;
    query_stream << "SELECT c.NAME as 'COMPONENT_NAME', "
                    "c.EVSE_ID as 'EVSE_ID', "
                    "c.CONNECTOR_ID as 'CONNECTOR_ID', "
                    "v.NAME as 'VARIABLE_NAME', "
                    "v.INSTANCE as 'VARIABLE_INSTANCE' "
                    "FROM VARIABLE_ATTRIBUTE va "
                    "JOIN VARIABLE v ON v.ID = va.VARIABLE_ID "
                    "JOIN COMPONENT c ON v.COMPONENT_ID = c.ID "
                    "WHERE va.TYPE_ID = "
                 << static_cast<int>(AttributeEnum::Actual)
                 << " AND va.VALUE IS NULL"
                    " AND v.REQUIRED = 1";
    auto select_stmt = this->db->new_statement(query_stream.str());

    if (select_stmt->step() != SQLITE_DONE) {
        std::stringstream error;
        error << "Corrupted device model: Missing the following required values for 'Actual' Variable Attributes:"
              << std::endl;
        do {
            error << "(Component/EvseId/ConnectorId/Variable/Instance: " << select_stmt->column_text(0) << "/"
                  << select_stmt->column_text_nullable(1).value_or("<null>") << "/"
                  << select_stmt->column_text_nullable(2).value_or("<null>") << "/" << select_stmt->column_text(3)
                  << "/" << select_stmt->column_text_nullable(4).value_or("<null>") << ")" << std::endl;
        } while (select_stmt->step() == SQLITE_ROW);

        throw DeviceModelStorageError(error.str());
    }
}

} // namespace v201
} // namespace ocpp
