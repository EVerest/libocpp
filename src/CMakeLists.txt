add_executable(charge_point charge_point.cpp)

target_link_libraries(charge_point
    PRIVATE
        Boost::thread
        Boost::program_options
        ocpp
)
