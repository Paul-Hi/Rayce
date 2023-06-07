if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.frag
)

set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" PARENT_SCOPE)
foreach(SHADER ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUTPUT_SPIRV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT_SPIRV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_SPIRV_DIR}
        COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${OUTPUT_SPIRV_FILE}
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_BINARY_FILES ${OUTPUT_SPIRV_FILE})
endforeach(SHADER)

add_custom_target(
    assets
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES}
)
