//
// Created by Segal on 02/05/2019.
//

#ifndef PART_1_CELLULAR_H
#define PART_1_CELLULAR_H

#include "serial_io_geckostk3402a_usart.h"
#include "cellular_constants.h"

/**
 * Struct used to describe cellular operator.
 */
typedef struct __OPERATOR_INFO
{
    char operatorName[20]; // long alphanumeric operator name.
    int operatorCode; // Numeric operator code.
    char accessTechnology[4]; // "2G" or "3G"
} OPERATOR_INFO;

/**
 * Initializes serial connection with cellular moden with port and baud rate
 * as defined in cellular_constants.h
 */
void CellularInit(char *port);

/**
 * Closes serial communication with cellular modem.
 */
void CellularDisable();


// Returns true if modem is currently connected to an operator, false otherwise
bool isOperatorConnected(void);

/**
 * Sends cellular command, reads response and validates it
 * command - command to sent
 * commandLength - length of command
 * readTimeout - timeout when reading response
 * responseBuffer - buffer for content of cellular modem response
 * responseBufferLength - length of response buffer
 * Returns true if operation was successful, and fales otherwise
 */
bool ProcessCellularCommand(char *command, int commandLength, int readTimeout, char *responseBuffer,
                            int responseBufferLength, char *requiredPrefix, int requiredPrefixLength);

/**
 * Checks if cellular modem is responsive using "AT" command.
 * @return true if cellular modem is responsive, and false otherwise
 */
bool CellularCheckModem(void);

/**
 * Runs "+CREG" command to check the registration status of the cellular modem
 * @param status - pointer to status, which will be populated if registration status is available
 * @return true if registration status is available, and false otherwise.
 */
bool CellularGetRegistrationStatus(int *status);

/**
 * Runs "+CSQ" command to check the cellular signal quality of current modem connection
 * @param csq - pointer to integer representing signal quality - will be populated if signal
 * quality is available.
 * @return true if signal quality is available, and false otherwise.
 */
bool CellularGetSignalQuality(int *csq);

/**
 * Forces modem to register/deregister from cellular network using "+COPS" command
 * @param mode - 0 - Auto-register to any netwrok (operatorName will be ignored)
 *               1 - Forces modem to register to network whose name is operatorName
 *               2 - Deregisters the modem from current network (operatorName will be ignored)
 * @param operatorName - Name of cellular operator to connect to, used only when mode = 1
 * @param accessTechnology - Access technology of cellular operator to connect to, only used when mode = 1
 * @return true if command was successful, and false otherwise.
 */
bool CellularSetOperator(int mode, char *operatorName, char *accessTechnology);

/**
 * Uses "+COPS=?" command to search for available operators
 * @param opList - array of OPERATOR_INFO of the operators found.
 * @param maxops - maximum amount of operators in opList.
 * @param numOpsFound - pointer to int containing number of operators found, will be at most maxops.
 * @return true if command was successful and operators were found, and false otherwise.
 */
bool CellularGetOperators(OPERATOR_INFO *opList, int maxops, int *numOpsFound);

/**
 * Uses "AT^SICS" command to initialize an internet connection
 * profile with inactTO=inact_time_sec,conType = GPRS0, and apn="postm2m.lu"
 */
bool CellularSetupInternetConnectionProfile(int inact_time_sec);

/**
 * Sends an HTTP request, opens and closes the socket.
 * URL - complete address of the page being posted to.
 * payload - POST request content
 * payload_len - length of payload
 * response - buffer for content of HTTP response
 * response_max_len - length of response buffer
 * mode - HTTP method to be sent (0 - GET, 1 - POST)
 * Returns number of bytes in response, or -1 on error
 */
int CellularSendHTTPRequest(char *URL, char *payload, int payload_len, char *response, int response_max_len, int mode);

/**
 * Close connection of serviceProfile (used in case sending fails)
 */
bool closeConnection(int serviceProfile);

/**
 * Returns additional information in case of error from
 * CellularSendHTTPPOSTRequest. Response is in format
 * "<urcInfoId>,<urcInfoText>"
 */
int CellularGetLastError(char *errmsg, int errmsg_max_len);

/**
 * Fetches CCID from cellular modem.
 * CCID - Buffer to contain CCID
 * Returns true if CCID was fetched successfully and false otherwise.
 */
bool CellularGetCCID(char *CCID);

#endif //PART_1_CELLULAR_H
