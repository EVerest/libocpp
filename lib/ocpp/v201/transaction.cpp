// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/transaction.hpp>

namespace ocpp {

namespace v201 {

Transaction EnhancedTransaction::get_transaction() {
<<<<<<< HEAD
    Transaction transaction = {this->transactionId,     boost::none,         this->chargingState,
=======
    Transaction transaction = {this->transactionId,     std::nullopt,         this->chargingState,
>>>>>>> 158e9e7208760f1ead8bffbdb331a1743b260b73
                               this->timeSpentCharging, this->stoppedReason, this->remoteStartId};
    return transaction;
}

int32_t EnhancedTransaction::get_seq_no() {
    this->seq_no += 1;
    return this->seq_no - 1;
}

} // namespace v201

<<<<<<< HEAD
} // namespace ocpp
=======
} // namespace ocpp
>>>>>>> 158e9e7208760f1ead8bffbdb331a1743b260b73
