add_library(audio-stream INTERFACE)
target_link_libraries(audio-stream
    INTERFACE
    efp
    rtaudio)
target_include_directories(audio-stream
    INTERFACE
    ./include)