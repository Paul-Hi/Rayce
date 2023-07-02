set(RAYCE_APP_INCLUDE_DIRECTORY ${RAYCE_INCLUDE_DIRECTORY}/app)


set(HEADERS
    ${RAYCE_APP_INCLUDE_DIRECTORY}/rayceApp.hpp
    ${RAYCE_APP_INCLUDE_DIRECTORY}/rayce.hpp
    ${RAYCE_APP_INCLUDE_DIRECTORY}/imguiInterface.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/rayce.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rayceApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imguiInterface.cpp
)

add_library(app
    ${HEADERS}
    ${SOURCES}
)

add_library(rayce::app ALIAS app)

target_include_directories(app
    PUBLIC
    ${RAYCE_INCLUDE_DIRECTORY}
    ${eigen_SOURCE_DIR}
    ${glfw_INCLUDE_DIRS}
    PRIVATE
    ${imgui_SOURCE_DIR}
)

set_target_properties(app
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_directories(app PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(app
    PUBLIC
    rayce::core
    rayce::vulkan
    PRIVATE
    imgui
)

target_compile_definitions(app
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

target_compile_options(app
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)
