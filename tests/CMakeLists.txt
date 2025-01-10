set(CONFIG_DIR_V16 "${CMAKE_CURRENT_BINARY_DIR}/resources/v16/")
set(MIGRATION_FILES_LOCATION_V16 "${CMAKE_CURRENT_BINARY_DIR}/resources/v16/migration_files")
set(MIGRATION_FILES_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/v201/migration_files")
set(CONFIG_FILE_LOCATION_V16 ${CMAKE_CURRENT_SOURCE_DIR}/config/v16/resources/config.json)
set(USER_CONFIG_FILE_LOCATION_V16 ${CMAKE_CURRENT_SOURCE_DIR}/config/v16/resources/user_config.json)
set(CONFIG_FILE_RESOURCES_LOCATION_V16 "${CMAKE_CURRENT_BINARY_DIR}/resources/config/v16/config.json")
set(USER_CONFIG_FILE_RESOURCES_LOCATION_V16 "${CMAKE_CURRENT_BINARY_DIR}/resources/config/v16/user_config.json")

set(MIGRATION_FILES_DEVICE_MODEL_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/v201/device_model_migration_files")
set(DEVICE_MODEL_DB_LOCATION_V201 "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201/device_model_storage.db")
set(DEVICE_MODEL_RESOURCES_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/config/v201")
set(DEVICE_MODEL_RESOURCES_CHANGED_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/config/v201/changed")
set(DEVICE_MODEL_RESOURCES_WRONG_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/config/v201/wrong")
set(DEVICE_MODEL_CURRENT_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config/v201/resources)
set(DEVICE_MODEL_CURRENT_CHANGED_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config/v201/resources_changed)
set(DEVICE_MODEL_CURRENT_WRONG_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/config/v201/resources_wrong)
set(DEVICE_MODEL_EXAMPLE_CONFIG_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/example_config/v201/component_config")
set(DEVICE_MODEL_CURRENT_EXAMPLE_CONFIG_LOCATION_V201 "${PROJECT_SOURCE_DIR}/config/v201/component_config")
set(TEST_PROFILES_LOCATION_V16 "${CMAKE_CURRENT_BINARY_DIR}/resources/profiles/v16")
set(TEST_PROFILES_LOCATION_V201 "${CMAKE_CURRENT_BINARY_DIR}/resources/profiles/v201")

add_executable(libocpp_unit_tests)

# For libocpp tests, there is one big executable, which links against the ocpp lib and all other libs.
# When it is useful to link only to the tested cpp files, a separate executable can be created for each file.
# The source files can be added to this variable, which is a list. For example:
# list(APPEND SEPARATE_UNIT_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ocpp/v201/functional_blocks/test_security.cpp)
# CMake will then create a new test executable based on the filename and adds 'libocpp_' in front of it. In above
# example, a test executable 'libocpp_test_security' will be created. In this example, in the CMakeLists of
# `lib/ocpp/v201/functional_blocks`, files to link against can be added to this target / executable.
#
# For each test in this list, cmake will link agaist some 'default' cpp files (like utils and enums etc), set all
# correct flags, add a test, set definitions, etc. See below.
set(SEPARATE_UNIT_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ocpp/common/utils_tests.cpp)

# Add separate tests for V201 only.
if(LIBOCPP_ENABLE_V201)
    # Add all v201 tests you don't want to include in the default test executable here.
    # Example (remove if there are real tests added):
    # list(APPEND SEPARATE_UNIT_TESTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/ocpp/v201/functional_blocks/test_security.cpp)
endif()

# Add variables that can be used for all tests if needed.
set(GTEST_LIBRARIES GTest::gmock_main GTest::gtest_main)
set(TEST_COMPILE_OPTIONS -pedantic-errors)
set(TEST_COMPILE_DEFINITIONS
    CONFIG_FILE_LOCATION_V16="${CONFIG_FILE_RESOURCES_LOCATION_V16}"
    USER_CONFIG_FILE_LOCATION_V16="${USER_CONFIG_FILE_RESOURCES_LOCATION_V16}"
    CONFIG_DIR_V16="${CONFIG_DIR_V16}"
    MIGRATION_FILES_LOCATION_V16="${MIGRATION_FILES_LOCATION_V16}"
    MIGRATION_FILES_LOCATION_V201="${MIGRATION_FILES_LOCATION_V201}"
    MIGRATION_FILE_VERSION_V16=${MIGRATION_FILE_VERSION_V16}
    MIGRATION_FILE_VERSION_V201=${MIGRATION_FILE_VERSION_V201}
    DEVICE_MODEL_DB_LOCATION_V201="${DEVICE_MODEL_DB_LOCATION_V201}"
    TEST_PROFILES_LOCATION_V16="${TEST_PROFILES_LOCATION_V16}"
    TEST_PROFILES_LOCATION_V201="${TEST_PROFILES_LOCATION_V201}")
set(TEST_COMPILE_FEATURES cxx_std_17)
set(LIBOCPP_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include)
set(LIBOCPP_LIB_PATH ${PROJECT_SOURCE_DIR}/lib)
set(LIBOCPP_3RDPARTY_PATH ${PROJECT_SOURCE_DIR}/3rd_party)
set(TEST_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CMAKE_CURRENT_SOURCE_DIR}/lib/ocpp/common
                             ${LIBOCPP_INCLUDE_PATH}
)
# If the test is not linked against the ocpp library, most probably those libraries are needed to link against.
set(LIBOCPP_TEST_DEFAULT_LINK_LIBRARIES
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
        date::date-tz
        everest::log
        everest::evse_security
)

# If the test is not linked against the ocpp library, those default sources can be linked against, they will often
# be needed to link against.
set(LIBOCPP_TEST_INCLUDE_COMMON_SOURCES ${LIBOCPP_LIB_PATH}/ocpp/common/types.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/ocpp_logging.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/utils.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/call_types.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/evse_security.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/database/database_connection.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/database/database_handler_common.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/database/sqlite_statement.cpp
                                        ${LIBOCPP_LIB_PATH}/ocpp/common/database/database_schema_updater.cpp
)

# If the test is not linked against the ocpp library, those default sources for ocpp v2.0.1 can be linked against, they
# will often be needed to link against.
set(LIBOCPP_TEST_INCLUDE_V201_SOURCES ${LIBOCPP_LIB_PATH}/ocpp/v16/ocpp_enums.cpp   # This is currently still needed but might be removed in the future.
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/ctrlr_component_variables.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/types.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/ocpp_types.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/enums.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/ocpp_enums.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/device_model.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/init_device_model_db.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/device_model_storage_sqlite.cpp
                                      ${LIBOCPP_LIB_PATH}/ocpp/v201/utils.cpp
)

target_compile_definitions(libocpp_unit_tests
    PRIVATE
    ${TEST_COMPILE_DEFINITIONS}
)

set(TEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(libocpp_unit_tests
    PRIVATE
        ${TEST_COMPILE_OPTIONS}
)

target_compile_features(libocpp_unit_tests PUBLIC ${TEST_COMPILE_FEATURES})

add_custom_command(TARGET libocpp_unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/unittest_device_model.db ${CMAKE_CURRENT_BINARY_DIR}/resources/unittest_device_model.db
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/unittest_device_model_missing_required.db ${CMAKE_CURRENT_BINARY_DIR}/resources/unittest_device_model_missing_required.db
        COMMAND ${CMAKE_COMMAND} -E copy ${CONFIG_FILE_LOCATION_V16} ${CONFIG_FILE_RESOURCES_LOCATION_V16}
        COMMAND ${CMAKE_COMMAND} -E copy ${USER_CONFIG_FILE_LOCATION_V16} ${USER_CONFIG_FILE_RESOURCES_LOCATION_V16}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CONFIG_DIR_V16}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${OCPP1_6_CONFIG_DIR} ${CONFIG_DIR_V16}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${MIGRATION_FILES_LOCATION_V16}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MIGRATION_FILES_SOURCE_DIR_V16} ${MIGRATION_FILES_LOCATION_V16}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${MIGRATION_FILES_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MIGRATION_FILES_SOURCE_DIR_V201} ${MIGRATION_FILES_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${MIGRATION_FILES_DEVICE_MODEL_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${MIGRATION_FILES_DEVICE_MODEL_SOURCE_DIR_V201} ${MIGRATION_FILES_DEVICE_MODEL_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DEVICE_MODEL_RESOURCES_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEVICE_MODEL_CURRENT_RESOURCES_DIR} ${DEVICE_MODEL_RESOURCES_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DEVICE_MODEL_RESOURCES_CHANGED_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEVICE_MODEL_CURRENT_CHANGED_RESOURCES_DIR} ${DEVICE_MODEL_RESOURCES_CHANGED_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DEVICE_MODEL_RESOURCES_WRONG_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEVICE_MODEL_CURRENT_WRONG_RESOURCES_DIR} ${DEVICE_MODEL_RESOURCES_WRONG_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DEVICE_MODEL_EXAMPLE_CONFIG_LOCATION_V201}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEVICE_MODEL_CURRENT_EXAMPLE_CONFIG_LOCATION_V201} ${DEVICE_MODEL_EXAMPLE_CONFIG_LOCATION_V201}
)

add_test(libocpp_unit_tests libocpp_unit_tests)

target_include_directories(libocpp_unit_tests PUBLIC
        ${TEST_INCLUDE_DIRECTORIES}
)
target_link_libraries(libocpp_unit_tests PRIVATE
        ocpp
        ${GTEST_LIBRARIES}
)

set(GCOVR_DEPENDENCIES libocpp_unit_tests)

# Add executables, link libraries etc for the unit tests that are separate and do not link against the ocpp lib.
# This loops over all tests added to `SEPARATE_UNIT_TESTS` and does the necessary things to make it build.
# For now, everything is added that seems to be needed in every test. If later some sources and link libraries should
# not be added here, they can always be removed from this loop and added to the test targets that actually need them.
foreach(ITEM ${SEPARATE_UNIT_TESTS})
    set(TEST_ROOT_NAME)
    cmake_path(GET ITEM STEM TEST_ROOT_NAME)
    set(TEST_NAME "libocpp_${TEST_ROOT_NAME}")
    add_executable(${TEST_NAME} ${ITEM})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${TEST_NAME} PUBLIC ${GTEST_LIBRARIES})
    list(APPEND GCOVR_DEPENDENCIES ${TEST_NAME})
    target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_COMPILE_DEFINITIONS})
    target_compile_definitions(${TEST_NAME} PRIVATE
        MIGRATION_FILE_VERSION_V16=${MIGRATION_FILE_VERSION_V16}
        MIGRATION_FILE_VERSION_V201=${MIGRATION_FILE_VERSION_V201}
        MIGRATION_DEVICE_MODEL_FILE_VERSION_V201=${MIGRATION_DEVICE_MODEL_FILE_VERSION_V201})
    target_compile_options(${TEST_NAME} PRIVATE ${TEST_COMPILE_OPTIONS})
    target_compile_features(${TEST_NAME} PUBLIC ${TEST_COMPILE_FEATURES})
    target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDE_DIRECTORIES})
    target_link_libraries(${TEST_NAME} PRIVATE ${LIBOCPP_TEST_DEFAULT_LINK_LIBRARIES})
    message("Add test ${TEST_NAME}")
endforeach(ITEM)

# Subdirectories should be added only after adding the tests, because they have to exist for the CMakeLists.txt in the
# child directories.
add_subdirectory(lib/ocpp/common)

if(LIBOCPP_ENABLE_V16)
    add_subdirectory(lib/ocpp/v16)
endif()

if(LIBOCPP_ENABLE_V201)
    add_subdirectory(lib/ocpp/v201)
endif()

setup_target_for_coverage_gcovr_html(
    NAME ${PROJECT_NAME}_gcovr_coverage
    EXECUTABLE ctest
    DEPENDENCIES libocpp_unit_tests ${GCOVR_DEPENDENCIES}
    EXCLUDE "src/*" "tests/*"
)

setup_target_for_coverage_gcovr_xml(
    NAME ${PROJECT_NAME}_gcovr_coverage_xml
    EXECUTABLE ctest
    DEPENDENCIES libocpp_unit_tests ${GCOVR_DEPENDENCIES}
    EXCLUDE "src/*" "tests/*"
)
