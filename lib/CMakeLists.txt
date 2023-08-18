
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

add_library(impolt STATIC)
set(IMPLOT_DIR ./implot)

target_sources(impolt
    PRIVATE
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${IMPLOT_DIR}/implot_demo.cpp
)

target_link_libraries(impolt
    PRIVATE
    imgui)

target_include_directories(impolt
    PUBLIC
    ${IMPLOT_DIR}/
)
add_subdirectory(efp)
add_subdirectory(simple-ble/simpleble)
add_subdirectory(ble-vital-client)
add_subdirectory(serial)


