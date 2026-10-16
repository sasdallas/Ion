# HAL CMake file
project(hal)

set(__HAL_SOURCES)
include(sources.cmake)

add_library(hal SHARED ${__HAL_SOURCES})
target_compile_definitions(hal PRIVATE HAL)
target_include_directories(hal PRIVATE ${CMAKE_SOURCE_DIR}/third_party/flanterm/src)

target_link_libraries(hal ke_rtl ionkrnl)

add_iso_file(hal DESTINATION "boot/hal.dll")
