add_library(rs232_stream
    INTERFACE
    ./include/rs232-stream.hpp)

target_link_libraries(rs232_stream
    INTERFACE
    serialib)

target_include_directories(rs232_stream
    INTERFACE
    ./include
)