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
#include "serial_io_geckostk3402a_leuart.h"
#include "em_leuart.h"

#define LEUART0_RXPIN 11

// We assume that the MCU is capable of reading 3 bytes per ms
#define ITERATIONS_PER_MS 3

bool SerialInit_LEUART(char *port, unsigned int baud)
{
	// Enable peripheral clocks
	CMU_ClockEnable(cmuClock_HFPER, true);

	// Configure GPIO pins
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Parses ports
	int rx_port = strtol(port, (char **)NULL, 10);
	if (!rx_port)
	{
		return false;
	}

	//  GPIO_PinModeSet(RETARGET_TXPORT, RETARGET_TXPIN, gpioModePushPull, 1);
	GPIO_PinModeSet(rx_port, LEUART0_RXPIN, gpioModeInput, 0);

	// Create Default LEUART_Init struct
	LEUART_Init_TypeDef init = LEUART_INIT_DEFAULT;
	init.baudrate = baud;

	// Enable CORE LE clock in order to access LE modules
	CMU_ClockEnable(cmuClock_CORELE, true);

	// Select LFXO for LEUARTs (and wait for it to stabilize)
	CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
	CMU_ClockEnable(cmuClock_LEUART0, true);

	// Do not prescale clock
	CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1);

	// Configure LEUART
	init.enable = leuartDisable;

	LEUART_Init(LEUART0, &init);

	// Enable pins at default location
	LEUART0->ROUTELOC0 = (LEUART0->ROUTELOC0 & ~_LEUART_ROUTELOC0_RXLOC_MASK)
	                     | (_LEUART_ROUTELOC0_RXLOC_LOC18 << _LEUART_ROUTELOC0_RXLOC_SHIFT);

	LEUART0->ROUTEPEN  = LEUART_ROUTEPEN_RXPEN;

	/* Finally enable it */
	LEUART_Enable(LEUART0, leuartEnableRx);

	return true;
}

unsigned int SerialRecv_LEUART(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms)
{
	uint32_t currTicks = msTicks;

	int c = 0;
	unsigned int i = 0;
	while ((i < maxlen) && ((msTicks - currTicks) < timeout_ms))
	{
		if ((LEUART0 -> STATUS) & (LEUART_STATUS_RXDATAV))
		{
			c = (uint8_t)LEUART0->RXDATA;
			if (c > 0)
			{
				buf[i] = c;
				i++;
			}
		}
	}

	return i;
//	int c = 0;
//	unsigned int i = 0, j = 0;
//	while ((i < maxlen) && (j < (ITERATIONS_PER_MS * timeout_ms)))
//	{
//		c = LEUART_Rx(LEUART0);
//
//		if (c > 0)
//		{
//			*(buf + i) = c;
//			i++;
//		}
//		j++;
//	}
//
//	return i;
}

void SerialFlushInputBuff_LEUART (void)
{
	// Clears LEUART0 interrupts
    LEUART_IntClear(LEUART0, LEUART_IntGet(LEUART0));
}

void SerialDisable_LEUART()
{
	// Disable UART and all needed clocks.
	LEUART_Enable(LEUART0, leuartDisable);
	CMU_ClockEnable(cmuClock_LEUART0, false);
//	CMU_ClockEnable(cmuClock_CORELE, false);
//	CMU_ClockEnable(cmuClock_GPIO, false);
//	CMU_ClockEnable(cmuClock_HFPER, false);
}
