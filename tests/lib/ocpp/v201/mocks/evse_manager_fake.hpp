// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#pragma once

#include <gmock/gmock.h>

#include <evse_mock.hpp>
#include <ocpp/v201/evse_manager.hpp>

namespace ocpp::v201 {

class EvseManagerFake : public EvseManagerInterface {
private:
    using EvseIteratorImpl = VectorOfUniquePtrIterator<EvseInterface>;

    std::vector<std::unique_ptr<EvseInterface>> evses;
    std::vector<std::unique_ptr<EnhancedTransaction>> transactions;

    DatabaseHandler db_handler{nullptr, ""}; // Only used to allow opening of transactions, will crash if actually used

public:
    explicit EvseManagerFake(size_t nr_of_evses) {
        transactions.resize(nr_of_evses);
        for (size_t i = 0; i < nr_of_evses; i++) {
            auto mock = std::make_unique<EvseMock>();
            ON_CALL(*mock, get_id).WillByDefault(testing::Return(i + 1));
            ON_CALL(*mock, get_transaction).WillByDefault(testing::ReturnRef(this->transactions.at(i)));
            ON_CALL(*mock, has_active_transaction()).WillByDefault(testing::Return(false));
            evses.push_back(std::move(mock));
        }
    }

    EvseIterator begin() override {
        return EvseIterator(std::make_unique<EvseIteratorImpl>(this->evses.begin()));
    }
    EvseIterator end() override {
        return EvseIterator(std::make_unique<EvseIteratorImpl>(this->evses.end()));
    }

    EvseInterface& get_evse(int32_t id) override {
        if (id > this->evses.size()) {
            throw EvseOutOfRangeException(id);
        }
        return *this->evses.at(id - 1);
    }

    const EvseInterface& get_evse(int32_t id) const override {
        if (id > this->evses.size()) {
            throw EvseOutOfRangeException(id);
        }
        return *this->evses.at(id - 1);
    }

    bool does_evse_exist(int32_t id) const override {
        return id <= this->evses.size();
    }

    void open_transaction(int evse_id, const std::string& transaction_id) {
        auto& transaction = this->transactions.at(evse_id - 1);
        transaction = std::make_unique<EnhancedTransaction>(db_handler, false);
        transaction->transactionId = transaction_id;

        auto& mock = this->get_mock(evse_id);
        EXPECT_CALL(mock, get_transaction).WillRepeatedly(testing::ReturnRef(this->transactions.at(evse_id - 1)));
        EXPECT_CALL(mock, has_active_transaction()).WillRepeatedly(testing::Return(true));
    }

    EvseMock& get_mock(int32_t evse_id) {
        return dynamic_cast<EvseMock&>(*evses.at(evse_id - 1).get());
    }
};

} // namespace ocpp::v201