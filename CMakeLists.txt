cmake_minimum_required(VERSION 3.10)
project(Dez VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -O0 -DDEBUG_MEMORY_STATS")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto -march=native")

# Include directories
include_directories(src)

# Create bin directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Automatically find all source files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.c")
file(GLOB_RECURSE TEST_SOURCES "tests/*.c")
file(GLOB_RECURSE TOOL_SOURCES "tools/*.c")
file(GLOB_RECURSE ASSEMBLER_SOURCES "src/assembler/*.c")

# VM Core Library
add_library(dez_vm_core ${CORE_SOURCES} ${ASSEMBLER_SOURCES})

# Test executables
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_link_libraries(${test_name} dez_vm_core)
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Tool executables
foreach(tool_file ${TOOL_SOURCES})
    get_filename_component(tool_name ${tool_file} NAME_WE)
    add_executable(${tool_name} ${tool_file})
    target_link_libraries(${tool_name} dez_vm_core)
    set_target_properties(${tool_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Assembler executable (only the main asm.c file)
add_executable(asm src/assembler/asm.c)
target_link_libraries(asm dez_vm_core)
set_target_properties(asm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Main VM executable
add_executable(dez_vm src/main.c)
target_link_libraries(dez_vm dez_vm_core)
set_target_properties(dez_vm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Installation
install(TARGETS dez_vm asm disasm_tool test_core test_strings test_memory test_performance test_assembly
    RUNTIME DESTINATION bin
)

# Install headers
file(GLOB_RECURSE HEADER_FILES "src/*.h")
install(FILES ${HEADER_FILES}
    DESTINATION include
)

# Enable CTest
enable_testing()

# Add tests
add_test(NAME test_core COMMAND test_core)
add_test(NAME test_strings COMMAND test_strings)
add_test(NAME test_memory COMMAND test_memory)
add_test(NAME test_performance COMMAND test_performance)
add_test(NAME test_assembly COMMAND test_assembly)