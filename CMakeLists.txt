cmake_minimum_required(VERSION 3.10)
project(
        Cryptos
        VERSION 0.0
        DESCRIPTION "Archive file format"
        LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# 1. FIND THE ZLIB LIBRARY ON LINUX SYSTEM
find_package(ZLIB REQUIRED)

# Gather core library source files (EXCLUDE files in src/tools/ that have a main function)
file(GLOB_RECURSE LIB_SRCS
        "src/archive/*.c"
        "src/io/*.c"
        "src/utils/*.c"
        "src/crypto/*.c"
)

# 1. CREATE THE TARGET BEFORE MODIFYING IT
# This creates a library named "Cryptos" using the sources gathered above
add_library(${PROJECT_NAME} ${LIB_SRCS})

# 2. NOW APPLY INCLUDE DIRECTORIES TO THE TARGET
target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Adding Executables
add_executable(
        ARCHIVE_TEST
        testing/archive/archive_test.c
)
add_executable(
        BIN_IO_TEST
        testing/io/bin_IO_test.c
)
add_executable(
        BST_TEST
        testing/utils/bst/bst_testing.c
)
add_executable(
        ARCHIVE_CLI
        src/tools/archive_cli.c
)

# 3. LINK THE LIBRARY TO YOUR EXECUTABLES
# You cannot link a library to itself. Link the core Cryptos library to each application.
target_link_libraries(ARCHIVE_TEST PRIVATE ${PROJECT_NAME})
target_link_libraries(BIN_IO_TEST PRIVATE ${PROJECT_NAME})
target_link_libraries(BST_TEST PRIVATE ${PROJECT_NAME})
target_link_libraries(ARCHIVE_CLI PRIVATE ${PROJECT_NAME})
