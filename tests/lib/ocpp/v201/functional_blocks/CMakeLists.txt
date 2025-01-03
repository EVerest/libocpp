target_include_directories(libocpp_unit_tests PUBLIC 
        ../mocks
        ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(libocpp_unit_tests PRIVATE
        test_data_transfer.cpp
        test_reservation.cpp)


# add_executable(libocpp_unit_tests_functional_block_authorization)
# add_test(libocpp_unit_tests_functional_block_authorization libocpp_unit_tests_functional_block_authorization
#          WORKING_DIRECTORY ${TEST_BINARY_DIR})

target_sources(libocpp_unit_tests_functional_block_authorization PRIVATE
               test_authorization.cpp)

target_include_directories(libocpp_unit_tests_functional_block_authorization PUBLIC
                           ${PROJECT_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR}/3rd_party
                           ${PROJECT_SOURCE_DIR}/include/ocpp/common
                           ../mocks
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/..
                           ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

MESSAGE("current source dir: ${CMAKE_CURRENT_SOURCE_DIR}, current project dir: ${PROJECT_SOURCE_DIR}, current binary dir: ${CMAKE_CURRENT_BINARY_DIR}")

target_sources(libocpp_unit_tests_functional_block_authorization PRIVATE
               ../device_model_test_helper.cpp)

target_compile_options(libocpp_unit_tests_functional_block_authorization
                       PRIVATE -pedantic-errors
)

target_compile_features(libocpp_unit_tests_functional_block_authorization PUBLIC cxx_std_17)

target_compile_definitions(libocpp_unit_tests_functional_block_authorization
                           PRIVATE
                           MIGRATION_FILES_LOCATION_V201="${MIGRATION_FILES_LOCATION_V201}"
                           MIGRATION_FILE_VERSION_V201=${MIGRATION_FILE_VERSION_V201}
                           DEVICE_MODEL_DB_LOCATION_V201="${DEVICE_MODEL_DB_LOCATION_V201}"
                           TEST_PROFILES_LOCATION_V201="${TEST_PROFILES_LOCATION_V201}"
)

target_link_libraries(libocpp_unit_tests_functional_block_authorization PRIVATE
    # everest::log    # TODO mz fix for not-everest, see lib/CMakeLists.txt
    # nlohmann_json::nlohmann_json    # TODO mz fix for not-everest
    # date::date-tz   # TODO mz fix for not-everest
    # everest::timer  # TODO mz fix for not-everest
    # websocketpp::websocketpp    # TODO mz euh why is this needed???
                      ocpp
                      GTest::gmock_main
                      GTest::gtest_main
)

set_target_properties(libocpp_unit_tests_functional_block_authorization PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_BINARY_DIR})
