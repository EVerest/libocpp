include(CTest)

#region database tests
add_executable(database_tests database_tests.cpp)

target_include_directories(database_tests PUBLIC ${GTEST_INCLUDE_DIRS})

find_package(GTest REQUIRED)

target_link_libraries(database_tests  PRIVATE
    ocpp
    date::date
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES} 
    pthread
)
add_test(NAME database_tests COMMAND database_tests)
#endregion database tests

#region doctest
add_executable(doctest_exe doctest.cpp)

add_test(NAME doctest COMMAND doctest)
#endregion doctest
