cmake_minimum_required(VERSION 3.10)

project(Rayce
    VERSION 1.0
    DESCRIPTION "Vulkan Raytracer."
    LANGUAGES CXX
)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(CMAKE_COLOR_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: Debug, Release" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install/rayce/" CACHE PATH "Install path prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

option(RAYCE_BUILD_DOC "Build documentation" ON)
option(RAYCE_PROFILE "Enables profiling when build is Release!" OFF)
option(RAYCE_BUILD_TESTS "Build Unit Tests" OFF)
option(RAYCE_ENABLE_HARD_WARNINGS "Enables some compiler parameters. This should not be enabled, Rayce will NOT build." OFF)

set(VERSION_MAJOR 0 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 1 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
message(STATUS "Rayce Build type: " ${CMAKE_BUILD_TYPE})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Setting project standards.")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)

if(Vulkan_FOUND)
    message(STATUS "Found Vulkan!")
endif()

if(RAYCE_BUILD_DOC)
    find_package(Doxygen REQUIRED dot)

    if(DOXYGEN_FOUND)
        message(STATUS "Found Doxygen!")
        add_subdirectory(doc)
    else()
        message(WARNING "Doxygen not found, documentation will not be build. Please Install Doxygen to build the documentation!")
    endif()
endif()

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file).
                Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
add_subdirectory(dependencies/glfw)
message(STATUS "Added glfw.")

add_subdirectory(dependencies/imgui)
message(STATUS "Added dear imgui.")

if(RAYCE_PROFILE)
    add_subdirectory(dependencies/tracy)
    message(STATUS "Added tracy.")
endif()

if(RAYCE_BUILD_TESTS)
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
    endif()

    add_subdirectory(dependencies/googletest)
    message(STATUS "Added googletest.")
    add_subdirectory(test)
endif()

add_subdirectory(rayce)

# spdlog_enable_warnings(rayce)
add_subdirectory(rayce_gui)
