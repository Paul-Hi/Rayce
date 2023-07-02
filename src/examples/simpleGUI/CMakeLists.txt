set(APP_NAME simpleGUI)

set(HEADERS
    ${RAYCE_EXAMPLES_INCLUDE_DIRECTORY}/${APP_NAME}/simpleGUI.hpp
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/simpleGUI.cpp
)

if(WIN32 AND MSVC)
    add_executable(
        ${APP_NAME} WIN32
        ${HEADERS}
        ${SOURCES}
    )
else()
    add_executable(
        ${APP_NAME}
        ${HEADERS}
        ${SOURCES}
    )
endif()

set_target_properties(${APP_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<$<CONFIG:Debug>:/debug>$<$<CONFIG:Release>:/release>/bin
)

target_link_directories(${APP_NAME} PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(${APP_NAME}
    PUBLIC
    rayce::vulkan
    rayce::core
    rayce::app
)

target_include_directories(${APP_NAME}
    PRIVATE
    ${eigen_SOURCE_DIR}
    ${glfw_INCLUDE_DIRS}
    ${RAYCE_INCLUDE_DIRECTORY}
)

target_compile_definitions(${APP_NAME}
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: _CRT_SECURE_NO_WARNINGS>
)

if(WIN32)
    set(SUBSYSTEM_LINKER_OPTIONS "/SUBSYSTEM:CONSOLE")
else()
    set(SUBSYSTEM_LINKER_OPTIONS "-mwindows")
endif()

target_link_options(${APP_NAME} PRIVATE ${SUBSYSTEM_LINKER_OPTIONS})

install(TARGETS ${APP_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION bin/assets)
install(FILES ${CMAKE_SOURCE_DIR}/imgui.ini DESTINATION bin)
