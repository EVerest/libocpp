target_include_directories(libocpp_unit_tests PUBLIC 
        mocks
        ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(libocpp_unit_tests PRIVATE
        device_model_test_helper.cpp
        smart_charging_test_utils.cpp
        test_charge_point.cpp
        test_database_handler.cpp
        test_database_migration_files.cpp
        test_device_model_storage_sqlite.cpp
        test_notify_report_requests_splitter.cpp
        test_ocsp_updater.cpp
        test_component_state_manager.cpp
        test_database_handler.cpp
        test_device_model.cpp
        test_init_device_model_db.cpp
        comparators.cpp
        test_message_queue.cpp
        test_composite_schedule.cpp
        test_profile.cpp
        )

# Copy the json files used for testing to the destination directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/json DESTINATION ${TEST_PROFILES_LOCATION_V2})

set(LIBOCPP_TESTS_V2_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(functional_blocks)
