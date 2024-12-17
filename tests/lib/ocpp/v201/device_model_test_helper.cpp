#include "device_model_test_helper.hpp"

#include <ocpp/common/database/database_connection.hpp>
#include <ocpp/v201/device_model.hpp>
#include <ocpp/v201/device_model_storage_sqlite.hpp>

namespace ocpp::v201 {
DeviceModelTestHelper::DeviceModelTestHelper(const std::string& database_path, const std::string& migration_files_path,
                                             const std::string& config_path) :
    database_path(database_path),
    migration_files_path(migration_files_path),
    config_path(config_path),
    database_connection(std::make_unique<ocpp::common::DatabaseConnection>(database_path)) {
    this->database_connection->open_connection();
    this->device_model = create_device_model();
}

DeviceModel* DeviceModelTestHelper::get_device_model() {
    if (this->device_model == nullptr) {
        return nullptr;
    }

    return this->device_model.get();
}

void DeviceModelTestHelper::create_device_model_db() {
    InitDeviceModelDb db(this->database_path, this->migration_files_path);
    db.initialize_database(this->config_path, true);
}

std::unique_ptr<DeviceModel> DeviceModelTestHelper::create_device_model() {
    create_device_model_db();
    auto device_model_storage = std::make_unique<DeviceModelStorageSqlite>(this->database_path);
    auto dm = std::make_unique<DeviceModel>(std::move(device_model_storage));

    return dm;
}
} // namespace ocpp::v201
