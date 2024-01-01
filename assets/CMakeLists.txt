message(STATUS "================================================")
message(STATUS "Adding rayce::assets!")

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.frag
)
set(SLANG_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/raygen.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/closestHit.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/closestHitSphere.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/sphereIntersection.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/miss.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/missShadow.slang
)
set(SLANG_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/bsdf.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/sampling.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/hostDeviceInterop.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/random.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/globals.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/data.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/utils.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/integrator.slang
)

set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" PARENT_SCOPE)
foreach(SHADER ${GLSL_SOURCE_FILES})
    find_program(GLSLC glslc)
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUTPUT_SPIRV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT_SPIRV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_SPIRV_DIR}
        COMMAND ${GLSLC} --target-env=vulkan1.2 -o ${OUTPUT_SPIRV_FILE} ${SHADER} -Werror
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_BINARY_FILES ${OUTPUT_SPIRV_FILE})
endforeach(SHADER)
foreach(SHADER ${SLANG_SOURCE_FILES})
    find_program(SLANGC slangc)
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUTPUT_SPIRV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT_SPIRV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_SPIRV_DIR}
        COMMAND ${SLANGC} ${SHADER} -profile glsl_460 -target spirv -o ${OUTPUT_SPIRV_FILE} -entry main -O3
        DEPENDS ${SHADER} ${SLANG_INCLUDES}
    )
    list(APPEND SPIRV_BINARY_FILES ${OUTPUT_SPIRV_FILE})
endforeach(SHADER)

add_custom_target(
    assets
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES} ${SLANG_SOURCE_FILES}
)

message(STATUS "================================================")
