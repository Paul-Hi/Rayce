project(raycelib)

add_subdirectory(core)
add_subdirectory(vulkan)
add_subdirectory(app)
add_subdirectory(scene)

if(RAYCE_MONOLYTHIC)
    message(STATUS "================================================")
    message(STATUS "Building Shared Monolythic Rayce Library!")

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

    add_library(rayce
        SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/rayce.cpp
    )

    target_compile_definitions(rayce PRIVATE RAYCE_SHARED)

    target_link_libraries(rayce
        PUBLIC
        rayce::core
        rayce::vulkan
        rayce::app
        rayce::scene
    )

    target_include_directories(rayce
        PUBLIC
        ${RAYCE_INCLUDE_DIR}
    )

    add_library(rayce::rayce ALIAS rayce)

    message(STATUS "Installing Shared Monolythic Rayce Library!")

    install(TARGETS rayce DESTINATION lib)
    install(DIRECTORY core/include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY vulkan/include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")
    install(DIRECTORY app/include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")

    message(STATUS "================================================")
endif()