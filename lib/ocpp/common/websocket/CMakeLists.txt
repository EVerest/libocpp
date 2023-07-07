
target_sources(ocpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/websocket_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/websocket_plain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/websocket_tls.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/websocket.cpp
)
