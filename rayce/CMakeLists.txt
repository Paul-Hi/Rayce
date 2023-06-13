project(rayce)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/loguru.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/export.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/log.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/macro.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rayceApp.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rayce.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imguiInterface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/immediateSubmit.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/window.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/instance.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/framebuffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/renderPass.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/image.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/imageView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/imageMemoryBarrier.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/commandPool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/descriptorPool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/commandBuffers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/surface.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/swapchain.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/semaphore.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/fence.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/shaderModule.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/graphicsPipeline.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/raytracingPipeline.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/vertex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/geometry.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/deviceMemory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/accelerationStructure.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkan/rtFunctions.hpp
)

set(PRIVATE_HEADERS

    # ${CMAKE_CURRENT_SOURCE_DIR}/src/.../*.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/loguru.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayce.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayceApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/framebuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/renderPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/imageView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/commandPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/descriptorPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/commandBuffers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/surface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/swapchain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/fence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/shaderModule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/graphicsPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/raytracingPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/geometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/deviceMemory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/accelerationStructure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan/rtFunctions.cpp
)

add_library(rayce
    SHARED
    ${PUBLIC_HEADERS}
    ${PRIVATE_HEADERS}
    ${SOURCES}
)

# Depends on assets target
add_dependencies(rayce assets)

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
    $<$<BOOL:${RAYCE_BUILD_TESTS}>:RAYCE_TEST>
    $<$<BOOL:${RAYCE_PROFILE}>:RAYCE_PROFILE>
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
    PRIVATE
    $<$<BOOL:${WIN32}>:RAYCE_WINDOWS_EXPORT>
    $<$<BOOL:${WIN32}>:WIN32>
    $<$<BOOL:${LINUX}>:LINUX>
)

target_compile_options(rayce
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)

set(BUILD_SPIRV_DIR "$<TARGET_FILE_DIR:rayce>/assets/shaders")
add_custom_command(
    TARGET rayce POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_SPIRV_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${OUTPUT_SPIRV_DIR} ${BUILD_SPIRV_DIR}
)
