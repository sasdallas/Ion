# Common library

# As a hack, two versions are compiled: One for the kernel and one for the userspace

set(__RTL_SOURCES
    rtl/print.c
    rtl/dbg.c
    rtl/assert.c
    rtl/memory.c
)

add_library(rtl STATIC ${__RTL_SOURCES})
add_library(ke_rtl STATIC ${__RTL_SOURCES})

target_compile_definitions(ke_rtl PUBLIC KERNEL)
