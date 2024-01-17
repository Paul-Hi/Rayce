# FindSlang.cmake

# Usage:
# find_package(Slang)

set(SLANG_VERSION "2023.5.5") # Hardcoded

# Download
if(WIN32)
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-win64.zip")
else()
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-linux-x86_64.zip")
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    set(OPTIONS DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
else()
    set(OPTIONS)
endif()

FetchContent_Declare(slangbuild
    URL ${SLANG_URL}
    ${OPTIONS}
)

message(STATUS "Looking for Slang ${SLANG_VERSION}")

if(NOT DEFINED SLANGC)
    message(STATUS "Downloading Slang ${SLANG_VERSION}")
    FetchContent_Populate(slangbuild)
    message(STATUS "Downloaded to ${slangbuild_SOURCE_DIR}")

    if(slangbuild_SOURCE_DIR)
        find_program(SLANGC
            NAMES slangc
            PATHS ${slangbuild_SOURCE_DIR}/bin/windows-x64/release ${slangbuild_SOURCE_DIR}/bin/linux-x64/release NO_DEFAULT_PATH
        )

        find_library(SLANG_LIB
            NAMES slang
            PATHS ${slangbuild_SOURCE_DIR}/bin/windows-x64/release ${slangbuild_SOURCE_DIR}/bin/linux-x64/release NO_DEFAULT_PATH
        )
    endif()

    if(NOT SLANGC)
        message(FATAL_ERROR "SlangC not found")
    endif()

    set(SLANGC ${SLANGC} CACHE FILEPATH "Path to SlangC")
    set(SLANG_INCLUDE_DIRECTORIES ${slangbuild_SOURCE_DIR} CACHE PATH "Path to Slang Include Headers")
endif()

message(STATUS "Using ${SLANGC}")
execute_process(COMMAND ${SLANGC} "-v" ERROR_VARIABLE SLANG_VERSION RESULTS_VARIABLE EXTRACT_RESULT)
message(STATUS "SLANGC version is ${SLANG_VERSION}")

if(NOT EXTRACT_RESULT EQUAL 0)
    message("Envoking Slang compiler failed with: ${EXTRACT_RESULT}")
endif()
