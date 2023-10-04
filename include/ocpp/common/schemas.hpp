// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2023 Pionix GmbH and Contributors to EVerest
#ifndef OCPP_COMMON_SCHEMAS_HPP
#define OCPP_COMMON_SCHEMAS_HPP

#include <map>
#include <regex>
#include <set>
#include <vector>

#include <everest/logging.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ocpp/common/support_older_cpp_versions.hpp>

using json = nlohmann::json;
using json_uri = nlohmann::json_uri;
using json_validator = nlohmann::json_schema::json_validator;

namespace ocpp {

/// \brief Contains the json schema validation for the libocpp config
class Schemas {
private:
    json schema;
    std::shared_ptr<json_validator> validator;
    fs::path schemas_path;
    std::set<fs::path> available_schemas_paths;
    const static std::vector<std::string> profiles;
    const static std::regex date_time_regex;

    /// \brief Loads the root schema "Config.json" from the schemas path
    void load_root_schema();

    /// \brief A custom json schema loader that loads \p schema files relative to the provided \p uri
    void loader(const json_uri& uri, json& schema);

public:
    /// \brief Creates a new Schemas object looking for the root schema file in relation to the provided \p main_dir
    explicit Schemas(fs::path schemas_path);

    /// \brief Provides the config schema
    /// \returns the config schema as as json object
    json get_schema();

    /// \brief Provides the config schema validator
    /// \returns a json_validator for the config
    std::shared_ptr<json_validator> get_validator();

    /// \brief Provides a format checker for the given \p format and \p value
    static void format_checker(const std::string& format, const std::string& value);
};
} // namespace ocpp

#endif // OCPP_COMMON_SCHEMAS_HPP
