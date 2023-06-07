project(raycetests)

add_executable(allTests
    mockClasses.hpp
    testMain.cpp
)

target_include_directories(allTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../rayce/src
)

target_compile_definitions(allTests
    PRIVATE
        $<$<BOOL:${WIN32}>:WIN32>
        $<$<BOOL:${LINUX}>:LINUX>
        $<$<CONFIG:Debug>:RAYCE_DEBUG>
        RAYCE_TEST
)

#set_target_properties(allTests
#    PROPERTIES
#        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#)

target_link_libraries(allTests
    gtest
    gmock
    gtest_main
    rayce
)
