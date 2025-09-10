cmake_minimum_required(VERSION 3.10)
project(Dez VERSION 1.0.0 LANGUAGES C)

# =============================================================================
# Configuration
# =============================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# =============================================================================
# Source Files
# =============================================================================

set(DEZ_SOURCES
    src/assembly_vm.c
    src/vm_errors.c
    src/vm_hash_table.c
    src/vm_validation.c
    src/vm_instruction_printer.c
    src/vm_utils.c
)

include_directories(include)

# =============================================================================
# Executables
# =============================================================================

# Main VM
add_executable(dez dez.c ${DEZ_SOURCES})

# Tools
add_executable(dez_asm tools/asm.c ${DEZ_SOURCES})
add_executable(dez_disasm tools/disasm.c ${DEZ_SOURCES})
add_executable(dez_objdump tools/objdump.c ${DEZ_SOURCES})

# =============================================================================
# Testing
# =============================================================================

enable_testing()

file(GLOB TEST_SOURCES "tests/test_*.c")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${DEZ_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE include)
    
    string(REGEX REPLACE "^test_" "" TEST_CATEGORY ${TEST_NAME})
    add_test(NAME ${TEST_CATEGORY}_operations 
             COMMAND ${TEST_NAME} 
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    set_tests_properties(${TEST_CATEGORY}_operations PROPERTIES
        PASS_REGULAR_EXPRESSION "All .* tests passed!"
        TIMEOUT 30
    )
endforeach()

add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    COMMENT "Running all tests"
)

# =============================================================================
# Installation
# =============================================================================

install(TARGETS dez dez_asm dez_disasm dez_objdump DESTINATION bin)