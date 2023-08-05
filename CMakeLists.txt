cmake_minimum_required(VERSION 3.0.0)
project(imgui-test VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(CTest)
enable_testing()

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3.8 REQUIRED)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(app)
add_subdirectory(lib)