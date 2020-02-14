//
// Created by Segal on 02/05/2019.
//

#ifndef PART_1_CELLULAR_CONSTANTS_H
#define PART_1_CELLULAR_CONSTANTS_H

// Connection constants
#define CELLULAR_PORT "0"
#define CELLULAR_BAUD 115200

#define ASCII_CONVERSION_CHAR '0'

// Buffer constants
#define DEFAULT_BUFFER_SIZE 500

// Timeout constants
#define SHORT_READ_TIMEOUT 800
#define MEDIUM_READ_TIMEOUT 8000
#define LONG_READ_TIMEOUT 120000

// AT command constants
#define CLOSE_QUOTES "\""
#define COMMA ","

#define ECHO_OFF_COMMAND "ATE0"
#define BASIC_AT_COMMAND "AT"
#define AT_SUFFIX "\r\n"
#define REGISTRATION_STATUS_READ_COMMAND "+CREG?"
#define SIGNAL_QUALITY_READ_COMMAND "+CSQ"
#define OPERATOR_WRITE_COMMAND "+COPS="
#define OPERATOR_WRITE_ARGS ",2,\""
#define OPERATOR_WRITE_ACT "\","
#define OPERATOR_READ_COMMAND "+COPS?"


#define OPERATOR_TEST_COMMAND "+COPS=?"

#define ICCID_READ_COMMAND "+CCID?"

#define SICS_CONTYPE_COMMAND "^SICS=0,conType,\"GPRS0\""
#define SICS_APN_COMMAND "^SICS=0,apn,\"REDACTED""
#define SICS_INACTTO_COMMAND "^SICS=0,inactTO,\""

#define GET_METHOD_CODE 0
#define POST_METHOD_CODE 1

#define SISS_PREFIX "^SISS="

#define SISS_SRVTYPE_COMMAND ",\"SrvType\",\"Http\""
#define SISS_CONID_COMMAND ",\"conId\",\"0\""
#define SISS_ADRRESS_COMMAND ",\"address\",\""
#define SISS_CMD_COMMAND ",\"cmd\",\""
#define SISS_HCCONTLEN_COMMAND ",\"hcContLen\",\"0\""
#define SISS_HCCONTENT_COMMAND ",\"hcContent\",\""
#define SISO_COMMAND "^SISO="
#define SISC_COMMAND "^SISC="
#define SISR_COMMAND "^SISR="
#define SISE_COMMAND "^SISE="

#define PARTNER_OPERATOR_CODE 42501
#define CELLCOM_OPERATOR_CODE 42502
#define PELEPHONE_OPERATOR_CODE 42503

// AT URC constants
#define SIS_URC_PREFIX "^SIS: "
#define SIS_URC_SUFFIX ",0,"
#define SIS_URC_MIN_ERROR_ID 1
#define SIS_URC_MAX_ERROR_ID 2000
#define SISW_URC_SENT "^SISW: 0,2"
#define SISR_URC_READY "^SISR: 0,1"

// AT response constants
#define REGISTRATION_STATUS_PREFIX "\r\n+CREG: "
#define SIGNAL_QUALITY_PREFIX "\r\n+CSQ: "
#define OPERATOR_PREFIX "\r\n+COPS: "
#define AUTO_CONNECTED_OPERATOR_PREFIX "\r\n+COPS: 0"
#define FORCED_CONNECTED_OPERATOR_PREFIX "\r\n+COPS: 1"
#define ICCID_PREFIX "\r\n+CCID: "
#define OK_MESSAGE "\r\nOK"
#define SISW_READY_PREFIX "\r\n^SISW: 0"
#define SISE_PREFIX "\r\n^SISE: 0,"

// AT response fields constants
#define REGSTATUS_INDEX 11
#define CSQ_INDEX 8
#define OPERATOR_ALPHANUMERIC_NAME_ARG_NUM 2
#define OPERATOR_CODE_ARG_NUM 4
#define OPERATOR_ACCESS_TECH_ARG_NUM 5
#define REGISTERED_STATUS 1
#define ROAMING_STATUS 5
#define GSM_ACCESS_TECHNOLOGY 0
#define SECOND_GENERATION "2G"
#define UTRAN_ACCESS_TECHNOLOGY 2
#define THIRD_GENERATION "3G"
#define CONFIRMED_WRITE_LENGTH_INDEX 11
#define CCID_LENGTH 20

// Signal quality constants
#define INITIAL_SIGNAL_QUALITY -113
#define UNKNOWN_SIGNAL_QUALITY 99

#endif //PART_1_CELLULAR_CONSTANTS_H