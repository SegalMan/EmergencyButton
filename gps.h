//
// Created by Segal on 08/04/2019.
//

#ifndef PART_1_GPS_H
#define PART_1_GPS_H

#include <stdbool.h>
#include "gps_constants.h"

/**
 * Struct used to describe GPS coordinates.
 */
typedef struct  _GPS_LOCATION_INFO
{
    __int32_t latitude;
    __int32_t longitude;
    __int32_t altitude;
    __uint8_t hdop;
    __uint8_t valid_fix : 1;
    __uint8_t reserved1 : 2;
    __uint8_t num_sats : 4;
    char fixtime[11];
} GPS_LOCATION_INFO;

/**
 * Initializes serial connection with gps device with port and baud rate
 * as defined in gps_constants.h
 */
void GPSInit(void);

/**
 * Reads raw data from GPS device
 * @param buf - buffer to fill with data from GPS device.
 * @param maxlen - maximum amount of bytes to read
 * @return amount of bytes read
 */
__uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen);

/**
 * Parses data from GPS device and fills location struct with fix coordinates
 * @param location - pointer of struct to contain fix coordinates
 * @return true if fix information was available, false otherwise
 */
bool GPSGetFixInformation (GPS_LOCATION_INFO *location);

/**
 * Disables GPS device, and closes serial communication with it.
 */
void GPSDisable(void);

#endif //PART_1_GPS_H
