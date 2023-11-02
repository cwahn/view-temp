add_executable(mk9-wifi-view
    "src/main.cpp"
)


find_package(ZeroMQ QUIET)

# libzmq autotools install: fallback to pkg-config
if(NOT ZeroMQ_FOUND)
  message(STATUS "CMake libzmq package not found, trying again with pkg-config (normal install of zeromq)")
  list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
  find_package(ZeroMQ REQUIRED)
endif()

# TODO "REQUIRED" above should already cause a fatal failure if not found, but this doesn't seem to work
if(NOT ZeroMQ_FOUND)
  message(FATAL_ERROR "ZeroMQ was not found, neither as a CMake package nor via pkg-config")
endif()



find_package(Flatbuffers REQUIRED)


if(APPLE)
  target_link_directories(mk9-wifi-view
    PRIVATE
    "/opt/homebrew/lib"
  )
endif()

target_link_libraries(mk9-wifi-view
    PRIVATE
    imgui
    implot
    efp
    plotlib
    signal
    zmq
    flatbuffers
)
