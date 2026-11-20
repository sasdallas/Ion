list(APPEND __KE_SOURCE
                    ke/lock.c
                    ke/init.c
                    ke/bugcheck.c
                    ke/trap.c
                    ke/irql.c
                    ke/semaphore.c
                    ke/dpc.c
                    ke/wait.c
)

if (ARCH STREQUAL "x86_64")
    list(APPEND __KE_SOURCE
                    ke/x86_64/init.c
                    ke/x86_64/dbg.c
                    ke/x86_64/cpu.c
                    ke/x86_64/trap.S
    )
else()
    message(FATAL_ERROR "Unsupported kernel architecture ${ARCH}")
endif()

list(APPEND __MM_SOURCE
                    mm/pfndb.c
                    mm/vmmgr.c
                    mm/iospace.c
                    mm/pool.c
                    mm/heap.c
)


if (ARCH STREQUAL "x86_64")
    list(APPEND __MM_SOURCE
                    mm/x86_64/page.c
    )
else()
    message(FATAL_ERROR "Unsupported kernel architecture ${ARCH}")
endif()

list(APPEND __LDR_SOURCE
                    ldr/ldr.c
)

list(APPEND __HAL_SOURCE
                    hal/hal.c
)

list(APPEND __OB_SOURCE
                    ob/create.c
                    ob/type.c
                    ob/reference.c
)

list(APPEND __PS_SOURCE
                    ps/init.c
                    ps/sched.c
                    ps/thread.c
                    ps/process.c
)

if (ARCH STREQUAL "x86_64")
    list(APPEND __PS_SOURCE 
                    ps/x86_64/switch.S
    )
endif()

list(APPEND __KRNL_SOURCE ${__KE_SOURCE} ${__MM_SOURCE} ${__LDR_SOURCE} ${__HAL_SOURCE} ${__OB_SOURCE} ${__PS_SOURCE})
