// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2024 Pionix GmbH and Contributors to EVerest

#include <ocpp/common/database/database_schema_updater.hpp>

#include <fstream>
#include <regex>

#include <everest/logging.hpp>

namespace ocpp::common {

// Helper functions

enum class Direction {
    Up,
    Down
};

struct MigrationFile {
    fs::path path;
    uint32_t version;
    Direction direction;
};

std::ostream& operator<<(std::ostream& os, const MigrationFile& info) {
    os << "Migration file [" << (info.direction == Direction::Up ? "up" : "down") << "] version " << info.version
       << ", path: " << info.path.c_str();
    return os;
}

std::vector<MigrationFile> get_migration_file_list(const fs::path& migration_file_directory) {
    std::regex filename_pattern{R"(^(\d+)_(up|down)(-[ \S]+|)\.sql$)"};

    std::vector<MigrationFile> result;

    for (auto& entry : fs::directory_iterator(migration_file_directory)) {
        if (entry.is_regular_file() and entry.file_size() > 0) {
            const fs::path& path = entry.path();
            std::cmatch match;
            std::string filename =
                path.filename(); // Store in a variable otherwise after the match the string temporary is gone
            if (std::regex_match(filename.c_str(), match, filename_pattern) and match.size() == 4) {
                // [0] = whole match
                // [1] = version id
                // [2] = up or down
                // [3] = description or empty
                result.push_back(MigrationFile{path, static_cast<uint32_t>(std::stoul(match[1].str())),
                                               match[2] == "up" ? Direction::Up : Direction::Down});
            }
        }
    }

    return result;
}

void filter_and_sort_migration_file_list(std::vector<MigrationFile>& list, Direction direction, uint32_t min_version,
                                         uint32_t max_version) {
    auto filter = [direction, min_version, max_version](const MigrationFile& item) {
        return item.direction != direction or item.version < min_version or item.version > max_version;
    };

    list.erase(std::remove_if(list.begin(), list.end(), filter), list.end());

    std::sort(list.begin(), list.end(), [direction](const auto& a, const auto& b) {
        if (direction == Direction::Up) {
            return a.version < b.version;
        } else {
            return b.version < a.version;
        }
    });
}

bool is_migration_file_sequence_valid(const std::vector<MigrationFile>& list, Direction direction, uint32_t min_version,
                                      uint32_t max_version) {
    auto test_sequence = [direction, &list](uint32_t start, uint32_t end) {
        auto version = start;
        for (const auto& item : list) {
            if (item.version != version) {
                return false;
            }
            version += direction == Direction::Up ? 1 : -1;
        }
        if (list.back().version != end) {
            EVLOG_error << "Missing last migration step";
            return false;
        }
        return true;
    };

    if (direction == Direction::Up) {
        return test_sequence(min_version, max_version);
    } else {
        return test_sequence(max_version, min_version);
    }
}

std::optional<std::vector<MigrationFile>> get_migration_file_sequence(const fs::path& migration_file_directory,
                                                                      Direction direction, uint32_t current_version,
                                                                      uint32_t target_version) {
    auto list = get_migration_file_list(migration_file_directory);

    EVLOG_info << "Migration list:";

    for (auto& item : list) {
        EVLOG_info << item;
    }

    const auto lowest = std::min(current_version, target_version) + 1;
    const auto highest = std::max(current_version, target_version);

    filter_and_sort_migration_file_list(list, direction, lowest, highest);

    EVLOG_info << "Migration files to apply:";

    for (auto& item : list) {
        EVLOG_info << item;
    }

    if (!is_migration_file_sequence_valid(list, direction, lowest, highest)) {
        return std::nullopt;
    }
    return list;
}

DatabaseSchemaUpdater::DatabaseSchemaUpdater(std::shared_ptr<DatabaseConnectionInterface> database) noexcept :
    database(database) {
}

uint32_t DatabaseSchemaUpdater::get_user_version() {
    auto statement = this->database->new_statement("PRAGMA user_version");

    if (statement->step() != SQLITE_ROW) {
        throw std::runtime_error("Could not get user_version from database");
    }
    return statement->column_int(0);
}

void DatabaseSchemaUpdater::set_user_version(uint32_t version) {
    using namespace std::string_literals;

    if (!this->database->execute_statement("PRAGMA user_version = "s + std::to_string(version))) {
        throw std::runtime_error("Could not set user_version in database");
    }
}

bool DatabaseSchemaUpdater::apply_migration_files(const fs::path& migration_file_directory, uint32_t target_version) {
    if (!fs::is_directory(migration_file_directory)) {
        EVLOG_error << "Migration files must be in a directory: " << migration_file_directory.c_str();
        return false;
    }

    if (target_version == 0) {
        EVLOG_error << "Migration target_version 0 is invalid";
        return false;
    }

    uint32_t current_version = 0;

    try {
        this->database->open_connection();
        current_version = this->get_user_version();
        EVLOG_info << "Target version: " << target_version << ", current version: " << current_version;
    } catch (std::runtime_error e) {
        EVLOG_error << "Failure during migration file apply: " << e.what();
        return false;
    }

    if (current_version == target_version) {
        EVLOG_info << "No migrations to apply since versions match: " << current_version;
        this->database->close_connection();
        return true;
    }

    Direction direction = Direction::Up;

    if (current_version > target_version) {
        direction = Direction::Down;
    }

    auto list = get_migration_file_sequence(migration_file_directory, direction, current_version, target_version);

    if (!list.has_value()) {
        EVLOG_error << "Missing migration files in sequence";
        this->database->close_connection();
        return false;
    }

    try {
        this->database->begin_transaction();

        for (const auto& item : list.value()) {
            std::ifstream stream{item.path};
            std::stringstream init_sql;

            init_sql << stream.rdbuf();

            if (!this->database->execute_statement(init_sql.str())) {
                EVLOG_error << "Could not apply migration file " << item.path;
                throw std::runtime_error("Database access error");
            }
        }

        this->set_user_version(target_version);
        this->database->commit_transaction();
    } catch (std::exception e) {
        this->database->close_connection();
        EVLOG_error << "Failure during migration file apply: " << e.what();
        return false;
    }

    this->database->close_connection();
    return true;
}

} // namespace ocpp