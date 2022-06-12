cmake_minimum_required(VERSION 3.22)

project(unnamed-platformer)
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE MyTarget_SRC
        "src/*.h"
        "src/*.cpp")

add_executable(MyTarget ${MyTarget_SRC})

target_link_libraries(MyTarget
        PUBLIC ${Vulkan_LIBRARIES}
        PUBLIC ${GLFW_STATIC_LIBRARIES})
