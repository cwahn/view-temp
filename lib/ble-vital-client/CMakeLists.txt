add_library(ble_vital_client
    STATIC
    src/utils.cpp)

target_link_libraries(ble_vital_client
    PRIVATE
    simpleble
)

target_include_directories(ble_vital_client
    PUBLIC
    ./include
)
