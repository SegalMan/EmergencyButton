//
// Created by Segal on 05/04/2019.
//

#include <stdlib.h>
#include <string.h>
#include "gps.h"
#include "serial_io_geckostk3402a_leuart.h"

void GPSInit(void)
{
    if (!SerialInit_LEUART(GPS_PORT, GPS_BAUD))
    {
        printf("FAIL\n");
        exit(1);
    }
}

bool advanceNGSSSentence(char **sentence, int delimiter_char)
{
	*sentence = strchr(*sentence, delimiter_char);

	if (sentence == NULL)
	{
		return false;
	}

	(*sentence)++;
	return true;
}

__uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen)
{
	SerialFlushInputBuff_LEUART();
    __uint32_t bytesRead = SerialRecv_LEUART((unsigned char *)buf, maxlen, TIMEOUT);
    return bytesRead;
}

bool GPSGetFixInformation (GPS_LOCATION_INFO *location)
{
    char information[MAX_BUFFER_LENGTH];
    char *rmc = NULL;
    char *gga = NULL;

    // Attempts to retrieve RMC sentence
	if (!GPSGetReadRaw(information, MAX_BUFFER_LENGTH))
	{
		return false;
	}

	rmc = strstr(information, RMC_PREFIX);
	if (rmc == NULL)
	{
		return false;
	}

	// Copies RMC sentence aside
	strcpy(rmc, rmc);
	if (sizeof(rmc) > 0)
	{
		*(rmc + strlen(rmc)) = 0;
	}

	// Attempts to retrieve GGA sentence
	gga = strstr(rmc, GGA_PREFIX);

	if (gga == NULL)
	{
		if (!GPSGetReadRaw(information, MAX_BUFFER_LENGTH))
		{
			return false;
		}
		gga = strstr(information, GGA_PREFIX);
	}

	if (gga == NULL)
	{
		return false;
	}

    location -> reserved1 = 0;

    char *time = NULL;
    char *date = NULL;

    for (int comma_counter = 1; comma_counter < 10; comma_counter++)
    {
        rmc = strchr(rmc, NMEA_SENTENCE_DELIMITER);

        // Should never happen, means RMC sentence is malformed.
        if (rmc == NULL)
        {
            return false;
        }
        rmc++;

        switch (comma_counter)
        {
        	// Time is located after first comma
        	case RMC_TIME_LOCATION:
				time = rmc;
				break;

			// Date is located after ninth comma.
			case RMC_DATE_LOCATION:
				date = rmc;
				break;
        }
    }

    strncpy(location -> fixtime, date, DATE_LENGTH);
    strncpy((char *)(location -> fixtime) + DATE_LENGTH, time, TIME_LENGTH);
    location -> fixtime[10] = '\0';

    // Adavance into second GGA sentence arguement
    for (int i = 1; i <= 2; i++)
    {
    	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
    	{
    		return false;
    	}
    }

    // Calculate latitude
    double nmea_lat_degrees = 0;
    for (int i = LATITUDE_DEGREES_DIGIT_NUM; i > 0; i--)
    {
    	int power = 1;
    	for (int j = 1; j < i; j++)
    	{
    		power *= 10;
    	}
    	nmea_lat_degrees += ((*(gga + (LATITUDE_DEGREES_DIGIT_NUM - i))) - '0') * power;
    }
	double nmea_lat_seconds = strtod(gga + 2, (char**)NULL);
	location -> latitude = (__int32_t)((nmea_lat_degrees + (nmea_lat_seconds / SECONDS_PER_MINUTE))
			* COORDINATE_SCALING_FACTOR);

	if (!advanceNGSSSentence(&gga, ','))
	{
		return false;
	}

	// Negate result for South coordinates
	if (gga[0] == SOUTH)
	{
		location -> latitude *= (-1);
	}

	// Calculate longitude
	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	double nmea_long_degrees = 0;
	for (int i = LONGITUDE_DEGREES_DIGIT_NUM; i > 0; i--)
	{
		int power = 1;
		for (int j = 1; j < i; j++)
		{
			power *= 10;
		}
		nmea_long_degrees += ((*(gga + (LONGITUDE_DEGREES_DIGIT_NUM - i))) - '0') * power;
	}
	double nmea_long_seconds = (strtod(gga + 3, (char **)NULL));
	location -> longitude = (__int32_t)((nmea_long_degrees + (nmea_long_seconds / SECONDS_PER_MINUTE))
			* COORDINATE_SCALING_FACTOR);

	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	// Negate result for South coordinates
	if (gga[0] == WEST)
	{
		location -> longitude *= (-1);
	}

	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	// Calculate valid fix
	location -> valid_fix = (gga[0] > '0');

	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	// Get # of satellites
	location -> num_sats = (__uint8_t)strtol(gga, (char**)NULL, 10);

	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	// Calculate HDOP
	location -> hdop = (__uint8_t )(strtod(gga, (char **)NULL) * HDOP_SCALING_FACTOR);

	if (!advanceNGSSSentence(&gga, NMEA_SENTENCE_DELIMITER))
	{
		return false;
	}

	// Calculates altitude
	location -> altitude = (__uint32_t)(strtod(gga, (char **)NULL) * ALTITUDE_SCALING_FACTOR);

    return true;
}

void GPSDisable(void)
{
    SerialDisable_LEUART();
}
