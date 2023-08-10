message(STATUS "================================================")
message(STATUS "Adding rayce::core!")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(APPEND SOURCES ../../assets/shaders/src/host_device.hpp)

add_library(core
    STATIC
    ${HEADERS}
    ${SOURCES}
)

add_library(rayce::core ALIAS core)

target_include_directories(core
    PUBLIC
    $<BUILD_INTERFACE:${eigen_SOURCE_DIR}>
    $<BUILD_INTERFACE:${glfw_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
    PRIVATE
    $<BUILD_INTERFACE:${RAYCE_INCLUDE_DIR}>
)

set_target_properties(core
    PROPERTIES
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
)

target_link_libraries(core
    PUBLIC
    Eigen3::Eigen
    PRIVATE
    glfw
    imgui
)

target_precompile_headers(core
    PUBLIC
    ${RAYCE_PRECOMPILED_HEADER}
)

target_compile_definitions(core
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_LIBRARY>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(core
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
)

message(STATUS "================================================")
