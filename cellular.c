//
// Created by Segal on 02/05/2019.
//

#include <stdlib.h>
#include <string.h>
#include "cellular.h"

bool ProcessCellularCommand(char *command, int commandLength, int readTimeout, char *responseBuffer,
                            int responseBufferLength, char *requiredPrefix, int requiredPrefixLength)
{
    if (!SerialSend_USART((unsigned char *)command, commandLength))
    {
        return false;
    }

    if (readTimeout > 0)
    {
        memset(responseBuffer, 0 ,strlen(responseBuffer));
        if (!SerialRecv_USART((unsigned char *) responseBuffer, responseBufferLength, readTimeout))
        {
            return false;
        }

        if (requiredPrefixLength > 0)
        {
            char responsePrefix[requiredPrefixLength + 1];
            strncpy(responsePrefix, (char *)responseBuffer, requiredPrefixLength);
            responsePrefix[requiredPrefixLength] = '\0';
            return strcmp(responsePrefix, requiredPrefix) == 0;
        }

        return true;
    }
    else
    {
        SerialFlushInputBuff_USART();
        return true;
    }
}

void CellularInit(char *port)
{
    // Initializes serial communication
    if (!SerialInit_USART(port, CELLULAR_BAUD))
    {
        exit(1);
    }

    // Builds echo off AT command
    char echoOffCommand[strlen(ECHO_OFF_COMMAND) + strlen(AT_SUFFIX) + 1];

    strcpy(echoOffCommand, ECHO_OFF_COMMAND);
    strcat(echoOffCommand, AT_SUFFIX);

    char buffer[DEFAULT_BUFFER_SIZE];
    ProcessCellularCommand(echoOffCommand, strlen(echoOffCommand), SHORT_READ_TIMEOUT,
    		buffer, DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE));

    SerialFlushInputBuff_USART();
}

void CellularDisable()
{
    SerialDisable_USART();
}

bool CellularCheckModem(void)
{

    // Builds basic AT command
    char basicATCommand[strlen(BASIC_AT_COMMAND) + strlen(AT_SUFFIX) + 1];

    strcpy(basicATCommand, BASIC_AT_COMMAND);
    strcat(basicATCommand, AT_SUFFIX);

    char basicATResponse[DEFAULT_BUFFER_SIZE];

    return ProcessCellularCommand(basicATCommand, strlen(basicATCommand), SHORT_READ_TIMEOUT,
            basicATResponse, DEFAULT_BUFFER_SIZE, NULL, 0);
}

bool CellularGetRegistrationStatus(int *status)
{

    // Builds registration status AT command
    char regStatusCommand[strlen(BASIC_AT_COMMAND)
               + strlen(REGISTRATION_STATUS_READ_COMMAND)
               + strlen(AT_SUFFIX) + 1];

    strcpy(regStatusCommand, BASIC_AT_COMMAND);
    strcat(regStatusCommand, REGISTRATION_STATUS_READ_COMMAND);
    strcat(regStatusCommand, AT_SUFFIX);

    char regStatus[DEFAULT_BUFFER_SIZE];

    if (!ProcessCellularCommand(regStatusCommand, strlen(regStatusCommand), SHORT_READ_TIMEOUT, regStatus,
                                DEFAULT_BUFFER_SIZE, REGISTRATION_STATUS_PREFIX, strlen(REGISTRATION_STATUS_PREFIX)))
    {
        return false;
    }
    else
    {
        *status = (regStatus[REGSTATUS_INDEX] - ASCII_CONVERSION_CHAR);
        return true;
    }
}

