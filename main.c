/***************************************************************************//**
 * @file
 * @brief Emergency button runner function
 ******************************************************************************/


#include "em_cmu.h"
#include "em_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bsp.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_usart.h"
#include "display.h"
#include "textdisplay.h"
#include "retargettextdisplay.h"
#include "cellular.h"
#include "gps.h"
#include "serial_io_common.h"
#include "capsense.h"
#include <math.h>

// General constants
#define INFLUX_SECTION_SEPARATOR " "
#define FIELD_SEPARATOR ","
#define GPS_PAYLOAD_PREFIX "location,ICCID="
#define URL "http://REDACTED"
#define WRITE_PATH "/write?db=REDACTED"
#define QUERY_PATH "/query?db=REDACTED"
#define Q_PREFIX "&q=SELECT%20LAST%28frequency%29%20FROM%20report_frequency%20WHERE%20ICCID%3D%27"
#define Q_TIMESTAMP_CONDITION "%27%20AND%20time%20%3E%20"
#define Q_SUFFIX "&epoch=ns"
#define SERVER_EMPTY_RESPONSE "{\"results\":[{\"statement_id\":0}]}"
#define SERVER_FREQUENCY_FORMAT "{\"results\":[{\"statement_id\":0,\"series\":[{\"name\":\"report_frequency\",\"columns\":[\"time\",\"last\"],\"values\":[[%*ld,%d]]}]}]}"
#define PAYLOAD_MAX_LEN 256 // Confirmed by forum
#define SINGLE_FIELD_MAX_LEN 50
#define EMPTY_STRING "\0"
#define FIXTIME_DAY_INDEX 0
#define FIXTIME_DAY_LENGTH 2
#define FIXTIME_MONTH_INDEX 2
#define FIXTIME_MONTH_LENGTH 2
#define FIXTIME_YEAR_INDEX 4
#define FIXTIME_YEAR_LENGTH 2
#define FIXTIME_HOUR_INDEX 6
#define FIXTIME_HOUR_LENGTH 2
#define FIXTIME_MINUTE_INDEX 8
#define FIXTIME_MINUTE_LENGTH 2
#define DEFAULT_RESPONSE_LENGTH 100
#define TIMESTAMP_LEN 20
#define MAX_SCREEN_LENGTH 21
#define CONNECTION_RETRY_MS 30000
#define CAPSENSE_UPDATE_INTERVAL_MS 5
#define FIXTIME_STRING "%s/%s/%s %s:%s UTC\n"

// Global variables
uint32_t g_capsense_update_timestamp = 0;
uint32_t g_gps_update_timestamp = 0;
uint32_t g_frequency_intervals_ms[5] = {300000, 900000, 3600000, 10800000, 43200000};
char *g_frequency_interval_strings[5] = {"5m", "15m", "1h", "3h", "12h"};
int g_report_frequency_index = 2;
uint32_t g_report_frequency_ms = 3600000;
bool g_apply_frequency_change = false;
bool g_is_emergency = false;
uint32_t prev_report_ms;
double g_read_timestamp = 0;


/*******************************************************************************
 * @brief Event handler for Systick - Updates count for active wait, slider
 * pressed and read timer
 ******************************************************************************/
void SysTick_Handler(void)
{
	++msTicks;
}

void sleep(int msecs)
{
	uint32_t currTicks = msTicks;
	while (msTicks - currTicks < msecs);
	return;
}

/*******************************************************************************
 * @brief Event handler for BTN0 (does nothing for any other even pin) -
 * Sends emergency report
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
	// Read and clear GPIO input.
	uint32_t gpioInt = GPIO -> IFC;

	// Use masking to check pin 6 (BTN0) was pressed.
	if (gpioInt & (1 << BSP_GPIO_PB0_PIN))
	{
		g_is_emergency = true;
	}
}

/*******************************************************************************
 * @brief Event handler for BTN1 (does nothing for any other even pin) -
 * Applies changes to device frequency
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
	// Read and clear GPIO input.
	uint32_t gpioInt = GPIO -> IFC;

	// Use masking to check pin 7 (BTN1) was pressed.
	if (gpioInt & (1 << BSP_GPIO_PB1_PIN))
	{
		g_apply_frequency_change = true;
	}
}

/*******************************************************************************
 * @brief Converts fixtime in ddmmyyhhss format to unix timestamp in nanoseconds
 * as string
 * fixtime - time string in ddmmyyhhss format
 * timestamp - empty string to fill
 ******************************************************************************/
