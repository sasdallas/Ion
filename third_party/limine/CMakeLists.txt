include(ExternalProject)

set(LIMINE_PREFIX "${CMAKE_BINARY_DIR}/limine")

find_program(MAKE_EXECUTABLE
    NAMES make
    REQUIRED
)

set(LIMINE_ARGUMENTS )
if (ARCH STREQUAL "x86_64")
    set(LIMINE_ARGUMENTS 
            --enable-bios
            --enable-bios-cd 
            --enable-uefi-x86-64
            --enable-uefi-cd
    )
endif()

ExternalProject_Add(limine
    GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
    GIT_TAG v12.x
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    PREFIX "${CMAKE_BINARY_DIR}/_limine_src"
    BUILD_IN_SOURCE FALSE

    CONFIGURE_COMMAND
        <SOURCE_DIR>/bootstrap
        COMMAND <SOURCE_DIR>/configure
            --prefix=${LIMINE_PREFIX}
            ${LIMINE_ARGUMENTS}

    BUILD_COMMAND "${MAKE_EXECUTABLE}"
    INSTALL_COMMAND "${MAKE_EXECUTABLE}" install
    BUILD_BYPRODUCTS "${LIMINE_PREFIX}/bin/limine"
)

add_iso_file(
    "${LIMINE_PREFIX}/share/limine/limine-uefi-cd.bin"
    DESTINATION "boot/limine/limine-uefi-cd.bin"
    DEPENDS limine
)

if (ARCH STREQUAL "x86_64")
    add_iso_file(
        "${LIMINE_PREFIX}/share/limine/limine-bios.sys"
        DESTINATION "boot/limine/limine-bios.sys"
        DEPENDS limine
    )

    add_iso_file(
        "${LIMINE_PREFIX}/share/limine/limine-bios-cd.bin"
        DESTINATION "boot/limine/limine-bios-cd.bin"
        DEPENDS limine
    )

    add_iso_file(
        "${LIMINE_PREFIX}/share/limine/BOOTX64.EFI"
        DESTINATION "EFI/BOOT/BOOTX64.EFI"
        DEPENDS limine
    )
elseif (ARCH STREQUAL "riscv64")
    add_iso_file(
        "${LIMINE_PREFIX}/share/limine/BOOTRISCV64.EFI"
        DESTINATION "EFI/BOOT/BOOTRISCV64.EFI"
        DEPENDS limine
    )
endif()

