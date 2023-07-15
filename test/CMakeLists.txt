project(raycetests)

add_executable(allTests
    mockClasses.hpp
    testMain.cpp
)

target_include_directories(allTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${RAYCE_INCLUDE_DIR}
)

target_compile_definitions(allTests
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
        $<$<CONFIG:Debug>:RAYCE_DEBUG>
        RAYCE_TEST
)

enable_testing()

target_link_libraries(allTests
    gtest_main
    gmock_main
    rayce::core
    rayce::vulkan
    rayce::app
)

include(GoogleTest)
gtest_discover_tests(allTests)