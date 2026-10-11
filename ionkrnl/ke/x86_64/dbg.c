/**
 * @file ionkrnl/ke/x86_64/dbg.c
 * @brief Debug
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ion.h>
#include <ion/x86_64/port.h>

#define COM_BASE 0x3F8
#define COM_RBR 0x00 // Receive buffer
#define COM_THR 0x00 // Transmit holding 
#define COM_IER 0x01 // Interrupt enable
#define COM_FCR 0x02 // FIFO control
#define COM_LCR 0x03 // Line control
#define COM_MCR 0x04 // Modem control
#define COM_LSR 0x05 // Line status
#define COM_MSR 0x06 // Modem status
#define COM_BAUD_LSB 0x00 // Baud rate
#define COM_BAUD_MSB 0x01 // Baud rate
#define LCR_8D 0x03 // 8 data bits
#define LCR_1S 0x00 // 1 stop bit
#define LCR_NPAR 0x00 // No parity
#define LCR_DLAB 0x80
#define MCR_DTR 0x01 // Data transmit ready
#define MCR_RTS 0x02 // Ready to send
#define MCR_OUT2 0x08 // IRQ pin

static VOID DbgWriteComPort(UINT8 Register, UINT8 Value) {
	PortWrite8(COM_BASE + Register, Value);
}

static UINT8 DbgReadComPort(UINT8 Register) {
	return PortRead8(COM_BASE + Register);
}

static VOID DbgSetBaudRate(UINT16 BaudRate) {
	UINT8 Lcr = DbgReadComPort(COM_LCR);
	DbgWriteComPort(COM_LCR, Lcr | LCR_DLAB);

	UINT32 Mode = 115200 / BaudRate;
	DbgWriteComPort(COM_BAUD_LSB, (UINT8)(Mode & 0xFF));
	DbgWriteComPort(COM_BAUD_MSB, (UINT8)((Mode >> 8) & 0xFF));
	DbgWriteComPort(COM_LCR, Lcr);
}

OS_STATUS DbgInit() {
	/* Disable all interrupts */
	DbgWriteComPort(COM_IER, 0x0);
	
	/* Update baud rate */
	DbgSetBaudRate(115200);

	/* Configure bit parameters */
	DbgWriteComPort(COM_LCR, LCR_8D | LCR_NPAR | LCR_1S);
	
	/* Enable FIFO and clear Tx/Rx */
	DbgWriteComPort(COM_FCR, 0xC7); /* TODO: Bit definitions */
	
	/* Data transmit ready, ready to send, and OUT2 IRQ*/
	DbgWriteComPort(COM_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);

    return STATUS_SUCCESS;
}

static VOID DbgPrintChar(CHAR c) {
	/* Wait for Tx ready */
	while ((DbgReadComPort(COM_LSR) & 0x20) == 0) {
        __builtin_ia32_pause();
    }

	DbgWriteComPort(COM_THR, c);
}

VOID DbgPrintString(const CHAR* String) {
    while (*String) {
		if (*String == '\n') {
            DbgPrintChar('\r');
        }
        
        DbgPrintChar(*String++);
	}
}
