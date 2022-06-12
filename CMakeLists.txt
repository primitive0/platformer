cmake_minimum_required(VERSION 3.22)
project(unnamed-platformer)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(glfw)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE MyTarget_SRC
        "src/*.h"
        "src/*.cpp")

add_executable(MyTarget ${MyTarget_SRC})
if(BUILD_GLFW)
        add_dependencies(MyTarget glfw)
endif()

target_include_directories(MyTarget
        PUBLIC ${GLFW_INCLUDE_DIR}
        PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries(MyTarget
        PUBLIC ${Vulkan_LIBRARIES}
        PUBLIC ${GLFW_LIBRARIES})
