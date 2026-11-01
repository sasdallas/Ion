/**
 * @file hal/halx86/hal/acpi_glue.c
 * @brief x86 uACPI integration
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <uacpi/kernel_api.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

#ifdef __x86_64__
#include <ion/x86_64/port.h>
#else
#error define port.h
#endif

/* Throw exception on bad func */
#define UACPI_UNIMPLEMENTED_FATAL() KeBugCheckWithMessage("uACPI: Unimplemented function \"%s\" called", __FUNCTION__)

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    LOADER_PARAMETER_BLOCK *Lpb = KeGetLoaderParameterBlock();
    if (Lpb->Machine.Rsdp == 0) return UACPI_STATUS_NOT_FOUND;
    *out_rsdp_address = (uacpi_phys_addr)(Lpb->Machine.Rsdp - Lpb->HhdmRegion);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    VOID *Mapping = MmMapIoSpace((UINT_PTR)addr, (SIZE_T)len, MmCacheUncached);
    return Mapping ? Mapping : UACPI_MAP_FAILED;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    MmUnmapIoSpace(addr, (SIZE_T)len);
}

void uacpi_kernel_log(uacpi_log_level lvl, const uacpi_char *str) {
    CHAR *HeaderString = NULL;
    switch (lvl) {
        case UACPI_LOG_DEBUG: HeaderString = "[DEBUG]"; break;
        case UACPI_LOG_TRACE: HeaderString = "[TRACE]"; break;
        case UACPI_LOG_INFO: HeaderString = "[INFO ]"; break;
        case UACPI_LOG_WARN: HeaderString = "[WARN ]"; break;
        case UACPI_LOG_ERROR: HeaderString = "[ERROR]"; break;
        default: HeaderString = "[???  ]"; break;
    }

    DbgPrint("uACPI: %s: %s", HeaderString, str);
}


uacpi_status uacpi_kernel_initialize(uacpi_init_level current_init_lvl) {
    return UACPI_STATUS_OK;
}

void uacpi_kernel_deinitialize(void) {

}


uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

void uacpi_kernel_pci_device_close(uacpi_handle) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_read8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_read16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_read32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_write8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
)
{
    *out_handle = (uacpi_handle)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
}

uacpi_status uacpi_kernel_io_read8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value
) {
    *out_value = PortRead8((unsigned short)handle + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value
) {
    *out_value = PortRead16((unsigned short)handle + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value
) {
    *out_value = PortRead32((unsigned short)handle + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value
) {
    PortWrite8((unsigned short)handle + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value
) {
    PortWrite16((unsigned short)handle + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value
) {
    PortWrite32((unsigned short)handle + offset, in_value);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return MmAllocateHeap(MmNonpagedHeap, (SIZE_T)size);
}

void uacpi_kernel_free(void *mem) {
    MmFreeHeap(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    DbgPrint("uacpi_kernel_get_nanoseconds_since_boot is NYI\n");
    return 0;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    UACPI_UNIMPLEMENTED_FATAL();
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_handle uacpi_kernel_create_semaphore(uacpi_u32 initial_units) {
    KSEMAPHORE *Semaphore = MmAllocateHeap(MmNonpagedHeap, sizeof(KSEMAPHORE));
    assert(Semaphore != NULL);
    KeInitializeSemaphore(Semaphore, initial_units, 0x7FFFFFFF);
    return (uacpi_handle)Semaphore;
}

void uacpi_kernel_free_semaphore(uacpi_handle) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    DbgPrint("uacpi_kernel_get_thread_id is NYI\n");
    return 0;
}

uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
    return (uacpi_interrupt_state)KeSetInterruptState(FALSE);
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
    KeSetInterruptState((BOOLEAN)state);
}

uacpi_status uacpi_kernel_wait_for_semaphore(uacpi_handle Semaphore, uacpi_u16 Timeout) {
    return KeTryAcquireSemaphore((KSEMAPHORE*)Semaphore)
        ? UACPI_STATUS_OK : UACPI_STATUS_TIMEOUT;
}

void uacpi_kernel_signal_semaphore(uacpi_handle Semaphore) {
    KeReleaseSemaphore((KSEMAPHORE*)Semaphore, 0, 1, FALSE);
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*) {
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
)
{
    UACPI_UNIMPLEMENTED_FATAL();
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler, uacpi_handle irq_handle
)
{
    UACPI_UNIMPLEMENTED_FATAL();
}

typedef struct {
    KSPIN_LOCK Lock;
    KIRQL Irql;
} ACPI_SPINLOCK;

uacpi_handle uacpi_kernel_create_spinlock(void) {
    ACPI_SPINLOCK *Lock = MmAllocateHeap(MmNonpagedHeap, sizeof(ACPI_SPINLOCK));
    ASSERT(Lock);
    KeInitializeSpinLock(&Lock->Lock);
    return (uacpi_handle)Lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle Lock) {
    MmFreeHeap((VOID*)Lock);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle Handle) {
    // TODO: This is really bad, Ion expects interrupt state to remain untouched basically unless by internal functions
    ACPI_SPINLOCK *Lock = (ACPI_SPINLOCK*)Handle;
    BOOLEAN InterruptState = KeSetInterruptState(FALSE);
    KeAcquireSpinLock(&Lock->Lock, &Lock->Irql);
    return (uacpi_cpu_flags)InterruptState;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle Handle, uacpi_cpu_flags Cpu) {
    ACPI_SPINLOCK *Lock = (ACPI_SPINLOCK*)Handle;
    KeReleaseSpinLock(&Lock->Lock, Lock->Irql);
    KeSetInterruptState((BOOLEAN)Cpu);
}

uacpi_handle uacpi_kernel_create_work_item(void) {
    // uACPI permits a token when deferred work is executed inline.
    return (uacpi_handle)1;
}

void uacpi_kernel_free_work_item(uacpi_handle handle) {
    (void)handle;
}

void uacpi_kernel_schedule_work(
    uacpi_work_type type, uacpi_handle work_item, uacpi_work_handler handler,
    uacpi_handle ctx
)
{
    (void)type;
    (void)work_item;
    handler(ctx);
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_OK;
}
