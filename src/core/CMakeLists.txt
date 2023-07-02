set(RAYCE_CORE_INCLUDE_DIRECTORY ${RAYCE_INCLUDE_DIRECTORY}/core)


set(HEADERS
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/types.hpp
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/loguru.hpp
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/export.hpp
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/log.hpp
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/macro.hpp
    ${RAYCE_CORE_INCLUDE_DIRECTORY}/utils.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/loguru.cpp
)

add_library(core
    ${HEADERS}
    ${SOURCES}
)

add_library(rayce::core ALIAS core)

target_include_directories(core
    PUBLIC
    ${RAYCE_INCLUDE_DIRECTORY}
    ${eigen_SOURCE_DIR}
    ${glfw_INCLUDE_DIRS}
    PRIVATE
    ${imgui_SOURCE_DIR}
)

set_target_properties(core
PROPERTIES
ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_directories(core PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(core
    PUBLIC
    Eigen3::Eigen
    glfw
    PRIVATE
    imgui
)

target_compile_definitions(core
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<BOOL:${RAYCE_PROFILE}>:RAYCE_PROFILE>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_WINDOWS_EXPORT>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(core
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)

# copy glfw dlls
add_custom_command(
    TARGET core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${glfw_BINARY_DIR}/src$<$<CONFIG:Debug>:/Debug>$<$<CONFIG:Release>:/Release>/ $<TARGET_FILE_DIR:core>
)