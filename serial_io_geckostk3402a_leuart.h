/*
 * serial_io.h
 *
 *  Created on: 08/04/2019
 *      Author: Segal
 */

#ifndef SRC_SERIAL_IO_GECKOSTK3402A_LEUART_H_
#define SRC_SERIAL_IO_GECKOSTK3402A_LEUART_H_

#include <stdbool.h>
#include "serial_io_common.h"

bool SerialInit_LEUART(char *port, unsigned int baud);

unsigned int SerialRecv_LEUART(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms);

void SerialFlushInputBuff_LEUART (void);

void SerialDisable_LEUART();

#endif /* SRC_SERIAL_IO_GECKOSTK3402A_LEUART_H_ */
