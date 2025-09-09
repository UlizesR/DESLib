cmake_minimum_required(VERSION 3.10)
project(Dez VERSION 1.0.0 LANGUAGES C)

# =============================================================================
# Project Configuration
# =============================================================================

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# =============================================================================
# Source Files and Libraries
# =============================================================================

# Core source files
set(DEZ_SOURCES
    src/assembly_vm.c
    src/vm_errors.c
    src/vm_hash_table.c
    src/vm_validation.c
)

# Include directories
include_directories(include)

# =============================================================================
# Main Executable
# =============================================================================

add_executable(dez dez.c ${DEZ_SOURCES})

# =============================================================================
# Testing
# =============================================================================

# Enable testing
enable_testing()

# Auto-discover test files
file(GLOB TEST_SOURCES "tests/test_*.c")

# Test executables
set(TEST_TARGETS)
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${DEZ_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE include)
    list(APPEND TEST_TARGETS ${TEST_NAME})
endforeach()

# Auto-configure tests
foreach(TEST_TARGET ${TEST_TARGETS})
    # Extract test category from target name (e.g., test_math -> math)
    string(REGEX REPLACE "^test_" "" TEST_CATEGORY ${TEST_TARGET})
    
    # Add test
    add_test(NAME ${TEST_CATEGORY}_operations 
             COMMAND ${TEST_TARGET} 
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    
    # Set test properties with generic pass regex
    set_tests_properties(${TEST_CATEGORY}_operations PROPERTIES
        PASS_REGULAR_EXPRESSION "All .* tests passed!"
        TIMEOUT 30
    )
endforeach()

# Custom test target
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS ${TEST_TARGETS}
    COMMENT "Running all tests"
)

# =============================================================================
# Installation
# =============================================================================

install(TARGETS dez DESTINATION bin)