bool CellularGetSignalQuality(int *csq)
{

    // Initializes signal quality
    int initial_csq = INITIAL_SIGNAL_QUALITY;

    // Builds signal quality AT command
    char csqCommand[strlen(BASIC_AT_COMMAND)
                 + strlen(SIGNAL_QUALITY_READ_COMMAND)
                 + strlen(AT_SUFFIX) + 1];

    strcpy(csqCommand, BASIC_AT_COMMAND);
    strcat(csqCommand, SIGNAL_QUALITY_READ_COMMAND);
    strcat(csqCommand, AT_SUFFIX);

    char sigQuality[DEFAULT_BUFFER_SIZE];

    if (!ProcessCellularCommand(csqCommand, strlen(csqCommand), SHORT_READ_TIMEOUT, sigQuality,
                                DEFAULT_BUFFER_SIZE, SIGNAL_QUALITY_PREFIX, strlen(SIGNAL_QUALITY_PREFIX)))
    {
        return false;
    }
    else
    {
        // Returns false on unknown signal quality
        char *commaLoc = strchr((char *)sigQuality, ',');
        int rssi = strtol((char *)sigQuality + CSQ_INDEX, &commaLoc, 10);
        if (rssi == UNKNOWN_SIGNAL_QUALITY)
        {
            return false;
        }

        // Calculates signal quality in dBm
        *csq = initial_csq + 2 * rssi;
        return true;
    }
}

bool isOperatorConnected(void)
{

	// Builds get operator AT command
    char readOpeartorCommand[strlen(BASIC_AT_COMMAND)
							 + strlen(OPERATOR_TEST_COMMAND)
							 + strlen(AT_SUFFIX)];

    strcpy(readOpeartorCommand, BASIC_AT_COMMAND);
    strcat(readOpeartorCommand, OPERATOR_READ_COMMAND);
    strcat(readOpeartorCommand, AT_SUFFIX);

    char responseBuffer[DEFAULT_BUFFER_SIZE];
    return ((ProcessCellularCommand(readOpeartorCommand, strlen(readOpeartorCommand), SHORT_READ_TIMEOUT, responseBuffer,
                                DEFAULT_BUFFER_SIZE, AUTO_CONNECTED_OPERATOR_PREFIX, strlen(AUTO_CONNECTED_OPERATOR_PREFIX))) ||
    		(ProcessCellularCommand(readOpeartorCommand, strlen(readOpeartorCommand), SHORT_READ_TIMEOUT, responseBuffer,
    				DEFAULT_BUFFER_SIZE, FORCED_CONNECTED_OPERATOR_PREFIX, strlen(FORCED_CONNECTED_OPERATOR_PREFIX))));
}

bool CellularSetOperator(int mode, char *operatorName, char *accessTechnology)
{
    // Returns false on unsupported mode
    if ((mode < 0) || (mode > 2))
    {
        return false;
    }

    // Parses mode as character
    char mode_char[2];
    mode_char[0] = mode + ASCII_CONVERSION_CHAR;

    // Builds set operator AT command
    char setOperatorCommand[strlen(BASIC_AT_COMMAND) +
                            strlen(OPERATOR_WRITE_COMMAND) +
                            strlen(mode_char) + 1];

    strcpy(setOperatorCommand, BASIC_AT_COMMAND);
    strcat(setOperatorCommand, OPERATOR_WRITE_COMMAND);
    strcat(setOperatorCommand, mode_char);

    char setOperatorCommandArgs[strlen(OPERATOR_WRITE_ARGS) +
                                strlen(operatorName) +
                                strlen(OPERATOR_WRITE_ACT) +
								strlen(accessTechnology) + 1];

    // If mode is forced connection, includes connection args in command.
    if (mode == 1)
    {
        strcpy(setOperatorCommandArgs, OPERATOR_WRITE_ARGS);
        strcat(setOperatorCommandArgs, operatorName);
        strcat(setOperatorCommandArgs, OPERATOR_WRITE_ACT);
        strcat(setOperatorCommandArgs, accessTechnology);

    }
    else
    {
        setOperatorCommandArgs[0] = '\0';
    }

    char fullSetOperatorCommand[strlen(setOperatorCommand)
                                + strlen(setOperatorCommandArgs)
                                + strlen(AT_SUFFIX) + 1];

    strcpy(fullSetOperatorCommand, setOperatorCommand);
    strcat(fullSetOperatorCommand, setOperatorCommandArgs);
    strcat(fullSetOperatorCommand, AT_SUFFIX);

    char responseBuffer[DEFAULT_BUFFER_SIZE];
    return ProcessCellularCommand(fullSetOperatorCommand, strlen(fullSetOperatorCommand),
                                  LONG_READ_TIMEOUT, responseBuffer, DEFAULT_BUFFER_SIZE, OK_MESSAGE,
                                  strlen(OK_MESSAGE));
}

