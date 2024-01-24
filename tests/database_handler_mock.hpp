// SPDX-License-Identifier: Apache-2.0

#ifndef OCPP_DATABASE_HANDLE_MOCK_H
#define OCPP_DATABASE_HANDLE_MOCK_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ocpp/v16/database_handler.hpp>

namespace ocpp {

class DatabaseHandlerMock : public DatabaseHandler {
public:
    
};

} // namespace ocpp

#endif // DATABASE_HANDLE_MOCK_H