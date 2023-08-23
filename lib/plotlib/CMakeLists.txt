add_library(plotlib INTERFACE)

target_link_libraries(plotlib
    INTERFACE
    imgui
    implot
)

target_include_directories(plotlib
    INTERFACE
    ./include
)