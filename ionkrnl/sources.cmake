list(APPEND __KE_SOURCE
                    ke/lock.c
                    ke/init.c
                    ke/bugcheck.c
                    ke/trap.c
                    ke/irql.c
                    ke/semaphore.c
                    ke/dpc.c
                    ke/wait.c
                    ke/timer.c
                    ke/event.c
                    ke/pushlock.c
)

if (ARCH STREQUAL "x86_64")
    list(APPEND __KE_SOURCE
                    ke/x86_64/init.c
                    ke/x86_64/dbg.c
                    ke/x86_64/cpu.c
                    ke/x86_64/trap.S
                    ke/x86_64/smp.c
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
                    ldr/driver.c
)

list(APPEND __HAL_SOURCE
                    hal/hal.c
)

list(APPEND __OB_SOURCE
                    ob/create.c
                    ob/type.c
                    ob/reference.c
                    ob/init.c
                    ob/handle.c
                    ob/dir.c
                    ob/name.c
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

list(APPEND __EX_SOURCE
                    ex/init.c
                    ex/evntobj.c
)

list(APPEND __IO_SOURCE
                    io/init.c
                    io/driver.c
                    io/bus.c
                    io/device.c
                    io/file.c
                    io/packet.c
)

list(APPEND __KRNL_SOURCE ${__KE_SOURCE} ${__MM_SOURCE} ${__LDR_SOURCE} ${__HAL_SOURCE} ${__OB_SOURCE} ${__PS_SOURCE} ${__EX_SOURCE} ${__IO_SOURCE})
