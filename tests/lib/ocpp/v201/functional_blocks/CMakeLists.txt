target_include_directories(libocpp_unit_tests PUBLIC
        ../mocks
        ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(libocpp_unit_tests PRIVATE
        test_data_transfer.cpp
        test_reservation.cpp)




add_executable(libocpp_unit_tests_functional_block_security)

target_sources(libocpp_unit_tests_functional_block_security PRIVATE
        test_security.cpp)

target_include_directories(libocpp_unit_tests_functional_block_security PUBLIC
        ${PROJECT_SOURCE_DIR}/include/ocpp/common
        ../stubs
        ../mocks
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
        )

MESSAGE("current source dir: ${CMAKE_CURRENT_SOURCE_DIR}, current project dir: ${PROJECT_SOURCE_DIR}, current binary dir: ${CMAKE_CURRENT_BINARY_DIR}")

target_sources(libocpp_unit_tests_functional_block_security PRIVATE
        ../device_model_test_helper.cpp)

target_compile_options(libocpp_unit_tests_functional_block_security
    PRIVATE
        -pedantic-errors
)

target_compile_features(libocpp_unit_tests_functional_block_security PUBLIC cxx_std_17)

target_compile_definitions(libocpp_unit_tests_functional_block_security
    PRIVATE
    MIGRATION_FILES_LOCATION_V201="${MIGRATION_FILES_LOCATION_V201}"
    MIGRATION_FILE_VERSION_V201=${MIGRATION_FILE_VERSION_V201}
    DEVICE_MODEL_DB_LOCATION_V201="${DEVICE_MODEL_DB_LOCATION_V201}"
    TEST_PROFILES_LOCATION_V201="${TEST_PROFILES_LOCATION_V201}"
)

add_test(libocpp_unit_tests_functional_block_security libocpp_unit_tests_functional_block_security)

target_link_libraries(libocpp_unit_tests_functional_block_security PRIVATE
        ocpp
        GTest::gmock_main
        GTest::gtest_main
)

set_target_properties(libocpp_unit_tests_functional_block_security PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_BINARY_DIR})
