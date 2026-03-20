cmake_minimum_required(VERSION 3.20)

project(DES VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DES_BUILD_EXAMPLES "Build example programs" ON)
option(DES_RUN_CLANG_FORMAT "Run clang-format during the default build" ON)
set(DES_BACKEND "Eigen" CACHE STRING "Linear algebra backend: Eigen or Fastor")
set_property(CACHE DES_BACKEND PROPERTY STRINGS Eigen Fastor)

set(DES_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(DES_METHODS_DIR "${DES_INCLUDE_DIR}/Methods")
set(DES_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/external")

function(des_find_eigen out_var)
    set(candidates
        "${PROJECT_SOURCE_DIR}/external/eigen"
        "${PROJECT_SOURCE_DIR}/external/Eigen"
        "${PROJECT_SOURCE_DIR}/external/eigen3"
        "${PROJECT_SOURCE_DIR}/external/eigen/include/eigen3"
        "${PROJECT_SOURCE_DIR}/external/Eigen/include/eigen3"
    )

    foreach(dir IN LISTS candidates)
        if(EXISTS "${dir}/Eigen/Dense")
            set(${out_var} "${dir}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(des_find_fastor out_var)
    set(candidates
        "${PROJECT_SOURCE_DIR}/external"
        "${PROJECT_SOURCE_DIR}/external/Fastor"
        "${PROJECT_SOURCE_DIR}/external/fastor"
    )

    foreach(dir IN LISTS candidates)
        if(EXISTS "${dir}/Fastor/Fastor.h")
            set(${out_var} "${dir}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

add_library(des INTERFACE)
add_library(DES::des ALIAS des)

target_compile_features(des INTERFACE cxx_std_17)
target_include_directories(des INTERFACE
    $<BUILD_INTERFACE:${DES_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

if(DES_BACKEND STREQUAL "Eigen")
    des_find_eigen(DES_EIGEN_INCLUDE_DIR)
    if(DES_EIGEN_INCLUDE_DIR STREQUAL "")
        message(FATAL_ERROR "Eigen headers were not found under ${DES_EXTERNAL_DIR}. Expected a submodule such as external/eigen or external/Eigen.")
    endif()

    target_compile_definitions(des INTERFACE DES_USE_EIGEN)
    target_include_directories(des INTERFACE
        $<BUILD_INTERFACE:${DES_EIGEN_INCLUDE_DIR}>
    )

elseif(DES_BACKEND STREQUAL "Fastor")
    if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        message(FATAL_ERROR
            "Fastor currently pulls in x86-only CPUID/RDTSC code on Apple Silicon. "
            "Use -DDES_BACKEND=Eigen or patch external/Fastor to guard CPUID/RDTSC on non-x86.")
    endif()

    des_find_fastor(DES_FASTOR_INCLUDE_DIR)
    if(DES_FASTOR_INCLUDE_DIR STREQUAL "")
        message(FATAL_ERROR "Fastor headers were not found under ${DES_EXTERNAL_DIR}. Expected external/Fastor or equivalent.")
    endif()

    target_include_directories(des INTERFACE
        $<BUILD_INTERFACE:${DES_FASTOR_INCLUDE_DIR}>
    )

else()
    message(FATAL_ERROR "DES_BACKEND must be either Eigen or Fastor")
endif()

if(DES_BUILD_EXAMPLES AND EXISTS "${PROJECT_SOURCE_DIR}/examples")
    file(GLOB_RECURSE DES_EXAMPLE_SOURCES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/examples/*.cpp"
        "${PROJECT_SOURCE_DIR}/examples/*.cc"
        "${PROJECT_SOURCE_DIR}/examples/*.cxx"
    )

    foreach(example_src ${DES_EXAMPLE_SOURCES})
        get_filename_component(example_name "${example_src}" NAME_WE)

        add_executable(${example_name} "${example_src}")
        target_link_libraries(${example_name} PRIVATE DES::des)
        target_compile_definitions(${example_name}
            PRIVATE DES_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        )
    endforeach()
endif()

find_program(CLANG_FORMAT_BIN
    NAMES clang-format clang-format-20 clang-format-19 clang-format-18 clang-format-17 clang-format-16 clang-format-15
)

set(DES_FORMAT_FILES)

if(EXISTS "${DES_INCLUDE_DIR}")
    file(GLOB_RECURSE DES_HEADER_FILES CONFIGURE_DEPENDS
        "${DES_INCLUDE_DIR}/*.h"
        "${DES_INCLUDE_DIR}/*.hpp"
    )
    list(APPEND DES_FORMAT_FILES ${DES_HEADER_FILES})
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/examples")
    file(GLOB_RECURSE DES_EXAMPLE_FORMAT_FILES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/examples/*.cpp"
        "${PROJECT_SOURCE_DIR}/examples/*.hpp"
    )
    list(APPEND DES_FORMAT_FILES ${DES_EXAMPLE_FORMAT_FILES})
endif()

list(REMOVE_DUPLICATES DES_FORMAT_FILES)

if(CLANG_FORMAT_BIN AND EXISTS "${PROJECT_SOURCE_DIR}/.clang-format" AND DES_FORMAT_FILES)
    set(DES_FORMAT_TARGET_OPTIONS)
    if(DES_RUN_CLANG_FORMAT)
        list(APPEND DES_FORMAT_TARGET_OPTIONS ALL)
    endif()

    add_custom_target(format ${DES_FORMAT_TARGET_OPTIONS}
        COMMAND "${CLANG_FORMAT_BIN}" -i --style=file:${PROJECT_SOURCE_DIR}/.clang-format ${DES_FORMAT_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Formatting C++ sources with clang-format"
        VERBATIM
    )
else()
    message(STATUS "clang-format target disabled: clang-format, .clang-format, or C++ source files were not found.")
endif()