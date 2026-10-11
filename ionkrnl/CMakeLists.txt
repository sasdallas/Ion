project(ionkrnl)

set(__KRNL_SOURCE)
include(sources.cmake)

add_executable(ionkrnl ${__KRNL_SOURCE})

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/${ARCH}.ld")
if (NOT EXISTS "${LINKER_SCRIPT}")
    message(FATAL_ERROR "Missing kernel linker script for architecture ${ARCH}")
endif()

set_property(TARGET ionkrnl APPEND PROPERTY LINK_DEPENDS "${LINKER_SCRIPT}")
target_link_options(ionkrnl PRIVATE "-Wl,-T,${LINKER_SCRIPT}")

set_target_properties(
    ionkrnl
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL ""
)

target_link_options(ionkrnl
    PRIVATE  -Wl,--exclude-all-symbols,--file-alignment=0x1000,-section-alignment=0x1000)

target_link_options(ionkrnl PRIVATE "-Wl,--entry,KeSystemStartup")
target_link_libraries(ionkrnl ke_rtl)

add_iso_file(ionkrnl DESTINATION "boot/ionkrnl.exe")
