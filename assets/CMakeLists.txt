message(STATUS "================================================")
message(STATUS "Adding rayce::assets!")

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.frag
)
set(SLANG_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/raygen.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/closestHit.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/closestHitSphere.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/sphereIntersection.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/miss.slang
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/rendering/raytracing/missShadow.slang
)

file(GLOB_RECURSE SLANG_DEPENDENCIES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang/*)

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
        COMMAND ${SLANGC}
                -profile glsl_460
                -target spirv
                #-O2
                -validate-ir
                -warnings-as-errors all
                -I "${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/slang"
                -o ${OUTPUT_SPIRV_FILE}
                -entry main
                ${SHADER}
        DEPENDS ${SHADER} ${SLANG_DEPENDENCIES}
    )
    list(APPEND SPIRV_BINARY_FILES ${OUTPUT_SPIRV_FILE})
endforeach(SHADER)

add_custom_target(
    assets
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES} ${SLANG_SOURCE_FILES}
)

message(STATUS "================================================")
