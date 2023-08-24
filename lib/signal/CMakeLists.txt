add_library(signal INTERFACE)
target_link_libraries(signal
    INTERFACE
    efp
    fft)

target_include_directories(signal
    INTERFACE
    ./include)
