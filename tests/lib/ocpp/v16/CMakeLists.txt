target_sources(libocpp_unit_tests PRIVATE
#        internal.cpp
        profile_tests_common.cpp
        profile_testsA.cpp
        profile_testsB.cpp
        profile_testsC.cpp
        test_database_migration_files.cpp
        test_smart_charging_handler.cpp
        test_smart_charging_handler.cpp
)

target_compile_definitions(libocpp_unit_tests PRIVATE BUILD_TESTING_LIBOCPP)