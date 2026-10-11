/**
 * @file include/ion/x86_64/port.h
 * @brief x86_64 port macros
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_X86_64_PORT_H
#define _ION_X86_64_PORT_H 1

static inline unsigned char PortRead8(unsigned short Port) {
    unsigned char r;
    asm volatile("inb %1, %0" : "=a"(r) : "dN"(Port));
    return r;
}

static inline unsigned short PortRead16(unsigned short Port) {
    unsigned short r;
    asm volatile("inw %1, %0" : "=a"(r) : "dN"(Port));
    return r;
}

static inline unsigned long PortRead32(unsigned short Port) {
    unsigned short r;
    asm volatile("inl %1, %0" : "=a"(r) : "dN"(Port));
    return r;
}

static inline void PortWrite8(unsigned short Port, unsigned char Value) {
    asm volatile ("outb %0, %1" :: "a"((unsigned char)(Value)), "Nd"((unsigned short)(Port)));
}

static inline void PortWrite16(unsigned short Port, unsigned short Value) {
    asm volatile ("outw %0, %1" :: "a"((unsigned short)(Value)), "Nd"((unsigned short)(Port)));
}

static inline void PortWrite32(unsigned short Port, unsigned int Value) {
    asm volatile ("outl %0, %1" :: "a"((unsigned int)(Value)), "Nd"((unsigned short)(Port)));
}

#endif