double fixtimeToTimestamp(char *fixtime)
{
	if (strcmp(fixtime, EMPTY_STRING) == 0)
	{
		return 0;
	}

	// Convert fixtime to timestamp
	char day[2], month[2], year[2], hour[2], minute[2];

	strncpy((char *) day,
			(char *) fixtime + FIXTIME_DAY_INDEX,
			FIXTIME_DAY_LENGTH);
	day[2] = '\0';
	strncpy((char *) month,
			(char *) fixtime + FIXTIME_MONTH_INDEX,
			FIXTIME_MONTH_LENGTH);
	month[2] = '\0';
	strncpy((char *) year,
			(char *) fixtime + FIXTIME_YEAR_INDEX,
			FIXTIME_YEAR_LENGTH);
	year[2] = '\0';
	strncpy((char *) hour,
			(char *) fixtime + FIXTIME_HOUR_INDEX,
			FIXTIME_HOUR_LENGTH);
	hour[2] = '\0';
	strncpy((char *) minute,
			(char *) fixtime + FIXTIME_MINUTE_INDEX,
			FIXTIME_MINUTE_LENGTH);
	minute[2] = '\0';

	struct tm timeStructure;
	timeStructure.tm_mday = strtol((char *)day, NULL, 10);
	timeStructure.tm_mon = strtol((char *)month, NULL, 10) - 1;
	timeStructure.tm_year = strtol((char *)year, NULL, 10) + 100;
	timeStructure.tm_hour = strtol((char *)hour, NULL, 10);
	timeStructure.tm_min = strtol((char *)minute, NULL, 10);
	timeStructure.tm_sec = 0;
	timeStructure.tm_isdst = -1;
	timeStructure.tm_wday = 0;
	timeStructure.tm_yday = 0;

	return (double)mktime(&timeStructure) * 10e8;
}

/*******************************************************************************
 * @brief build GPS payload
 * info - location info struct
 * ccid - CCID of cellular modem
 * emergency - is report an emergency report
 * NOTE: Does not validate payload length
 ******************************************************************************/
double buildGPSPayload(GPS_LOCATION_INFO info, char *ccid, char *payload)
{

	// Insert tags into payload
	strcpy(payload, GPS_PAYLOAD_PREFIX);
	strcat(payload, ccid);
	if (g_is_emergency)
	{
		strcat(payload, ",emergency=true");
	}
	strcat(payload, INFLUX_SECTION_SEPARATOR);

	// Construct GPS payload fields
	char latitudeField[SINGLE_FIELD_MAX_LEN];
	sprintf(latitudeField, "latitude=%f,", info.latitude * 1.0 / COORDINATE_SCALING_FACTOR);
	strcat(payload, latitudeField);

	char longitudeField[SINGLE_FIELD_MAX_LEN];
	sprintf(longitudeField, "longitude=%f,", info.longitude * 1.0 / COORDINATE_SCALING_FACTOR);
	strcat(payload, longitudeField);

	char altitudeField[SINGLE_FIELD_MAX_LEN];
	sprintf(altitudeField, "altitude=%f,", info.altitude * 1.0 / ALTITUDE_SCALING_FACTOR);
	strcat(payload, altitudeField);

	char hdopField[SINGLE_FIELD_MAX_LEN];
	sprintf(hdopField, "hdop=%d,", info.hdop / HDOP_SCALING_FACTOR);
	strcat(payload, hdopField);

	char validFixField[SINGLE_FIELD_MAX_LEN];
	sprintf(validFixField, "valid_fix=%d,", info.valid_fix);
	strcat(payload, validFixField);

	char numSatsField[SINGLE_FIELD_MAX_LEN];
	sprintf(numSatsField, "num_sats=%d", info.num_sats);
	strcat(payload, numSatsField);

	double timestamp = fixtimeToTimestamp(info.fixtime);

	if (!g_is_emergency)
	{
		char nextTimestampField[SINGLE_FIELD_MAX_LEN];
		memset(nextTimestampField, 0, strlen(nextTimestampField));
		sprintf(nextTimestampField, ",next_report=%.0lf", timestamp + ((double)g_report_frequency_ms * 10e5));
		strcat(payload, nextTimestampField);
	}

	strcat(payload, INFLUX_SECTION_SEPARATOR);

	// Add timestamp to end of payload
	char timestampField[TIMESTAMP_LEN];
	memset(timestampField, 0 ,strlen(timestampField));
	sprintf(timestampField, "%.0lf", timestamp);
	timestampField[TIMESTAMP_LEN] = '\0';
	strcat(payload, timestampField);

	return timestamp;
}