bool CellularGetOperators(OPERATOR_INFO *opList, int maxops, int *numOpsFound)
{

    // Builds get operator AT command
    char getOpeartorsCommand[strlen(BASIC_AT_COMMAND)
                 + strlen(OPERATOR_TEST_COMMAND)
                 + strlen(AT_SUFFIX) + 1];

    strcpy(getOpeartorsCommand, BASIC_AT_COMMAND);
    strcat(getOpeartorsCommand, OPERATOR_TEST_COMMAND);
    strcat(getOpeartorsCommand, AT_SUFFIX);

    char allOperators[DEFAULT_BUFFER_SIZE];

    if (!ProcessCellularCommand(getOpeartorsCommand, strlen(getOpeartorsCommand), LONG_READ_TIMEOUT, allOperators,
                                DEFAULT_BUFFER_SIZE, OPERATOR_PREFIX, strlen(OPERATOR_PREFIX)))
    {
        return false;
    }
    else
    {
        *numOpsFound = 0;

        // Parses operators according to parentheses
        char *operatorStart = strchr((char *)allOperators, '(');
        char *operatorEnd = strchr((char *)allOperators, ')');
        while (operatorStart != NULL)
        {
            operatorStart++;
            operatorEnd++;

            // If operator is available, builds __OPERATOR_INFO struct
            if (strtol(operatorStart, NULL, 10) == 1)
            {
                OPERATOR_INFO opInfo = {0};
                char *argument = strchr((char *)operatorStart, ',');
                int argCount = OPERATOR_ALPHANUMERIC_NAME_ARG_NUM;

                // Parses operator arguments and populates struct
                while ((argument != NULL) && (operatorEnd - argument > 0))
                {
                    argument++;
                    if (argCount == OPERATOR_ALPHANUMERIC_NAME_ARG_NUM)
                    {
                        strncpy(opInfo.operatorName, argument + 1,
                                (int)(strchr(argument + 1, '"') - (int)(argument + 1) ));
                    }
                    else if (argCount == OPERATOR_CODE_ARG_NUM)
                    {
                        opInfo.operatorCode = strtol(argument + 1, NULL, 10);
                    }
                    else if (argCount == OPERATOR_ACCESS_TECH_ARG_NUM)
                    {
                        int accessTechnology = strtol(argument, NULL, 10);
                        if (accessTechnology == GSM_ACCESS_TECHNOLOGY)
                        {
                            strcpy(opInfo.accessTechnology, SECOND_GENERATION);
                        }
                        else if (accessTechnology == UTRAN_ACCESS_TECHNOLOGY)
                        {
                            strcpy(opInfo.accessTechnology, THIRD_GENERATION);
                        }
                        else
                        {
                            return false;
                        }
                    }
                    argument = strchr(argument, ',');
                    argCount++;
                }
                memcpy(opList, &opInfo, sizeof(opInfo));
                opList++;
                *numOpsFound = (*numOpsFound) + 1;

                // Stops at maxops if exceeded
                if (*numOpsFound >= maxops)
                {
                    return true;
                }
            }
            operatorStart = strchr(operatorStart, '(');
            operatorEnd = strchr(operatorEnd, ')');
        }
        // Returns true if at least 1 operator was found.
        return *numOpsFound > 0;
    }
}

