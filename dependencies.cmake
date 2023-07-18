cmake_minimum_required(VERSION 3.12)
message(STATUS "Fetching all external libs. This may take some time")
    
include(FetchContent)

# Set the external project's name, URL, and tag
set(EXTERNAL_WEBSOCKET_NAME websocketpp)
set(EXTERNAL_WEBSOCKET_URL https://github.com/zaphoyd/websocketpp.git)
set(EXTERNAL_WEBSOCKET_TAG 0.8.2)

set(EXTERNAL_JSON_NAME nlohmann_json)
set(EXTERNAL_JSON_URL https://github.com/nlohmann/json.git)
set(EXTERNAL_JSON_TAG v3.10.5)

set(EXTERNAL_JSON_SCHEMA_NAME nlohmann_json_schema_validator)
set(EXTERNAL_JSON_SCHEMA_URL https://github.com/pboettch/json-schema-validator)
set(EXTERNAL_JSON_SCHEMA_TAG 2.1.0)

set(EXTERNAL_DATE_NAME date)
set(EXTERNAL_DATE_URL https://github.com/HowardHinnant/date.git)
set(EXTERNAL_DATE_TAG v3.0.1)

set(EXTERNAL_EVEREST_TIMER_NAME everest-timer)
set(EXTERNAL_EVEREST_TIMER_URL https://github.com/EVerest/libtimer.git)
set(EXTERNAL_EVEREST_TIMER_TAG v0.1.1)

set(EXTERNAL_EVEREST_LOG_NAME everest-log)
set(EXTERNAL_EVEREST_LOG_URL https://github.com/EVerest/liblog.git)
set(EXTERNAL_EVEREST_LOG_TAG v0.1.0)

set(JSON_BUILD_TESTS        OFF CACHE INTERNAL "Build JSON tests")
set(JSON_MultipleHeaders    ON  CACHE INTERNAL "Build JSON multi header config")
set(BUILD_TESTS             OFF CACHE INTERNAL "Build tests")
set(BUILD_EXAMPLES          OFF CACHE INTERNAL "Build examples")

set(BUILD_TZ_LIB        ON CACHE BOOL "Build TZ lib")
set(HAS_REMOTE_API      0  CACHE BOOL  "Enable remote API")
set(USE_AUTOLOAD        0  CACHE BOOL  "Use autoload")
set(USE_SYSTEM_TZ_DB    ON CACHE BOOL "Use system DB for timezones")

# Fetch websocketpp
message(STATUS "Fetching websocketpp")
FetchContent_Declare(
    ${EXTERNAL_WEBSOCKET_NAME}
    GIT_REPOSITORY              ${EXTERNAL_WEBSOCKET_URL}
    GIT_TAG                     ${EXTERNAL_WEBSOCKET_TAG}
)

# Fetch nlohmann_json
message(STATUS "Fetching nlohmann_json")
FetchContent_Declare(
    ${EXTERNAL_JSON_NAME}
    GIT_REPOSITORY          ${EXTERNAL_JSON_URL}
    GIT_TAG                 ${EXTERNAL_JSON_TAG}
)

# Fetch nlohmann_json_schema
message(STATUS "Fetching nlohmann_json_schema")
FetchContent_Declare(
    ${EXTERNAL_JSON_SCHEMA_NAME}
    GIT_REPOSITORY          ${EXTERNAL_JSON_SCHEMA_URL}
    GIT_TAG                 ${EXTERNAL_JSON_SCHEMA_TAG}
)

# Fetch Date
message(STATUS "Fetching date")
FetchContent_Declare(
    ${EXTERNAL_DATE_NAME}
    GIT_REPOSITORY          ${EXTERNAL_DATE_URL}
    GIT_TAG                 ${EXTERNAL_DATE_TAG}
)

# Fetch everest-timer
message(STATUS "Fetching everest timer")
FetchContent_Declare(
    ${EXTERNAL_EVEREST_TIMER_NAME}
    GIT_REPOSITORY          ${EXTERNAL_EVEREST_TIMER_URL}
    GIT_TAG                 ${EXTERNAL_EVEREST_TIMER_TAG}
)

# Fetch everest-log
message(STATUS "Fetching everest log")
FetchContent_Declare(
    ${EXTERNAL_EVEREST_LOG_NAME}
    GIT_REPOSITORY          ${EXTERNAL_EVEREST_LOG_URL}
    GIT_TAG                 ${EXTERNAL_EVEREST_LOG_TAG}
)

# Find the date package
FetchContent_MakeAvailable(${EXTERNAL_DATE_NAME})

FetchContent_MakeAvailable(${EXTERNAL_WEBSOCKET_NAME})
add_library(websocketpp::websocketpp INTERFACE IMPORTED)
set_target_properties(websocketpp::websocketpp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${websocketpp_SOURCE_DIR}")

FetchContent_MakeAvailable(${EXTERNAL_JSON_NAME})
FetchContent_MakeAvailable(${EXTERNAL_JSON_SCHEMA_NAME})
FetchContent_MakeAvailable(${EXTERNAL_EVEREST_TIMER_NAME})
FetchContent_MakeAvailable(${EXTERNAL_EVEREST_LOG_NAME})
