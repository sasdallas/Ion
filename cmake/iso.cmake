# helper ISO manipulation macros

set(ISO_ROOT "${CMAKE_BINARY_DIR}/iso")
set(ISO_OUTPUT "${CMAKE_BINARY_DIR}/ion.iso")

# add_iso_file adds a file to the ISO
# it will create a new copy target that will be called later on
function(add_iso_file SOURCE)
    cmake_parse_arguments(ARG
        ""
        "DESTINATION"
        "DEPENDS"
        ${ARGN}
    )

    if(NOT ARG_DESTINATION)
        message(FATAL_ERROR "add_iso_file requires DESTINATION")
    endif()

    if(TARGET "${SOURCE}")
        set(INPUT_FILE "$<TARGET_FILE:${SOURCE}>")
        set(DEPENDENCY "${SOURCE}")
    else()
        get_filename_component(INPUT_FILE "${SOURCE}" ABSOLUTE)

        if(ARG_DEPENDS)
            set(DEPENDENCY "")
        else()
            set(DEPENDENCY "${INPUT_FILE}")
        endif()
    endif()

    set_property(GLOBAL APPEND PROPERTY ISO_INPUTS "${INPUT_FILE}")
    set_property(GLOBAL APPEND PROPERTY ISO_OUTPUTS
        "${ISO_ROOT}/${ARG_DESTINATION}"
    )
    set_property(GLOBAL APPEND PROPERTY ISO_FILE_DEPENDS "${DEPENDENCY}")
    set_property(GLOBAL APPEND PROPERTY ISO_EXTRA_DEPENDS "${ARG_DEPENDS}")
endfunction()

function(create_iso)
    get_property(INPUTS GLOBAL PROPERTY ISO_INPUTS)
    get_property(OUTPUTS GLOBAL PROPERTY ISO_OUTPUTS)
    get_property(FILE_DEPS GLOBAL PROPERTY ISO_FILE_DEPENDS)
    get_property(EXTRA_DEPS GLOBAL PROPERTY ISO_EXTRA_DEPENDS)

    message(STATUS "${INPUTS} ${OUTPUTS} ${FILE_DEPS} ${EXTRA_DEPS}")

    set(STAGED_FILES)

    list(LENGTH INPUTS COUNT)

    if(COUNT GREATER 0)
        math(EXPR last "${COUNT} - 1")

        foreach(i RANGE 0 ${last})
            list(GET INPUTS ${i} input)
            list(GET OUTPUTS ${i} output)

            get_filename_component(OUTPUT_DIR "${output}" DIRECTORY)

            add_custom_command(
                OUTPUT "${output}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${input}" "${output}"
                DEPENDS "${input}"
                COMMENT "Staging ${output}"
                VERBATIM
            )

            list(APPEND STAGED_FILES "${output}")
        endforeach()
    endif()

    message(STATUS "${STAGED_FILES}")

    find_program(XORRISO xorriso REQUIRED)

    add_custom_command(
        OUTPUT "${ISO_OUTPUT}"
        COMMAND "${XORRISO}" -as mkisofs
            -R -r -J
            -o "${ISO_OUTPUT}"
            "${ISO_ROOT}"
            -b boot/limine-bios-cd.bin
            -no-emul-boot
            -boot-load-size 4
            -boot-info-table
            -hfsplus
            -apm-block-size 2048
            --efi-boot boot/limine-uefi-cd.bin
            -efi-boot-part
            --efi-boot-image
            --protective-msdos-label
        DEPENDS ${STAGED_FILES} ${EXTRA_DEPS}
        COMMENT "Creating ISO"
        VERBATIM
    )

    add_custom_target(iso ALL
        DEPENDS "${ISO_OUTPUT}"
    )
endfunction()
