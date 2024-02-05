add_definitions(-D_SQL_INIT_FILE="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP/init.sql")
add_executable(${PROJECT_NAME}_database_tests database_tests.cpp)

target_include_directories(${PROJECT_NAME}_database_tests PUBLIC ${GTEST_INCLUDE_DIRS})

find_package(GTest REQUIRED)

target_link_libraries(${PROJECT_NAME}_database_tests PRIVATE
    ocpp
    date::date
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES} 
    pthread
    )


add_test(${PROJECT_NAME}_database_tests ${PROJECT_NAME}_database_tests)

add_executable(${PROJECT_NAME}_unit_tests
        lib/ocpp/v201/test_component_state_manager.cpp)

add_custom_command(TARGET ${PROJECT_NAME}__unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/unittest_device_model.db ${CMAKE_CURRENT_BINARY_DIR}/resources/unittest_device_model.db
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/unittest_device_model_missing_required.db ${CMAKE_CURRENT_BINARY_DIR}/resources/unittest_device_model_missing_required.db
)

add_test(${PROJECT_NAME}_unit_tests ${PROJECT_NAME}_unit_tests)

target_include_directories(${PROJECT_NAME}_unit_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${GTEST_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_unit_tests PRIVATE
        ocpp
        GTest::gmock_main
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)
target_sources(${PROJECT_NAME}_unit_tests PRIVATE
        comparators.cpp)

add_subdirectory(lib/ocpp/v201)
add_subdirectory(lib/ocpp/common)


add_executable(${PROJECT_NAME}_utils_tests utils_tests.cpp)

target_include_directories(${PROJECT_NAME}_utils_tests PUBLIC ${GTEST_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}_utils_tests PRIVATE
        ocpp
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

add_test(${PROJECT_NAME}_utils_tests ${PROJECT_NAME}_utils_tests)
