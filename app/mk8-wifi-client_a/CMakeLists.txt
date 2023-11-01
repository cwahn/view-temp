add_executable(mk8-wifi-client_a
    "src/main.cpp"
)



if (NOT MOSQUITTO_INCLUDE_DIR)
  find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
endif()

if (NOT MOSQUITTO_LIBRARY)
  find_library(
    MOSQUITTO_LIBRARY
    NAMES mosquitto)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  MOSQUITTO DEFAULT_MSG
  MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)

message(STATUS "libmosquitto include dir: ${MOSQUITTO_INCLUDE_DIR}")
message(STATUS "libmosquitto: ${MOSQUITTO_LIBRARY}")
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY})

mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY)


find_package(Flatbuffers REQUIRED)


if(APPLE)
  target_link_directories(mk8-wifi-client_a
    PRIVATE
    "/opt/homebrew/lib"
  )
endif()

target_link_libraries(mk8-wifi-client_a
    PRIVATE
    imgui
    implot
    efp
    plotlib
    signal
    mosquitto
    flatbuffers
)
