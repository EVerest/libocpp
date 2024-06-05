// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ocpp/v201/init_device_model_db.hpp>

#include <cstdint>
#include <map>
#include <string>

#include <everest/logging.hpp>

const static std::string STANDARDIZED_COMPONENT_SCHEMAS_DIR = "standardized";
const static std::string CUSTOM_COMPONENT_SCHEMAS_DIR = "custom";

/* clang-format off */
const static std::map<std::string, uint8_t> DATATYPE_ENCODINGS = {
    { "string", 0 },
    { "decimal", 1 },
    { "integer", 2 },
    { "dateTime", 3 },
    { "boolean", 4 },
    { "OptionList", 5 },
    { "SequenceList", 6 },
    { "MemberList", 7 }
};

const static std::map<std::string, uint8_t> MUTABILITY_ENCODINGS = {
    { "ReadOnly", 0 },
    { "WriteOnly", 1 },
    { "ReadWrite", 2 },
};

const static std::map<std::string, uint8_t> VARIABLE_ATTRIBUTE_TYPE_ENCODING = {
    { "Actual", 0},
    { "Target", 1},
    { "MinSet", 2},
    { "MaxSet", 3}
};
/* clang-format on */

ocpp::v201::InitDeviceModelDb::InitDeviceModelDb(const std::filesystem::path& database_path) :
    database_path(database_path) {
}

bool ocpp::v201::InitDeviceModelDb::initialize_database(const std::filesystem::path& schemas_path,
                                                        bool delete_db_if_exists = true) {
    if (!execute_init_sql(delete_db_if_exists)) {
        // TODO throw??
        return false;
    }
}

bool ocpp::v201::InitDeviceModelDb::insert_config_and_default_values(const std::filesystem::path& schemas_path,
                                                                     const std::filesystem::path& config_path) {
}

bool ocpp::v201::InitDeviceModelDb::execute_init_sql(const bool delete_db_if_exists) {
    if (delete_db_if_exists) {
        if (std::filesystem::exists(database_path)) {
            if (!std::filesystem::remove(database_path)) {
                EVLOG_error << "Could not remove database " << database_path.u8string();
                // TODO log or return false / throw?
                return false;
            }
        }
    }

    // Connect to the database.

    // Open sql initialization file.

    // Execute the sql init script

    return true;
}