bool CellularGetCCID(char *CCID)
{
	char responseBuffer[DEFAULT_BUFFER_SIZE];

	// Builds AT CCIDCommand
	char ccidCommand[strlen(BASIC_AT_COMMAND)
					 + strlen(ICCID_READ_COMMAND)
					 + strlen(AT_SUFFIX) + 1];

	strcpy(ccidCommand, BASIC_AT_COMMAND);
	strcat(ccidCommand, ICCID_READ_COMMAND);
	strcat(ccidCommand, AT_SUFFIX);

	if (!ProcessCellularCommand(ccidCommand, strlen(ccidCommand), SHORT_READ_TIMEOUT, responseBuffer,
								DEFAULT_BUFFER_SIZE, ICCID_PREFIX, strlen(ICCID_PREFIX))) return false;

	char *responseStart = responseBuffer + strlen(ICCID_PREFIX);
	char *responseEnd = strstr(responseBuffer + 1, AT_SUFFIX) + strlen(AT_SUFFIX);
	responseStart[(int)responseEnd - (int)responseStart] = '\0';
	strncpy(CCID, responseStart, CCID_LENGTH);
	CCID[CCID_LENGTH - 1] = '\0';
	return true;
}

bool CellularSetupInternetConnectionProfile(int inact_time_sec)
{
	char sicsResponse[DEFAULT_BUFFER_SIZE];

    // Set connection type to GPRS0
    char conTypeCommand[strlen(BASIC_AT_COMMAND)
                         + strlen(SICS_CONTYPE_COMMAND)
                         + strlen(AT_SUFFIX) + 1];

    strcpy(conTypeCommand, BASIC_AT_COMMAND);
    strcat(conTypeCommand, SICS_CONTYPE_COMMAND);
    strcat(conTypeCommand, AT_SUFFIX);

    if (!ProcessCellularCommand(conTypeCommand, strlen(conTypeCommand), SHORT_READ_TIMEOUT, sicsResponse,
                                DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

    // Set apn to postm2m.lu
    char apnCommand[strlen(BASIC_AT_COMMAND)
                         + strlen(SICS_APN_COMMAND)
                         + strlen(AT_SUFFIX) + 1];

    strcpy(apnCommand, BASIC_AT_COMMAND);
    strcat(apnCommand, SICS_APN_COMMAND);
    strcat(apnCommand, AT_SUFFIX);

    if (!ProcessCellularCommand(apnCommand, strlen(apnCommand), SHORT_READ_TIMEOUT, sicsResponse,
                                DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

    // Set inactive timeout to inact_time_sec
    char inact_time_sec_str[12];
    sprintf(inact_time_sec_str, "%d", inact_time_sec);

    char inactToCommand[strlen(BASIC_AT_COMMAND)
                         + strlen(SICS_INACTTO_COMMAND)
                         + strlen(inact_time_sec_str)
                         + strlen(CLOSE_QUOTES)
                         + strlen(AT_SUFFIX) + 1];

    strcpy(inactToCommand, BASIC_AT_COMMAND);
    strcat(inactToCommand, SICS_INACTTO_COMMAND);
    strcat(inactToCommand, inact_time_sec_str);
    strcat(inactToCommand, CLOSE_QUOTES);
    strcat(inactToCommand, AT_SUFFIX);

    return (ProcessCellularCommand(inactToCommand, strlen(inactToCommand), SHORT_READ_TIMEOUT, sicsResponse,
                                   DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE)));
}

int CellularSendHTTPRequest(char *URL, char *payload, int payloadLen, char *response, int response_max_len
		,int method)
{
	if ((method > POST_METHOD_CODE) || (method < GET_METHOD_CODE))
	{
		return 0;
	}

	char methodChar[2] = {0};
	methodChar[0] = method + '0';

	char responseBuffer[DEFAULT_BUFFER_SIZE];

	// Set request service type to HTTP
	char srvTypeCommand[strlen(BASIC_AT_COMMAND)
						+ strlen(SISS_PREFIX)
						+ strlen(methodChar)
						+ strlen(SISS_SRVTYPE_COMMAND)
						+ strlen(AT_SUFFIX) + 1];

	strcpy(srvTypeCommand, BASIC_AT_COMMAND);
	strcat(srvTypeCommand, SISS_PREFIX);
	strcat(srvTypeCommand, methodChar);
	strcat(srvTypeCommand, SISS_SRVTYPE_COMMAND);
	strcat(srvTypeCommand, AT_SUFFIX);
	srvTypeCommand[strlen(srvTypeCommand)] = '\0';

	if (!ProcessCellularCommand(srvTypeCommand, strlen(srvTypeCommand), SHORT_READ_TIMEOUT, responseBuffer,
								DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

	// Set connection profile ID
	char conIdCommand[strlen(BASIC_AT_COMMAND)
					  + strlen(SISS_PREFIX)
					  + strlen(methodChar)
					  + strlen(SISS_CONID_COMMAND)
					  + strlen(AT_SUFFIX) + 1];

	strcpy(conIdCommand, BASIC_AT_COMMAND);
	strcat(conIdCommand, SISS_PREFIX);
	strcat(conIdCommand, methodChar);
	strcat(conIdCommand, SISS_CONID_COMMAND);
	strcat(conIdCommand, AT_SUFFIX);
	conIdCommand[strlen(conIdCommand)] = '\0';

	if (!ProcessCellularCommand(conIdCommand, strlen(conIdCommand), SHORT_READ_TIMEOUT, responseBuffer,
								DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

    // Set URL as request URL
    char addressCommand[strlen(BASIC_AT_COMMAND)
						+ strlen(SISS_PREFIX)
						+ strlen(methodChar)
						+ strlen(SISS_ADRRESS_COMMAND)
						+ strlen(URL)
						+ strlen(CLOSE_QUOTES)
						+ strlen(AT_SUFFIX) + 1];

    strcpy(addressCommand, BASIC_AT_COMMAND);
    strcat(addressCommand, SISS_PREFIX);
    strcat(addressCommand, methodChar);
    strcat(addressCommand, SISS_ADRRESS_COMMAND);
    strcat(addressCommand, URL);
    strcat(addressCommand, CLOSE_QUOTES);
    strcat(addressCommand, AT_SUFFIX);
    addressCommand[strlen(addressCommand)] = '\0';

    if (!ProcessCellularCommand(addressCommand, strlen(addressCommand), SHORT_READ_TIMEOUT, responseBuffer,
                                DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;


	// Set request method
	char cmdCommand[strlen(BASIC_AT_COMMAND)
					+ strlen(SISS_PREFIX)
					+ strlen(methodChar)
					+ strlen(SISS_CMD_COMMAND)
					+ strlen(methodChar)
					+ strlen(CLOSE_QUOTES)
					+ strlen(AT_SUFFIX) + 1];

	strcpy(cmdCommand, BASIC_AT_COMMAND);
	strcat(cmdCommand, SISS_PREFIX);
	strcat(cmdCommand, methodChar);
	strcat(cmdCommand, SISS_CMD_COMMAND);
	strcat(cmdCommand, methodChar);
	strcat(cmdCommand, CLOSE_QUOTES);
	strcat(cmdCommand, AT_SUFFIX);
	cmdCommand[strlen(cmdCommand)] = '\0';

	if (!ProcessCellularCommand(cmdCommand, strlen(cmdCommand), SHORT_READ_TIMEOUT, responseBuffer,
								DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

	// Set request content length
	char hcContLenCommand[strlen(BASIC_AT_COMMAND)
						  + strlen(SISS_PREFIX)
						  + strlen(methodChar)
						  + strlen(SISS_HCCONTLEN_COMMAND)
						  + strlen(AT_SUFFIX) + 1];

	strcpy(hcContLenCommand, BASIC_AT_COMMAND);
	strcat(hcContLenCommand, SISS_PREFIX);
	strcat(hcContLenCommand, methodChar);
	strcat(hcContLenCommand, SISS_HCCONTLEN_COMMAND);
	strcat(hcContLenCommand, AT_SUFFIX);
	hcContLenCommand[strlen(hcContLenCommand)] = '\0';

	if (!ProcessCellularCommand(hcContLenCommand, strlen(hcContLenCommand), SHORT_READ_TIMEOUT, responseBuffer,
								DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

	if (payloadLen != 0)
	{
		// Set request payload
		char hcContentCommand[strlen(BASIC_AT_COMMAND)
							  + strlen(SISS_PREFIX)
							  + strlen(methodChar)
							  + strlen(SISS_HCCONTENT_COMMAND)
							  + payloadLen
							  + strlen(CLOSE_QUOTES)
							  + strlen(AT_SUFFIX) + 1];

		strcpy(hcContentCommand, BASIC_AT_COMMAND);
		strcat(hcContentCommand, SISS_PREFIX);
		strcat(hcContentCommand, methodChar);
		strcat(hcContentCommand, SISS_HCCONTENT_COMMAND);
		strcat(hcContentCommand, payload);
		strcat(hcContentCommand, CLOSE_QUOTES);
		strcat(hcContentCommand, AT_SUFFIX);
		hcContentCommand[strlen(hcContentCommand)] = '\0';

		if (!ProcessCellularCommand(hcContentCommand, strlen(hcContentCommand), SHORT_READ_TIMEOUT, responseBuffer,
									DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;
	}

    // Create request
    char sisoCommand[strlen(BASIC_AT_COMMAND)
                     + strlen(SISO_COMMAND)
					 + strlen(methodChar)
                     + strlen(AT_SUFFIX) + 1];

    strcpy(sisoCommand, BASIC_AT_COMMAND);
    strcat(sisoCommand, SISO_COMMAND);
    strcat(sisoCommand, methodChar);
    strcat(sisoCommand, AT_SUFFIX);
    sisoCommand[strlen(sisoCommand)] = '\0';

    if (!ProcessCellularCommand(sisoCommand, strlen(sisoCommand), MEDIUM_READ_TIMEOUT, responseBuffer,
                                DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE))) return false;

    char urcCode[strlen(SIS_URC_PREFIX)
				 + strlen(methodChar)
				 + strlen(SIS_URC_SUFFIX) + 1];
    strcpy(urcCode, SIS_URC_PREFIX);
    strcat(urcCode, methodChar);
    strcat(urcCode, SIS_URC_SUFFIX);
    urcCode[strlen(urcCode)] = '\0';

    // Parse URCs:
    char *sisUrc = strstr((char *)responseBuffer, urcCode);

    // If SIS cause is not 0, returns false
    if (sisUrc == NULL) return false;

    // If urcInfoId contains an error, returns false
    int urcInfoId = strtol(sisUrc + strlen(urcCode), NULL, 10);
    if ((urcInfoId >= SIS_URC_MIN_ERROR_ID) && (urcInfoId <= SIS_URC_MAX_ERROR_ID))
    {
        return false;
    }

    // Build SISC command to close connection in case of error
    char siscCommand[strlen(BASIC_AT_COMMAND)
                     + strlen(SISC_COMMAND)
					 + strlen(methodChar)
                     + strlen(AT_SUFFIX) + 1];

    strcpy(siscCommand, BASIC_AT_COMMAND);
    strcat(siscCommand, SISC_COMMAND);
    strcat(siscCommand, methodChar);
    strcat(siscCommand, AT_SUFFIX);
    siscCommand[strlen(siscCommand)] = '\0';

    if (response_max_len > 0)
    {
//		char *siswUrc = strstr((char *)responseBuffer, SISW_URC_SENT);
		char *sisrUrc = strstr((char *)responseBuffer, SISR_URC_READY);

		// If data is not ready to read, close connection and return false
		if (sisrUrc == NULL)
		{
			ProcessCellularCommand(siscCommand, strlen(siscCommand), 0, NULL, 0, NULL, 0);
			return false;
		}

		char response_max_len_str[12];
		sprintf(response_max_len_str, "%d", response_max_len);

		// Read response from address
		char sisrCommand[strlen(BASIC_AT_COMMAND)
						 + strlen(SISR_COMMAND)
						 + strlen(methodChar)
						 + strlen(COMMA)
						 + strlen(response_max_len_str)
						 + strlen(AT_SUFFIX) + 1];

		strcpy(sisrCommand, BASIC_AT_COMMAND);
		strcat(sisrCommand, SISR_COMMAND);
		strcat(sisrCommand, methodChar);
		strcat(sisrCommand, COMMA);
		strcat(sisrCommand, response_max_len_str);
		strcat(sisrCommand, AT_SUFFIX);
		sisrCommand[strlen(sisrCommand)] = '\0';

		if (!ProcessCellularCommand(sisrCommand, strlen(sisrCommand), SHORT_READ_TIMEOUT, responseBuffer,
				DEFAULT_BUFFER_SIZE, NULL, 0))
		{
			ProcessCellularCommand(siscCommand, strlen(siscCommand), 0, NULL, 0, NULL, 0);
			return false;
		}

		char *responseStart = strstr(responseBuffer + 1, AT_SUFFIX) + strlen(AT_SUFFIX);
		char *responseEnd = strstr(responseBuffer, OK_MESSAGE);
		responseStart[(int)responseEnd - (int)responseStart - 1] = '\0';
		strcpy(response, responseStart);
    }

    // Close connection after reading response
    if (!ProcessCellularCommand(siscCommand, strlen(siscCommand), SHORT_READ_TIMEOUT, responseBuffer,
    		DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE)))
    {
    	return false;
    }

    // Return length of response
    if (response_max_len > 0)
    {
		// Return actual length of response
		return strlen(response);
    }

    return true;

}

bool closeConnection(int serviceProfile)
{
	char serviceProfileChar[2] = {0};
	serviceProfileChar[0] = serviceProfile + '0';

	char responseBuffer[DEFAULT_BUFFER_SIZE];

    // Build SISC command to close connection
    char siscCommand[strlen(BASIC_AT_COMMAND)
                     + strlen(SISC_COMMAND)
					 + strlen(serviceProfileChar)
                     + strlen(AT_SUFFIX) + 1];

    strcpy(siscCommand, BASIC_AT_COMMAND);
    strcat(siscCommand, SISC_COMMAND);
    strcat(siscCommand, serviceProfileChar);
    strcat(siscCommand, AT_SUFFIX);
    siscCommand[strlen(siscCommand)] = '\0';

    // Close connection after reading response
	if (!ProcessCellularCommand(siscCommand, strlen(siscCommand), SHORT_READ_TIMEOUT, responseBuffer,
			DEFAULT_BUFFER_SIZE, OK_MESSAGE, strlen(OK_MESSAGE)))
	{
		return false;
	}
	return true;
}

int CellularGetLastError(char *errmsg, int errmsg_max_len)
{
    // Builds AT SISE Command
    char siseCommand[strlen(BASIC_AT_COMMAND)
                     + strlen(SISE_COMMAND)
                     + strlen(AT_SUFFIX) + 1];

    strcpy(siseCommand, BASIC_AT_COMMAND);
    strcat(siseCommand, SISE_COMMAND);
    strcat(siseCommand, AT_SUFFIX);

    char responseBuffer[DEFAULT_BUFFER_SIZE];

    if (!ProcessCellularCommand(siseCommand, strlen(siseCommand), SHORT_READ_TIMEOUT, responseBuffer,
                                DEFAULT_BUFFER_SIZE, SISE_PREFIX, strlen(SISE_PREFIX))) return false;

    strncpy(errmsg, responseBuffer + strlen(SISE_PREFIX), errmsg_max_len);

    return strlen(errmsg);
}
