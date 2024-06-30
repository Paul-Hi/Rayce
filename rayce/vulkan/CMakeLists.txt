message(STATUS "================================================")
message(STATUS "Adding rayce::vulkan!")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE INLINE ${CMAKE_CURRENT_SOURCE_DIR}/*.inl)
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(rayceVulkan
    STATIC
    ${HEADER}
    ${INLINE}
    ${SOURCES}
)

# Depends on assets target
if(WIN32)
add_dependencies(rayceVulkan assets copySlangBinaries)
else()
add_dependencies(rayceVulkan assets)
endif()
add_library(rayce::vulkan ALIAS rayceVulkan)

target_include_directories(rayceVulkan
    PUBLIC
    $<BUILD_INTERFACE:${eigen_SOURCE_DIR}>
    $<BUILD_INTERFACE:${glfw_INCLUDE_DIRS}>
    ../../assets/shaders/src/slang/core
    PRIVATE
    $<BUILD_INTERFACE:${SLANG_INCLUDE_DIRECTORIES}>
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
    ${SLANG_LIB}
    rayce::core
)

target_precompile_headers(rayceVulkan
    PUBLIC
    ${RAYCE_PRECOMPILED_HEADER}
)

set(SLANG_SHADER_BASEPATH "${CMAKE_SOURCE_DIR}/assets/shaders/src/slang/")
target_compile_definitions(rayceVulkan
    PUBLIC
    $<$<CONFIG:Debug>:RAYCE_DEBUG>
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    SLANG_SHADER_BASEPATH="${SLANG_SHADER_BASEPATH}"
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
