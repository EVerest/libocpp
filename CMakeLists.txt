cmake_minimum_required(VERSION 3.14)

project(ocpp
    VERSION 0.8.5
    DESCRIPTION "A C++ implementation of the Open Charge Point Protocol"
    LANGUAGES CXX
)

if (USE_FIND_PACKAGE)
    find_package(everest-cmake 0.1 REQUIRED
      PATHS ../everest-cmake
    )
else()
    
    include(FetchContent)

    set(EXTERNAL_EVEREST_CMAKE_NAME everest-cmake)
    set(EXTERNAL_EVEREST_CMAKE_URL https://github.com/EVerest/everest-cmake.git)
    set(EXTERNAL_EVEREST_CMAKE_TAG v0.1.0)

    message(STATUS "Fetching everest cmake")
    FetchContent_Declare(
        ${EXTERNAL_EVEREST_CMAKE_NAME}
        GIT_REPOSITORY              ${EXTERNAL_EVEREST_CMAKE_URL}
        GIT_TAG                     ${EXTERNAL_EVEREST_CMAKE_TAG}
    )

    FetchContent_MakeAvailable(${EXTERNAL_EVEREST_CMAKE_NAME})

    # Add the cmake directory to the module path
    # this is needed because some submodules (everest-timer and everest-log) still need to find the cmake module using "find_package"
    set(${EXTERNAL_EVEREST_CMAKE_NAME}_DIR ${CMAKE_BINARY_DIR}/_deps/${EXTERNAL_EVEREST_CMAKE_NAME}-src)
    find_package(everest-cmake REQUIRED PATHS ${everest-cmake_DIR})

endif()

# options
option(BUILD_TESTING "Run unit tests" OFF)
option(CMAKE_RUN_CLANG_TIDY "Run clang-tidy" OFF)
option(LIBOCPP_BUILD_EXAMPLES "Build charge_point and central_system binaries." OFF)
option(OCPP_INSTALL "Install the library (shared data might be installed anyway)" ${EVC_MAIN_PROJECT})

# dependencies
find_package(Boost COMPONENTS program_options regex system thread REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

if(NOT DISABLE_EDM)
    evc_setup_edm()

    # In EDM mode, we can't install exports (because the dependencies usually do not install their exports)
    set(OCPP_INSTALL OFF)

    # FIXME (aw): websocketpp doesn't play well with EDM/CPM
    add_library(websocketpp::websocketpp INTERFACE IMPORTED)
    set_target_properties(websocketpp::websocketpp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${websocketpp_SOURCE_DIR}")

else()
    if (USE_FIND_PACKAGE)
        find_package(date REQUIRED)
        find_package(nlohmann_json REQUIRED)
        find_package(nlohmann_json_schema_validator REQUIRED)
        find_package(websocketpp REQUIRED)
    
        find_package(fsm REQUIRED)
        find_package(everest-timer REQUIRED)
        find_package(everest-log REQUIRED)
    else()
        cmake_minimum_required(VERSION 3.12)
        include(dependencies.cmake)
    endif()
endif()

# library code
add_subdirectory(lib)

# config and auxillary files
add_subdirectory(config)


# packaging
if (OCPP_INSTALL)
    install(
        TARGETS ocpp
        EXPORT ocpp-targets
    )

    install(
        DIRECTORY include/
        TYPE INCLUDE
    )

    evc_setup_package(
        NAME everest-ocpp
        NAMESPACE everest
        EXPORT ocpp-targets
        ADDITIONAL_CONTENT
            "find_dependency(OpenSSL)"
            "find_dependency(SQLite3)"
            "find_dependency(websocketpp)"
            "find_dependency(everest-timer)"
    )
endif()


if(LIBOCPP16_BUILD_EXAMPLES)
    message("Building libocpp 1.6 example binaries.")
    add_subdirectory(src)
else()
    message("Not building libocpp 1.6 example binaries.")
endif()

# configure clang-tidy if requested
if(CMAKE_RUN_CLANG_TIDY)
    message("Running clang-tidy")
    string(CONCAT CLANG_TIDY_CHECKS "*,"
                                    "-llvmlibc*,"
                                    "-fuchsia-default-arguments-calls,"
                                    "-fuchsia-overloaded-operator,"
                                    "-fuchsia-statically-constructed-objects,"
                                    "-readability-function-cognitive-complexity,"
                                    "-modernize-use-trailing-return-type,"
                                    "-abseil-string-find-startswith,"
                                    "-abseil-string-find-str-contains,"
                                    ";")
    set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter='.*'
    -checks=${CLANG_TIDY_CHECKS}
    -export-fixes=clang-tidy-fixes.yaml)
endif()

if(BUILD_TESTING_LIBOCPP)
    add_subdirectory(tests)
endif()

# build doxygen documentation if doxygen is available
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY dist/docs)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    doxygen_add_docs(doxygen-${PROJECT_NAME} README.md include lib src)
else()
    message("Doxygen is needed to generate documentation")
endif()
