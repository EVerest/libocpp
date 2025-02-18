// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest

#pragma once

#include <atomic>

namespace ocpp {
class IncrementalCounter {
public:
    static int get();

private:
    static std::atomic<unsigned int> counter;
};

} // namespace ocpp
