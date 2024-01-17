cmake_minimum_required(VERSION 3.26)

project(Rayce
    VERSION 0.01
    DESCRIPTION "Vulkan Raytracer."
    LANGUAGES CXX
)

set(CMAKE_COLOR_MAKEFILE ON)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build, options are: Debug, Release" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install/rayce/" CACHE PATH "Install path prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

option(RAYCE_BUILD_DOC "Build Documentation" OFF)
option(RAYCE_MONOLYTHIC "Build Monolythic" ON)
option(RAYCE_BUILD_TESTS "Build Unit Tests" OFF)

set(VERSION_MAJOR 0 CACHE STRING "Project major version number.")
set(VERSION_MINOR 0 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 1 CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

message(STATUS "================================================")
message(STATUS " ########     ###    ##    ##  ######  ######## ")
message(STATUS " ##     ##   ## ##    ##  ##  ##    ## ##       ")
message(STATUS " ##     ##  ##   ##    ####   ##       ##       ")
message(STATUS " ########  ##     ##    ##    ##       ######   ")
message(STATUS " ##   ##   #########    ##    ##       ##       ")
message(STATUS " ##    ##  ##     ##    ##    ##    ## ##       ")
message(STATUS " ##     ## ##     ##    ##     ######  ######## ")
message(STATUS "================================================")
message(STATUS "Copyright (c) 2023-present, Paul Himmler")
message(STATUS "================================================")
message(STATUS "Version v" ${VERSION_MAJOR} "." ${VERSION_MINOR} ${VERSION_PATCH} " Licensed Apache License 2.0")
message(STATUS "================================================")
message(STATUS "Build Type: " ${CMAKE_BUILD_TYPE})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "Setting Project Standards.")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)

if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

message(STATUS "================================================")

message(STATUS "Fetching Dependencies.")

# dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
include(FetchContent)
set(FETCHCONTENT_QUIET TRUE)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)


# Vulkan
find_package(Vulkan REQUIRED)

# Slang
find_package(Slang REQUIRED)

# USD
# set(PXR_DIR $ENV{PXR_DIR} CACHE STRING "The root directory of the USD installation")
# set(MATERIALX_DIR $ENV{MATERIALX_DIR} CACHE STRING "PXR_DIR/lib/cmake/MaterialX")
# find_package(PXR REQUIRED)

cmake_language(GET_MESSAGE_LOG_LEVEL CMAKE_MESSAGE_LOG_LEVEL_BACKUP)
set(CMAKE_MESSAGE_LOG_LEVEL ERROR)
set(FETCHCONTENT_QUIET ON)

# imgui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.89.7-docking
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)

# Eigen
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    GIT_VERBOSE ON
)

# glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.3.8
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)

FetchContent_MakeAvailable(imgui eigen glfw)
set(CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL_BACKUP})

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
message(STATUS "Added glfw in: " ${glfw_SOURCE_DIR})
set(glfw_INCLUDE_DIRS ${glfw_SOURCE_DIR}/include)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC . ${glfw_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS} ${imgui_SOURCE_DIR})
target_link_directories(imgui PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

message(STATUS "Added dear imgui in: " ${imgui_SOURCE_DIR})

unset(BUILD_TESTING CACHE)
message(STATUS "Added Eigen in: " ${eigen_SOURCE_DIR})

if(RAYCE_BUILD_TESTS)
    # googletest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG b796f7d44681514f58a683a3a71ff17c94edb0c1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    message(STATUS "Added googletest: " ${googletest_SOURCE_DIR})

    add_subdirectory(test)
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

message(STATUS "================================================")

set(RAYCE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rayce)

add_subdirectory(assets)

add_subdirectory(rayce)

add_subdirectory(examples)

message(STATUS "Done!")
message(STATUS "================================================")
