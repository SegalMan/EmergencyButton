//
// Created by Segal on 08/04/2019.
//

#ifndef PART_1_GPS_CONSTANTS_H
#define PART_1_GPS_CONSTANTS_H

#define GPS_PORT "3"
#define GPS_BAUD 9600
#define TIMEOUT 1200
#define MAX_BUFFER_LENGTH 1000
#define RMC_PREFIX "$GNRMC"
#define GGA_PREFIX "$GPGGA"
#define RMC_TIME_LOCATION 1
#define RMC_DATE_LOCATION 9
#define NMEA_SENTENCE_DELIMITER ','
#define DATE_LENGTH 6
#define TIME_LENGTH 4
#define COORDINATE_SCALING_FACTOR 10000000
#define LATITUDE_DEGREES_DIGIT_NUM 2
#define SOUTH 'S'
#define LONGITUDE_DEGREES_DIGIT_NUM 3
#define WEST 'W'
#define ALTITUDE_SCALING_FACTOR 100
#define HDOP_SCALING_FACTOR 5
#define SECONDS_PER_MINUTE 60

#endif //PART_1_GPS_CONSTANTS_H
