// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#include <ocpp/common/incremental_counter.hpp>

namespace ocpp {

std::atomic<unsigned int> IncrementalCounter::counter{0};

int IncrementalCounter::get() {
    return ++counter;
}

} // namespace ocpp