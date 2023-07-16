message(STATUS "================================================")
message(STATUS "Adding rayce::scene!")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(scene
    STATIC
    ${HEADERS}
    ${SOURCES}
)

add_library(rayce::scene ALIAS scene)

target_include_directories(scene
    PRIVATE
    $<BUILD_INTERFACE:${RAYCE_INCLUDE_DIR}>
)

set_target_properties(scene
    PROPERTIES
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
)

target_link_libraries(scene
    PRIVATE
    rayce::core
    rayce::vulkan
)

target_precompile_headers(scene
    PUBLIC
    ${RAYCE_PRECOMPILED_HEADER}
)

target_compile_definitions(scene
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_LIBRARY>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(scene
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
)

message(STATUS "================================================")
