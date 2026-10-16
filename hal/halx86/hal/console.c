/**
 * @file hal/halx86/hal/console.c
 * @brief Framebuffer system console
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ion.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>

extern const UCHAR Halx86VgaFont[256][16];

static struct flanterm_context *Halx86Console = NULL;

VOID Halx86PrintString(const CHAR *String) {
    while (*String) {
        flanterm_write(Halx86Console, String, 1);

        if (*String == '\n') {
            flanterm_write(Halx86Console, (char[]){ '\r' }, 1);
        }

        String++;
    }
}

OS_STATUS Halx86InitSystemConsole() {
    LOADER_PARAMETER_BLOCK *Lpb = KeGetLoaderParameterBlock();
    UINT32 Background = 0x0000a8;
    UINT32 Foreground = 0xaaaaaa;

    LOADER_FRAMEBUFFER *Framebuffer = &Lpb->Framebuffers[0];
    if (Framebuffer->Bpp != 32) {
        return STATUS_INVALID_PARAMETER;
    }

    Halx86Console = flanterm_fb_init(
        NULL, NULL,
        (UINT32*)Framebuffer->Address, Framebuffer->Width, Framebuffer->Height, Framebuffer->Pitch,
        Framebuffer->RedMaskSize, Framebuffer->RedMaskShift,
        Framebuffer->GreenMaskSize, Framebuffer->GreenMaskShift,
        Framebuffer->BlueMaskSize, Framebuffer->BlueMaskShift,
        NULL, NULL, NULL,
        &Background, &Foreground, NULL, NULL,
        (VOID*)Halx86VgaFont, 8, 16, 0,
        1, 1, 0, FLANTERM_FB_ROTATE_0, TRUE
    );
    
    return Halx86Console ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
}
