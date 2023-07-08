message(STATUS "================================================")
message(STATUS "Adding rayce::vulkan!")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(rayceVulkan
    STATIC
    ${HEADER}
    ${SOURCES}
)

# Depends on assets target
add_dependencies(rayceVulkan assets)

add_library(rayce::vulkan ALIAS rayceVulkan)

target_include_directories(rayceVulkan
    PUBLIC
    $<BUILD_INTERFACE:${eigen_SOURCE_DIR}>
    $<BUILD_INTERFACE:${glfw_INCLUDE_DIRS}>
    PRIVATE
    $<BUILD_INTERFACE:${RAYCE_INCLUDE_DIR}>
)

set_target_properties(rayceVulkan
    PROPERTIES
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
)

target_link_libraries(rayceVulkan
    PUBLIC
    Vulkan::Vulkan
    PRIVATE
    rayce::core
)

target_compile_definitions(rayceVulkan
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_LIBRARY>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(rayceVulkan
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
)

set(BUILD_SPIRV_DIR "$<TARGET_FILE_DIR:rayceVulkan>/assets/shaders")
add_custom_command(
    TARGET rayceVulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_SPIRV_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${OUTPUT_SPIRV_DIR} ${BUILD_SPIRV_DIR}
)

message(STATUS "================================================")
