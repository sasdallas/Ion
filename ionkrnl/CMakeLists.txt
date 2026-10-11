project(ionkrnl)

set(__KRNL_SOURCE)
include(sources.cmake)

add_executable(ionkrnl ${__KRNL_SOURCE})

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
