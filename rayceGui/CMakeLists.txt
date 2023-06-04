project(rayceGui)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayceGui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rayceGui.cpp
)

if(WIN32 AND MSVC)
    add_executable(
        rayceGui WIN32
        ${SOURCES}
    )
else()
    add_executable(
        rayceGui
        ${SOURCES}
    )
endif()

set_target_properties(rayceGui
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# add_custom_command(TARGET rayceGui POST_BUILD
# COMMAND ${CMAKE_COMMAND} -E copy_directory
# ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin/res)
target_link_libraries(rayceGui
    PUBLIC
    rayce::rayce
)

target_compile_definitions(rayceGui
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
)

target_compile_options(rayceGui
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: /WX>>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:${RAYCE_ENABLE_HARD_WARNINGS}>: -pedantic -Werror -Wconversion -pedantic-errors>>
)

if (WIN32)
    set(SUBSYSTEM_LINKER_OPTIONS "/SUBSYSTEM:CONSOLE")
else()
    set(SUBSYSTEM_LINKER_OPTIONS "-mwindows")
endif()

target_link_options(rayceGui PRIVATE ${SUBSYSTEM_LINKER_OPTIONS})

install(TARGETS rayceGui DESTINATION bin)

# install(DIRECTORY ${CMAKE_SOURCE_DIR}/res/ DESTINATION bin/res)
install(FILES ${CMAKE_SOURCE_DIR}/imgui.ini DESTINATION bin)
