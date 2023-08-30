add_library(imgui STATIC)
set(IMGUI_DIR ./imgui)

target_sources(imgui
    PRIVATE
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp

    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui
    PUBLIC
    ${IMGUI_DIR}/
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui
    PUBLIC
    OpenGL::GL
    glfw)

add_library(implot STATIC)
set(IMPLOT_DIR ./implot)

target_sources(implot
    PRIVATE
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${IMPLOT_DIR}/implot_demo.cpp
)

target_link_libraries(implot
    PRIVATE
    imgui)

target_include_directories(implot
    PUBLIC
    ${IMPLOT_DIR}/
)

add_library(fft esp32-fft/src/fft.c)
target_include_directories(fft PUBLIC esp32-fft/src)

add_subdirectory(efp)
add_subdirectory(simple-ble/simpleble)
add_subdirectory(ble-vital-client)
add_subdirectory(serialib)
add_subdirectory(rs232-stream)
add_subdirectory(plotlib)
add_subdirectory(signal)
add_subdirectory(rtaudio)
add_subdirectory(audio-stream)
