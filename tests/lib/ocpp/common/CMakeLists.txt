add_executable(message_queue_tests test_message_queue.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/database_handler_base.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/call_types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/message_queue.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/ocpp_logging.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/common/utils.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v16/enums.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v16/ocpp_types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v16/types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v201/enums.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v201/ocpp_types.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/ocpp/v201/types.cpp"
)


target_include_directories(message_queue_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../include" ${GTEST_INCLUDE_DIRS})

find_package(GTest REQUIRED)

#############
# Logging configuration
#############
if (EVEREST_CUSTOM_LOGGING_INCLUDE_PATH)
    if (NOT EXISTS "${EVEREST_CUSTOM_LOGGING_INCLUDE_PATH}/everest/logging.hpp")
        message(FATAL_ERROR "everest/logging.hpp not found in directory ${EVEREST_CUSTOM_LOGGING_INCLUDE_PATH}")
    else()
        target_include_directories(message_queue_tests
                PUBLIC
                include
                ${EVEREST_CUSTOM_LOGGING_INCLUDE_PATH}
        )
    endif()
    message(STATUS "Using the following logging header: ${EVEREST_CUSTOM_LOGGING_INCLUDE_PATH}/everest/logging.hpp")
endif()

if (NOT EVEREST_CUSTOM_LOGGING_INCLUDE_PATH)
    target_link_libraries(message_queue_tests
            PUBLIC
            everest::log
    )
    message(STATUS "Using the default logging header")
endif()

#############
# End logging configuration
#############

target_link_libraries(message_queue_tests PRIVATE
        GTest::gmock_main
        everest::timer
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)


add_test(message_queue_tests message_queue_tests)


target_sources(libocpp_unit_tests PRIVATE test_message_queue.cpp)