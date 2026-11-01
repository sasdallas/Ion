# HAL CMake file
project(hal)

include(../../third_party/uACPI/uacpi.cmake)

set(__HAL_SOURCES)
include(sources.cmake)

add_library(hal SHARED ${__HAL_SOURCES})
set_target_properties(hal PROPERTIES PREFIX "")
set_target_properties(hal PROPERTIES OUTPUT_NAME "hal")
target_compile_options(hal PRIVATE -fno-omit-frame-pointer)
target_compile_definitions(hal PRIVATE HAL)
target_include_directories(hal PRIVATE ${CMAKE_SOURCE_DIR}/third_party/flanterm/src)
target_include_directories(hal PRIVATE ${UACPI_INCLUDES})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(hal
        PRIVATE  -Wl,--export-all-symbols,--file-alignment=0x1000,-section-alignment=0x1000)
else()
    target_link_options(hal
        PRIVATE  -Wl,--exclude-all-symbols,--file-alignment=0x1000,-section-alignment=0x1000)
endif()

target_link_libraries(hal ke_rtl ionkrnl)

add_iso_file(hal DESTINATION "boot/hal.dll")
