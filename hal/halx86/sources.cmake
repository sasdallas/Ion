list(APPEND __HAL_SOURCES
    hal/init.c
    hal/console.c
    hal/font.c
    hal/acpi_glue.c
    hal/acpi.c
    hal/apic.c
    hal/clock.c
    ../../third_party/flanterm/src/flanterm.c
    ../../third_party/flanterm/src/flanterm_backends/fb.c
    ${UACPI_SOURCES}
)

