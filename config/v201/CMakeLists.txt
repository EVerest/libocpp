set(SQL_INIT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/init_core.sql")

# find all migration files, store only the filenames by showing relative to the folder
file(GLOB TEST_VAR RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/core_migrations" "${CMAKE_CURRENT_SOURCE_DIR}/core_migrations/*.sql")

include(CMakePrintHelpers)
cmake_print_variables(TEST_VAR)

# set()

foreach(path ${TEST_VAR})
     string(REGEX MATCH "^([0-9]+)_(up|down)(|-.+)\.sql$" path2 ${path})
     message(${path})
     if (path2 STREQUAL "")
          message(FATAL_ERROR "Migration filename does not match specification: " ${path})
     endif()
endforeach()


set(MIGRATION_FILE_VERSION 3 PARENT_SCOPE)

option(LIBOCPP_INSTALL_DEVICE_MODEL_DATABASE "Install device model database for OCPP201" ON)

list(APPEND CONFIGS
     config.json
     ../logging.ini
)

install(
     FILES ${CONFIGS}
     DESTINATION ${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201
)

install(FILES ${SQL_INIT_FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201)

if (LIBOCPP_INSTALL_DEVICE_MODEL_DATABASE)
     set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/config.json)

     find_program(
          PYTHON_EXECUTABLE
          python3
          REQUIRED
     )

     set(INIT_DEVICE_MODULE_DB_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/init_device_model_db.py")
     set(DEVICE_MODEL_DATABASE_FILE "device_model_storage.db")

     if(NOT LIBOCPP_V201_CONFIG_FILE)
          set(LIBOCPP_V201_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
     endif()

     if (NOT LIBOCPP_COMPONENT_SCHEMAS_PATH)
          set(LIBOCPP_COMPONENT_SCHEMAS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/component_schemas")
     endif()

     
     message(STATUS "Using ocpp v201 config file path: ${LIBOCPP_V201_CONFIG_FILE}")
     
     execute_process(
             COMMAND
             ${PYTHON_EXECUTABLE} ${INIT_DEVICE_MODULE_DB_SCRIPT} init --db ${CMAKE_CURRENT_BINARY_DIR}/${DEVICE_MODEL_DATABASE_FILE} --schemas ${LIBOCPP_COMPONENT_SCHEMAS_PATH}
          WORKING_DIRECTORY 
               ${CMAKE_CURRENT_SOURCE_DIR}
     )

     execute_process(
             COMMAND
             ${PYTHON_EXECUTABLE} ${INIT_DEVICE_MODULE_DB_SCRIPT} insert --db ${CMAKE_CURRENT_BINARY_DIR}/${DEVICE_MODEL_DATABASE_FILE} --schemas ${LIBOCPP_COMPONENT_SCHEMAS_PATH} --config ${LIBOCPP_V201_CONFIG_FILE}
          WORKING_DIRECTORY 
               ${CMAKE_CURRENT_SOURCE_DIR}
     )

     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DEVICE_MODEL_DATABASE_FILE} DESTINATION ${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201)
endif()
