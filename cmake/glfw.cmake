include(ExternalProject)
find_package(GLFW 3.3 QUIET)

if(GLFW_FOUND)
    message(STATUS "Found GLFW")
else()
    message(STATUS "GLFW not found - will build from source")

    set(BUILD_GLFW 1)

    ExternalProject_Add(glfw PREFIX glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.7

        UPDATE_COMMAND ""

        CMAKE_ARGS
            "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DGLFW_BUILD_EXAMPLES=OFF"
            "-DGLFW_BUILD_TESTS=OFF"
            "-DGLFW_BUILD_DOCS=OFF"

        CMAKE_CACHE_ARGS
            "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"

        BUILD_BYPRODUCTS "<INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX}"

        LOG_DOWNLOAD 1 LOG_UPDATE 1 LOG_CONFIGURE 1 LOG_BUILD 1 LOG_INSTALL 1
    )

    ExternalProject_Get_Property(glfw INSTALL_DIR)

    set(GLFW_INCLUDE_DIR ${INSTALL_DIR}/include)
    set(GLFW_LIBRARIES ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})

    if(UNIX)
        find_package(Threads REQUIRED)
        find_package(X11 REQUIRED)

        if (NOT X11_FOUND)
            message(FATAL_ERROR "X11 libraries not found - required for GLFW")
        endif()

        list(APPEND GLFW_LIBRARIES "${X11_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" -lrt -ldl)
    endif()
endif()

set(GLFW_INCLUDE_DIR ${GLFW_INCLUDE_DIR} CACHE STRING "")
set(GLFW_LIBRARIES ${GLFW_LIBRARIES} CACHE STRING "")
