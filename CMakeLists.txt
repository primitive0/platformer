cmake_minimum_required(VERSION 3.22)
project(unnamed-platformer)

include(CMakeDependentOption)

if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(APP_BUILD_RELEASE 1)
endif()

cmake_dependent_option(MSVC_STATIC_LINK "Static link Visual C++ Runtime" ON "MSVC;APP_BUILD_RELEASE" OFF)

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

if(MSVC AND MSVC_STATIC_LINK)
        set_property(TARGET MyTarget PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()
