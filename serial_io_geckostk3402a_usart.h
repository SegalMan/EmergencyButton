/*
 * serial_io.h
 *
 *  Created on: 08/04/2019
 *      Author: Segal
 */

#ifndef SRC_SERIAL_IO_GECKOSTK3402A_USART_H_
#define SRC_SERIAL_IO_GECKOSTK3402A_USART_H_

#include <stdbool.h>
#include "serial_io_common.h"

/**
 * Sends information through serial connection
 * @param buf - buffer to read into
 * @param size - bytes to send
 */
bool SerialSend_USART(unsigned char *buf, unsigned int size);

bool SerialInit_USART(char *port, unsigned int baud);

unsigned int SerialRecv_USART(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms);

void SerialFlushInputBuff_USART(void);

void SerialDisable_USART();

#endif /* SRC_SERIAL_IO_GECKOSTK3402A_USART_H_ */
