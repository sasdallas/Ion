cmake_minimum_required(VERSION 3.15)

project(
    Ion VERSION 1.0
    DESCRIPTION "An operating system"
    LANGUAGES C ASM
)

if (NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "Missing toolchain file, it is in cmake/toolchain/x86_64.cmake")
endif()

set(ARCH "x86_64" CACHE STRING "Architecture target")

if (NOT ARCH STREQUAL "x86_64")
    message(FATAL_ERROR "death")
endif()

if (ARCH STREQUAL "x86_64")
    add_definitions(-D_ION64)
endif()

# Helpers
include(cmake/iso.cmake)

# C options
include_directories(${CMAKE_SOURCE_DIR}/include/)
add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
add_compile_options("-fno-builtin-memset")

# Directories
add_subdirectory("third_party")
add_subdirectory("rtl")
add_subdirectory("ionkrnl")
add_subdirectory("hal")

add_iso_file(${CMAKE_SOURCE_DIR}/limine.conf DESTINATION "boot/limine.conf")

create_iso()