/*******************************************************************************
 * @brief Sends request with payload
 * payload - payload to be sent
 * url - url to send request to
 * method - method to use (0 - GET, 1 - POST)
 ******************************************************************************/
bool sendHTTPRequest(char *payload, char *url, char *response, int responseLen, int method)
{
	if (!CellularSendHTTPRequest(url, payload, strlen(payload),
			response, responseLen, method))
	{
		printf("FAIL\n");
		printf("ERROR: ");
		char errmsg[DEFAULT_RESPONSE_LENGTH];
		if (!(CellularGetLastError((char *)errmsg, DEFAULT_RESPONSE_LENGTH)))
		{
			printf("General Error\n");
		}
		else
		{
			printf("%s\n", errmsg);
		}
		return false;
	}

	printf("DONE\n");
	return true;
}

void reportGPS(char *ccid, bool readResponse)
{
	// Initialize GPS coordinate struct
	GPS_LOCATION_INFO info;
	info.latitude = 0;
	info.longitude = 0;
	info.altitude = 0;
	info.hdop = 0;
	info.valid_fix = 0;
	info.num_sats = 0;
	strcpy(info.fixtime, EMPTY_STRING);
	info.reserved1 = 0;

	// Attempts to fetch GPS coordinates every 5 seconds
	printf("\nFetching GPS.....");
	while (!GPSGetFixInformation(&info))
	{
		printf("FAIL\n\nretrying in 5 secs...\n");
		sleep(5000);
		printf("\nFetching GPS.....");
	}
	printf("DONE\n");

	// If cellular modem is not connected, attempts connection every 30 seconds
	if (!isOperatorConnected())
	{
		printf("\nConnecting.......");

		CellularSetOperator(2, NULL, NULL);
		while (!CellularSetOperator(0, NULL, NULL))
		{
			printf("FAIL\nretrying in 30 secs...\n");

			// Active waits for a minute before trying again
			uint32_t currTicks = msTicks;
			while (msTicks - currTicks < CONNECTION_RETRY_MS);
			printf("\nConnecting.......");
		}
		printf("DONE\n");
	}

	printf("\n*********************\n");
	printf("*Reporting location**\n");
	printf("*********************\n");

	char day[3], month[3], year[3], hour[3], minute[3];

	day[0] = info.fixtime[0];
	day[1] = info.fixtime[1];
	day[2] = '\0';

	month[0] = info.fixtime[2];
	month[1] = info.fixtime[3];
	month[2] = '\0';

	year[0] = info.fixtime[4];
	year[1] = info.fixtime[5];
	year[2] = '\0';

	hour[0] = info.fixtime[6];
	hour[1] = info.fixtime[7];
	hour[2] = '\0';

	minute[0] = info.fixtime[8];
	minute[1] = info.fixtime[9];
	minute[2] = '\0';

	printf(FIXTIME_STRING, day, month, year, hour, minute);

	printf("\nInternet setup...");
	while (!CellularSetupInternetConnectionProfile(20))
	{
		printf("FAIL\nretrying in 60 seconds...\n");

		// Active waits for a minute before trying again
		uint32_t currTicks = msTicks;
		while (msTicks - currTicks < CONNECTION_RETRY_MS);
		printf("\nInternet setup...");
	}
	printf("DONE\n");

	// If synchronization to the server is required, first reads frequency from there
	if (readResponse)
	{
		char readTimestampStr[TIMESTAMP_LEN] = {0};
		sprintf(readTimestampStr, "%.0f", g_read_timestamp);
		readTimestampStr[strlen(readTimestampStr)] = '\0';

		char readUrl[strlen(URL)
					  + strlen(QUERY_PATH)
					  + strlen(Q_PREFIX)
					  + strlen(ccid)
					  + strlen(Q_TIMESTAMP_CONDITION)
					  + strlen(readTimestampStr)
					  + strlen(Q_SUFFIX)
					  + 1];

		strcpy(readUrl, URL);
		strcat(readUrl, QUERY_PATH);
		strcat(readUrl, Q_PREFIX);
		strcat(readUrl, ccid);
		strcat(readUrl, Q_TIMESTAMP_CONDITION);
		strcat(readUrl, readTimestampStr);
		strcat(readUrl, Q_SUFFIX);
		readUrl[strlen(readUrl)] = '\0';

		char response[250] = {0};

		// 3 time retry counter
		for (int i = 0; i < 3; i++)
		{
			printf("Freq. update.....");
			if (sendHTTPRequest(NULL, readUrl, response, 250, GET_METHOD_CODE))
			{
				int serverFrequency = 0;
				if (strcmp(response, SERVER_EMPTY_RESPONSE) != 0)
				{
					sscanf(response, SERVER_FREQUENCY_FORMAT, &serverFrequency);
					if (serverFrequency * 6e4 != g_report_frequency_ms)
					{
						// Server frequency is in minutes
						g_report_frequency_ms = serverFrequency * 6e4;
						printf("Freq. changed by server\n");
					}
				}
				break;
			}

			// If sending fails, closes service profile before retrying
			else
			{
				closeConnection(GET_METHOD_CODE);
				printf("\nRetrying..........%d/3\n\n", i);
			}
		}
	}

	// Initialize GPS payload
	char payload[PAYLOAD_MAX_LEN];
	double currTimestamp = buildGPSPayload(info, ccid, payload);

	char writeUrl[strlen(URL) + strlen(WRITE_PATH) + 1];
	strcpy(writeUrl, URL);
	strcat(writeUrl, WRITE_PATH);
	writeUrl[strlen(writeUrl)] = '\0';

	for (int i = 0; i < 3; i++)
	{
		printf("Sending GPS......");
		if (sendHTTPRequest(payload, writeUrl, NULL, 0, POST_METHOD_CODE))
		{
			// On successful GPS send, update frequency read timestamp
			g_read_timestamp = currTimestamp;
			break;
		}
		else
		{
			closeConnection(POST_METHOD_CODE);
			printf("\nRetrying..........%d/3\n\n", i);
		}
	}

	int i = 0;
	while (i < 5)
	{
		if (g_frequency_intervals_ms[i] == g_report_frequency_ms)
		{
			printf("Next report in %s\n", g_frequency_interval_strings[i]);
			break;
		}
		i++;
	}
	if (i == 5)
	{
		printf("Next report in %dm", (int)(g_report_frequency_ms / 6e4));
	}

	// If report is not an emergency report, updates timestamp of last send
	if (!g_is_emergency)
	{
		g_gps_update_timestamp = msTicks;
	}
}

