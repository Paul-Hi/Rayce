set(RAYCE_VK_INCLUDE_DIRECTORY ${RAYCE_INCLUDE_DIRECTORY}/vk)

set(HEADER
    ${RAYCE_VK_INCLUDE_DIRECTORY}/immediateSubmit.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/window.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/instance.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/device.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/framebuffer.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/renderPass.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/image.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/imageView.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/imageMemoryBarrier.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/commandPool.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/descriptorPool.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/commandBuffers.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/descriptorSets.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/surface.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/swapchain.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/semaphore.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/fence.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/shaderModule.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/graphicsPipeline.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/raytracingPipeline.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/vertex.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/geometry.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/buffer.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/deviceMemory.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/accelerationStructure.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/rtFunctions.hpp
    ${RAYCE_VK_INCLUDE_DIRECTORY}/descriptorSetLayout.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/framebuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imageView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptorPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptorSets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/surface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fence.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaderModule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphicsPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raytracingPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deviceMemory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/accelerationStructure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rtFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptorSetLayout.cpp
)

add_library(rayceVulkan
    ${HEADER}
    ${SOURCES}
)

# Depends on assets target
add_dependencies(rayceVulkan assets)

add_library(rayce::vulkan ALIAS rayceVulkan)

target_include_directories(rayceVulkan
    PUBLIC
    ${RAYCE_INCLUDE_DIRECTORY}
    ${eigen_SOURCE_DIR}
    ${glfw_INCLUDE_DIRS}
)

set_target_properties(rayceVulkan
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_directories(rayceVulkan PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(rayceVulkan
    PUBLIC
    Vulkan::Vulkan
    rayce::core
)

target_compile_definitions(rayceVulkan
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

target_compile_options(rayceVulkan
    PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)

set(BUILD_SPIRV_DIR "$<TARGET_FILE_DIR:rayceVulkan>/assets/shaders")
add_custom_command(
    TARGET rayceVulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_SPIRV_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${OUTPUT_SPIRV_DIR} ${BUILD_SPIRV_DIR}
)
