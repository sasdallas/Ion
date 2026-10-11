list(APPEND __KE_SOURCE
                    ke/lock.c
                    ke/init.c
                    ke/bugcheck.c
)

if (ARCH STREQUAL "x86_64")
    list(APPEND __KE_SOURCE
                    ke/x86_64/init.c
                    ke/x86_64/dbg.c
    )
else()
    message(FATAL_ERROR "Unsupported kernel architecture ${ARCH}")
endif()

list(APPEND __MM_SOURCE
                    mm/pfndb.c
)


if (ARCH STREQUAL "x86_64")
    list(APPEND __MM_SOURCE
                    mm/x86_64/page.c
    )
else()
    message(FATAL_ERROR "Unsupported kernel architecture ${ARCH}")
endif()

list(APPEND __KRNL_SOURCE ${__KE_SOURCE} ${__MM_SOURCE})
