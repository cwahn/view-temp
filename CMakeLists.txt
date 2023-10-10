cmake_minimum_required(VERSION 3.0.0)

option(BUILD_SHARED_LIBS OFF)

project(imgui-test VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(MSVC)
    # to properly link with static library
    # do not define _USE_MATH_DEFINES in source code
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /D_USE_MATH_DEFINES")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /D_USE_MATH_DEFINES")
endif()


include(CTest)
enable_testing()

# Ubuntu Requirment
# libasound2-dev pkg-config libglfw3 libglfw3-dev libdbus-1-dev

find_package(glfw3 REQUIRED)
if(NOT glfw3_FOUND)
    if(WIN32)
        message("Prebuilt GLFW3 used.")
        file(DOWNLOAD https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip ${CMAKE_BINARY_DIR}/lib/glfw-3.3.8.bin.WIN64.zip)

        execute_process(
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND tar -xf ${CMAKE_BINARY_DIR}/lib/glfw-3.3.8.bin.WIN64.zip -C ${CMAKE_BINARY_DIR}/lib
        )

        set(GLFW3_INCLUDE_DIR ${CMAKE_BINARY_DIR}/lib/glfw-3.3.8.bin.WIN64/include)
        set(GLFW3_LIBRARY ${CMAKE_BINARY_DIR}/lib/glfw-3.3.8.bin.WIN64/lib-vc2022/glfw3_mt.lib)
    else()
        # todo: support other OS
        message(WARNING "Prebuilt library of GLFW3 not supported.")
    endif()
endif()

find_package(OpenGL REQUIRED)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(app)
add_subdirectory(lib)