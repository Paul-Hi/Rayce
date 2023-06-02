project(rayce)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loguru.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/export.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/macro.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rayceApp.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rayce.hpp
)

set(PRIVATE_HEADERS

    # ${CMAKE_CURRENT_SOURCE_DIR}/src/.../*.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loguru.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayce.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayceApp.cpp
)

add_library(rayce
    SHARED
    ${PUBLIC_HEADERS}
    ${PRIVATE_HEADERS}
    ${SOURCES}
)

add_library(rayce::rayce ALIAS rayce)

target_include_directories(rayce
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(rayce SYSTEM
    PUBLIC
    ${CMAKE_SOURCE_DIR}/dependencies/eigen
)

set_target_properties(rayce
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_libraries(rayce
    PUBLIC
    Vulkan::Vulkan

    # spdlog::spdlog
    imgui

    # tinyfd
    $<$<CONFIG:Release>:$<$<BOOL:${RAYCE_PROFILE}>:tracy>>

    # $<$<BOOL:${RAYCE_BUILD_TESTS}>:glad>
    # tinygltf
    # PRIVATE
    glfw

    # stb
)

target_compile_definitions(rayce
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:RAYCE_WINMAIN>
    $<$<CXX_COMPILER_ID:Clang>:$<$<BOOL:{ CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC" }>:RAYCE_WINMAIN>>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<BOOL:${RAYCE_PROFILE}>:RAYCE_PROFILE>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_WINDOWS_EXPORT>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(rayce
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)
