// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <comparators.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v201/component_state_manager.hpp>

namespace ocpp::v201 {


class DatabaseHandlerMock : public DatabaseHandler {
private:
    std::map<std::pair<int32_t, int32_t>, OperationalStatusEnum> data;

    void insert(int32_t evse_id, int32_t connector_id, OperationalStatusEnum status, bool replace) {
        if (replace || this->data.count(std::make_pair(evse_id, connector_id)) == 0) {
            this->data.insert_or_assign(std::make_pair(evse_id, connector_id), status);
        }
    }

    OperationalStatusEnum get(int32_t evse_id, int32_t connector_id) {
        if (this->data.count(std::make_pair(evse_id, connector_id)) == 0) {
            throw std::logic_error("Get: no data available");
        } else {
            return this->data.at(std::make_pair(evse_id, connector_id));
        }
    }

public:
    DatabaseHandlerMock() : DatabaseHandler("/dev/null", "/dev/null") {
    }

    virtual void insert_cs_availability(OperationalStatusEnum operational_status, bool replace) override {
        this->insert(0, 0, operational_status, replace);
    }
    virtual OperationalStatusEnum get_cs_availability() override {
        return this->get(0, 0);
    }

    virtual void insert_evse_availability(int32_t evse_id, OperationalStatusEnum operational_status,
                                          bool replace) override {

        this->insert(evse_id, 0, operational_status, replace);
    }
    virtual OperationalStatusEnum get_evse_availability(int32_t evse_id) override {
        return this->get(evse_id, 0);
    }

    virtual void insert_connector_availability(int32_t evse_id, int32_t connector_id,
                                               OperationalStatusEnum operational_status, bool replace) override {
        this->insert(evse_id, connector_id, operational_status, replace);
    }
    virtual OperationalStatusEnum get_connector_availability(int32_t evse_id, int32_t connector_id) override {
        return this->get(evse_id, connector_id);
    }
};

class MockCallbacks {
public:
    MOCK_METHOD(bool, connector_status_update, (int32_t, int32_t, std::string), ());
    MOCK_METHOD(void, cs_op_state_update, (std::string), ());
    MOCK_METHOD(void, evse_op_state_update, (int32_t, std::string), ());
    MOCK_METHOD(void, connector_op_state_update, (int32_t, int32_t, std::string), ());
};

class ComponentStateManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        this->mock_database = std::make_shared<DatabaseHandlerMock>();
    }

    ComponentStateManager component_state_manager(std::vector<uint32_t> connector_structure) {
        std::map<int32_t, int32_t> evse_connector_structure;
        for (int i = 0; i < connector_structure.size(); i++) {
            evse_connector_structure.insert_or_assign(i + 1, connector_structure[i]);
        }

        ComponentStateManager mgr(evse_connector_structure, this->mock_database,
                                  [this](int32_t evse_id, int32_t connector_id, ConnectorStatusEnum status) {
                                      return this->callbacks.connector_status_update(
                                          evse_id, connector_id, conversions::connector_status_enum_to_string(status));
                                      return true;
                                  });
        mgr.set_cs_effective_availability_changed_callback(
            [this](OperationalStatusEnum old_status, OperationalStatusEnum status) {
                this->callbacks.cs_op_state_update(conversions::operational_status_enum_to_string(status));
            });
        mgr.set_evse_effective_availability_changed_callback(
            [this](int32_t evse_id, OperationalStatusEnum old_status, OperationalStatusEnum status) {
                this->callbacks.evse_op_state_update(evse_id, conversions::operational_status_enum_to_string(status));
            });
        mgr.set_connector_effective_availability_changed_callback([this](int32_t evse_id, int32_t connector_id,
                                                                         OperationalStatusEnum old_status,
                                                                         OperationalStatusEnum status) {
            this->callbacks.connector_op_state_update(evse_id, connector_id,
                                                      conversions::operational_status_enum_to_string(status));
        });

        return mgr;
    }

    void TearDown() override {
    }

    std::shared_ptr<DatabaseHandler> mock_database;
    MockCallbacks callbacks;
};

/// \brief Test that the ComponentStateManager can be constructed on an empty database
TEST_F(ComponentStateManagerTest, test_boot_empty_db) {
    auto state_mgr = this->component_state_manager({1, 2, 3});
}

} // namespace ocpp::v201