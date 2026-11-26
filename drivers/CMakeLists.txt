add_custom_target(drivers)

function(add_driver)
    cmake_parse_arguments(OPT "" "DRIVER_NAME" "SOURCES" ${ARGN})
    set(TARGET "${OPT_DRIVER_NAME}")
    add_library("${OPT_DRIVER_NAME}" SHARED ${OPT_SOURCES})

    target_link_options("${OPT_DRIVER_NAME}" PRIVATE "-Wl,--entry,DriverEntry")

    set_target_properties("${OPT_DRIVER_NAME}" PROPERTIES PREFIX "")
    set_target_properties("${OPT_DRIVER_NAME}" PROPERTIES SUFFIX ".sys")
    set_target_properties("${OPT_DRIVER_NAME}" PROPERTIES OUTPUT_NAME "${OPT_DRIVER_NAME}")
    set_target_properties("${OPT_DRIVER_NAME}" PROPERTIES ENABLE_EXPORTS TRUE DEFINE_SYMBOL "")
    target_link_libraries("${OPT_DRIVER_NAME}" ke_rtl)
    target_link_libraries("${OPT_DRIVER_NAME}" ionkrnl)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options("${OPT_DRIVER_NAME}"
            PRIVATE  -Wl,--export-all-symbols,--file-alignment=0x1000,-section-alignment=0x1000)
    else()
        target_link_options("${OPT_DRIVER_NAME}"
            PRIVATE  -Wl,--exclude-all-symbols,--file-alignment=0x1000,-section-alignment=0x1000)
    endif()

    add_iso_file("${OPT_DRIVER_NAME}" DESTINATION "boot/${OPT_DRIVER_NAME}.sys")
    add_dependencies(drivers "${OPT_DRIVER_NAME}")
endfunction()

include("drivers.cmake")

foreach (DRIVER IN LISTS DRIVER_LIST)
    add_subdirectory("${DRIVER}")
endforeach()

