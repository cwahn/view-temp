add_executable(mk8-guitest
    "src/main.cpp"
)


target_link_libraries(mk8-guitest
    PRIVATE
    imgui
    implot
    plotlib
    signal
)