/*******************************************************************************
 * @brief  Main function
 ******************************************************************************/
int main(void)
{

	// Use default settings for DCDC
	EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_STK_DEFAULT;
	CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_STK_DEFAULT;

	// Chip errata
	CHIP_Init();

	// Init DCDC regulator with kit specific parameters
	EMU_DCDCInit(&dcdcInit);
	CMU_HFXOInit(&hfxoInit);

	// Switch HFCLK to HFXO and disable HFRCO
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);

	// Initialize the display module.
	DISPLAY_Init();

	// Initialize slider
	CAPSENSE_Init();

	// Configures SysTick for 1ms interrupts
	if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
	    while (1);
	}

	// Enables reading clear flag register.
	MSC->CTRL |= MSC_CTRL_IFCREADCLEAR;

	// Setup BTN0
	GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
	GPIO_ExtIntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, BSP_GPIO_PB0_PIN, false, true, true);

	// Setup BTN1
	GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);
	GPIO_ExtIntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, BSP_GPIO_PB1_PIN, false, true, true);

	// Enables interrupts from even and odd pins.
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);

	// Retarget stdio to a text display
	if (RETARGET_TextDisplayInit() != TEXTDISPLAY_EMSTATUS_OK) {
		while (1);
	}

	printf("\n*********************\n");
	printf("****Initialization***\n");
	printf("*********************\n");

	// Initializes GPS
	printf("\nGPS Init.........");
	GPSInit();
	printf("DONE\n");
	
	printf("Modem init.......");
	CellularInit(CELLULAR_PORT);
	printf("DONE\n");
	
	printf("Response check...");
	if (!CellularCheckModem())
	{
		printf("FAIL\n");
		printf("*Please reset device*");
		while(1);
	}
	printf("PASS\n");

	char ccid[CCID_LENGTH];

	printf("Acquiring CCID");
	if (!(CellularGetCCID((char *)ccid)))
	{
		printf("..ERROR\n");
		printf("*Please reset device*");
		while(1);
	}
	printf("...DONE\n");

	reportGPS(ccid, false);
	g_report_frequency_ms = g_frequency_intervals_ms[g_report_frequency_index];

	while (1)
	{

		// Increment frequency in case right side was touched
		if (CAPSENSE_getPressed(BUTTON1_CHANNEL) && !CAPSENSE_getPressed(BUTTON0_CHANNEL))
		{
			// Increase frequency without applying
			if (g_report_frequency_index + 1 < 5)
			{
				g_report_frequency_index += 1;
				printf("Frequency: 1/%s\n", g_frequency_interval_strings[g_report_frequency_index]);
				printf("Press BTN1 to apply\n\n");
			}
		}

		// Decrement frequency in case left side was touched
		else if (CAPSENSE_getPressed(BUTTON0_CHANNEL) && !CAPSENSE_getPressed(BUTTON1_CHANNEL))
		{
			// Increase frequency without applying
			if (g_report_frequency_index - 1 >= 0)
			{
				g_report_frequency_index -= 1;
				printf("Frequency: 1/%s\n", g_frequency_interval_strings[g_report_frequency_index]);
				printf("Press BTN1 to apply\n\n");
			}
		}

		if (msTicks - g_capsense_update_timestamp >= CAPSENSE_UPDATE_INTERVAL_MS)
		{
			CAPSENSE_Sense();
			g_capsense_update_timestamp = msTicks;
		}

		if (g_apply_frequency_change)
		{
			if (g_report_frequency_ms != g_frequency_intervals_ms[g_report_frequency_index])
			{
				// Apply frequency change
				g_report_frequency_ms = g_frequency_intervals_ms[g_report_frequency_index];
				g_apply_frequency_change = false;
				printf("Next report in %s\n", g_frequency_interval_strings[g_report_frequency_index]);
				reportGPS(ccid, false);
			}
			else
			{
				g_apply_frequency_change = false;
			}
		}

		// Report GPS when necessary
		else if (((msTicks - g_gps_update_timestamp >= g_report_frequency_ms)) || (g_is_emergency))
		{
			reportGPS(ccid, true);
			if (g_is_emergency)
			{
				g_is_emergency = false;
			}
		}
	}
}
