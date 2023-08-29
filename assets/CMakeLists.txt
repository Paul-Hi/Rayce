message(STATUS "================================================")
message(STATUS "Adding rayce::assets!")


find_program(GLSLC glslc)

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/basic.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/raygen.rgen
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/closestHit.rchit
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/sphereIntersection.rint
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/closestHitSphere.rchit
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/src/miss.rmiss
)

set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(OUTPUT_SPIRV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders" PARENT_SCOPE)
foreach(SHADER ${GLSL_SOURCE_FILES})
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

add_custom_target(
    assets
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES}
)

message(STATUS "================================================")
