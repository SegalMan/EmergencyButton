/*
 * serial_io_geckostk3402a.c
 *
 *  Created on: 08/04/2019
 *      Author: Segal
 */

#include <stdio.h>
#include <stdlib.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_ldma.h"
#include "bspconfig.h"
#include "retargetserial.h"
#include "serial_io_geckostk3402a_usart.h"
#include "em_usart.h"

bool SerialInit_USART(char *port, unsigned int baud)
{

	// Parses ports
	int usart_port = strtol(port, (char **)NULL, 10);

	// Enable peripheral clocks
	CMU_ClockEnable(cmuClock_HFPER, true);

	CMU_ClockEnable(cmuClock_USART2, true);

	// Configure GPIO pins
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Create Default USART_ASYNC init struct
	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
	init.baudrate = baud;

	USART_InitAsync(USART2, &init);

	USART2->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
	USART2->ROUTELOC0 = (USART2->ROUTELOC0 & ~(_USART_ROUTELOC0_TXLOC_MASK
				| _USART_ROUTELOC0_RXLOC_MASK))
				| (_USART_ROUTELOC0_TXLOC_LOC1 << _USART_ROUTELOC0_TXLOC_SHIFT)
				| (_USART_ROUTELOC0_RXLOC_LOC1 << _USART_ROUTELOC0_RXLOC_SHIFT);

	/* Enable I/O and set location */
	GPIO_PinModeSet(usart_port, 7, gpioModeInput,0);
	GPIO_PinModeSet(usart_port, 6, gpioModePushPull,1);

	RETARGET_SerialCrLf(1);

	return true;
}

unsigned int SerialRecv_USART(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms)
{
	uint32_t currTicks = msTicks;

	int c = 0;
	unsigned int i = 0;
	while ((i < maxlen) && ((msTicks - currTicks) < timeout_ms))
	{
		if ((USART2 -> STATUS) & (USART_STATUS_RXDATAV))
		{
			c =  (uint8_t)USART2 -> RXDATA;
			if (c > 0)
			{
				buf[i] = c;
				i++;
			}
		}
	}

	return i;
}

bool SerialSend_USART(unsigned char *buf, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
	{
		USART_Tx(USART2, buf[i]);
	}
	return true;
}

void SerialFlushInputBuff_USART(void)
{
	// Clears LEUART0 interrupts
    USART_IntClear(USART2, USART_IntGet(USART2));
}

void SerialDisable_USART()
{
	// Disable USART and all needed clocks.
	USART_Enable(USART2, usartDisable);
//	CMU_ClockEnable(cmuClock_GPIO, false);
	CMU_ClockEnable(cmuClock_USART2, false);
//	CMU_ClockEnable(cmuClock_HFPER, false);
}
