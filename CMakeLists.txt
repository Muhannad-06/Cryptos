cmake_minimum_required(VERSION 3.16)
project(Archive
        VERSION 0.1.0
        DESCRIPTION "Archive file format inspired by ZIP"
        LANGUAGES C)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)          # asprintf() and friends need GNU extensions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Keep binaries together: <build>/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# Core library (everything except src/tools, which contains main())
# ---------------------------------------------------------------------------
set(ARCHIVE_LIB_SOURCES
    src/archive/archive.c
    src/archive/entry.c
    src/archive/field.c
    src/archive/group.c
    src/io/Bin_IO.c
    src/utils/assertion.c
    src/utils/bst.c
    src/utils/error.c
    src/utils/vector.c
)

add_library(archive_core STATIC ${ARCHIVE_LIB_SOURCES})
add_library(Archive::core ALIAS archive_core)

# Sources include headers as "types.h", "archive/archive.h", "utils/bst.h", ...
# so <root>/include is the single include root, exposed to all consumers.
target_include_directories(archive_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_options(archive_core PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

# ---------------------------------------------------------------------------
# Command-line tool
# ---------------------------------------------------------------------------
add_executable(archive_cli src/tools/archive_cli.c)
target_link_libraries(archive_cli PRIVATE archive_core)
target_compile_options(archive_cli PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(ARCHIVE_BUILD_TESTS "Build the test executables" ON)

if(ARCHIVE_BUILD_TESTS)
    include(CTest)

    add_executable(archive_test testing/archive/archive_test.c)
    add_executable(bin_io_test  testing/io/bin_IO_test.c)
    add_executable(bst_test     testing/utils/bst/bst_testing.c)

    # archive_test compares its output against a reference file in its working dir.
    # Drop cmp_archive.arc into testing/archive/ and it is copied automatically.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/testing/archive/cmp_archive.arc)
        configure_file(testing/archive/cmp_archive.arc
                       ${CMAKE_BINARY_DIR}/test_run/archive_test/cmp_archive.arc COPYONLY)
    else()
        message(WARNING "testing/archive/cmp_archive.arc not found: archive_test will fail until it is added")
    endif()

    foreach(t archive_test bin_io_test bst_test)
        target_link_libraries(${t} PRIVATE archive_core)
        target_compile_options(${t} PRIVATE
            $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>)
        # Run from a scratch dir so tests that create files don't litter the source tree.
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_run/${t})
        add_test(NAME ${t}
                 COMMAND $<TARGET_FILE:${t}>
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test_run/${t})
    endforeach()
endif()
