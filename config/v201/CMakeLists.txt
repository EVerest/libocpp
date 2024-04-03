# find all migration files, store only the filenames by showing relative to the folder
file(GLOB MIGRATION_FILE_LIST RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/core_migrations" "${CMAKE_CURRENT_SOURCE_DIR}/core_migrations/*.sql")
list(SORT MIGRATION_FILE_LIST)

# The first file should always start with 1_up so make use of that fact.
# Next we always check the next number "down" and then "up" which come in order since we sorted alphabetically
set(CURRENT_MIGRATION_FILE_ID 1)
set(NEXT_MIGRATION_FILE_TYPE "up")

foreach(MIGRATION_FILE ${MIGRATION_FILE_LIST})
     string(REGEX MATCH "^([0-9]+)_(up|down)(|-.+)\.sql$" MIGRATION_FILE_MATCHED ${MIGRATION_FILE})
     if (MIGRATION_FILE_MATCHED STREQUAL "")
          message(FATAL_ERROR "Migration filename does not match specification: " ${MIGRATION_FILE})
     endif()

     string(CONCAT NEXT_ID "^" ${CURRENT_MIGRATION_FILE_ID} "_")
     if(NOT ${MIGRATION_FILE_MATCHED} MATCHES ${NEXT_ID})
          message(FATAL_ERROR "Skipped migration file ID, expected " ${CURRENT_MIGRATION_FILE_ID} "_*.sql, but got " ${MIGRATION_FILE_MATCHED})
     endif()

     string(APPEND NEXT_ID ${NEXT_MIGRATION_FILE_TYPE})
     if(NOT ${MIGRATION_FILE_MATCHED} MATCHES ${NEXT_ID})
          message(FATAL_ERROR "Missing " ${NEXT_MIGRATION_FILE_TYPE} " migration file: " ${MIGRATION_FILE_MATCHED})
     endif()

     if(NEXT_MIGRATION_FILE_TYPE STREQUAL "up")
          math(EXPR CURRENT_MIGRATION_FILE_ID "${CURRENT_MIGRATION_FILE_ID}+1")
          set(NEXT_MIGRATION_FILE_TYPE "down")
     elseif(NEXT_MIGRATION_FILE_TYPE STREQUAL "down")
          set(NEXT_MIGRATION_FILE_TYPE "up")
     endif()
endforeach()

if (NEXT_MIGRATION_FILE_TYPE STREQUAL "up")
     message(FATAL_ERROR "Down migration file " ${CURRENT_MIGRATION_FILE_ID} "_*.sql is missing up migration file")
endif()

# Since we always add on the up file we need to subtract one here
math(EXPR CURRENT_MIGRATION_FILE_ID "${CURRENT_MIGRATION_FILE_ID}-1")
set(MIGRATION_FILE_VERSION ${CURRENT_MIGRATION_FILE_ID} PARENT_SCOPE)

option(LIBOCPP_INSTALL_DEVICE_MODEL_DATABASE "Install device model database for OCPP201" ON)

list(APPEND CONFIGS
     config.json
     ../logging.ini
)

install(
     FILES ${CONFIGS}
     DESTINATION ${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201
)

list(TRANSFORM MIGRATION_FILE_LIST PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/core_migrations/")
install(FILES ${MIGRATION_FILE_LIST} DESTINATION ${CMAKE_INSTALL_DATADIR}/everest/modules/OCPP201/core_migrations)

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
