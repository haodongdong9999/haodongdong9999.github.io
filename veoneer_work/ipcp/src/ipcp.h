/**
 * \file        ipcp.h
 *
 * \brief       A brief description (one line).
 *
 * [Optional] A detailed description of the function, purpose
 * and usage of this file (more lines).
 *
 * \author $Author: Sunil Kumar Nagaborappa (sunilkumar.nagaborappa) $
 *
 * Revision:    $Revision: 1.1 $
 * Created on:  $Date: 2018/12/10 22:56:35IST $
 *
 * <br>=====================================================<br>
 * <b>Copyright 2012 by Veoneer India Pvt Ltd.</b>
 *
 *  All rights reserved. Property of Veoneer India Pvt Ltd.<br>
 *  Restricted rights to use, duplicate or disclose of this code<br>
 *  are granted through contract.
 * <br>=====================================================<br>
 */

/*==================[inclusions]============================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <mqueue.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/sockio.h>
#include <net/if.h>

typedef float float32_t;
typedef double float64_t;

#define DEBUG
#ifdef DEBUG
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

/*==================[external constants]====================================*/
/*==================[macros]================================================*/
#define STATIC static
#define LOOP_ENDLESS				(1u)
#define MAX_MTU_SIZE 				(1500u)
#define IPCP_PKT_MAX_LEN			(1400u)
#define MAX_NUM_RX_PKT 				(10u)
#define DEFAULT_NUMRETRIES 			(10u)
#define DEFAULT_WFA_TIMEOUT 		(500u)
#define IPCP_ENCODED_MESSAGE 		(0x00u)
#define IPCP_NORMAL_MESSAGE		 	(0x01u)
#define IPCP_TOTAL_HDR_LENGTH 		(16u)
#define DEFAULT_IPCP_HDR_LENGTH 	(8u)
#define DEFAULT_IPCP_VERSION 		(0x01u)
#define IPCP_HEADER_OFFSET 			(0u)
#define IPCP_PAYLOAD_OFFSET 		(IPCP_HEADER_OFFSET + 16u)
#define IPCP_MAX_PAYLOAD_LEN		(1384u)
#define ENDLESS_LOOP				(1)
#define ASDM_IPCP_NAME              "vlan0"
/*ASDM IP Address*/
#define EHP_IHU_PORT 				(50174u)
#define ASDM_IPADDRESS 				"198.18.36.1"

/*EHP IHU IP Address*/
#define EHP_IHU_IP_ADDRESS 			"198.18.34.15"

/*IPCP Protocol Version */
#define DEFAULT_PROTOCOL_VER 		(0x03u)

/*IPCP ERROR values*/
#define SOCKET_ERROR 				(-1)
#define IPCP_GENERIC_ERROR 			(-2)
#define IPCP_BUF_NOT_AVAILABLE 		(-3)
#define IPCP_INVALID_OPTYPE 		(-4)
#define IPCP_UDP_SEND_FAILURE 		(-5)
#define IPCP_HDR_ERROR 				(-6)

/* Default configured timeout values */

#define TIMEDOUT -1

#define IPCP_DEFAULT_TIMEOUT_WFA   500     /* Default time out value to receive the ACK is 500 millisecs */
#define IPCP_DEFAULT_TIMEOUT_WFR   500     /* Default time out value to receive the Response is 500 millisecs after ACK received*/
#define IPCP_MAX_RETRY_CNT_ACK     7       /* Maximum num of resend request until either recieve the ACK or reaches to max retry cnt */
#define IPCP_MAX_RETRY_CNT_RES     2       /* Maximum num of resend request until either recieve the response or reaches to max retry cnt */
#define IPCP_INC_TIMER_VALUE_WFA   1.5     /* Increase the ACK wait time by 1.5 times for every resend */
#define IPCP_INC_TIMER_VALUE_WFR   2       /* Increase the response wait time by 2 times for every resend */



/*==================[type definitions]======================================*/
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef int SOCKET;
typedef struct timespec IPCP_Timer;
/*Supported operation ID*/
typedef enum
{
	TJP_GLOBAL_DATA_MESSAGE = 0x0001,    ///< Global Data message.
	TJP_PATH_CONTROL_MESSAGE = 0x0002,   ///< Path Control message.
	TJP_POSITION_MESSAGE = 0x0003,       ///< Position Message
	TJP_PROFILE_MESSAGE = 0x0004,        ///< Profile Message.
	TJP_PROFILE_CONTROL_MESSAGE = 0x0005,///< Profile Control Message.
	TJP_EHR_STATUS_MESSAGE = 0x0006,     ///< EHR Status message.
	TJP_PROBE_MESSAGE = 0x0007,          ///< Probe Message.
}
tjp_operationId_t;
/*Supported Service ID*/
typedef enum
{
	REMOTE_MONITORING_SYSTEM   = 0x00A1u,  ///< RMS Measurement Assignment (MA).
	FACIAL_RECOGNITION         = 0x00A5u,  ///< Face Recognition
	REMOTE_PARKING_ASSIST      = 0x00A7u,  ///< Remote parking by APP in cell phone
	TRAFFIC_JAM_PILOT          = 0x00AEu,  ///< High Definition map
	IP_LINK_MANAGER            = 0xFFFFu ///< IP Link Manager.
}
ipcp_serviceId_t;
/*Compare types*/
typedef enum
{
	TX = 0u,
	RX = 1u
}IPCP_TYPE;

/*Operation Type*/
typedef enum
{
	REQUEST = 0u,
	SETREQUEST_NORETURN,
	SETREQUEST,
	NOTIFICATION_REQUEST,
	RESPONSE,
	NOTIFICATION,
	NOTIFICATION_CYCLIC,
	ACK = 0x70,
	ERROR = 0xE0
} IPCP_OpType;

/*Error Codes*/
typedef enum
{
	NotOk,
	ServiceIDNotAvailable,
	OperationIDNotAvailable,
	OperationTypeNotAvailable,
	InvalidProtocolVersion,
	Processing,
	InvalidLength,
	ApplicationError,
	Timeout,
	Busy
}IPCP_ErrorCodes;

/* States of IPCP Retransmission mechanism */
typedef enum
{
	Empty = 0,
	REQSentStartWFA,
	ACKReceivedStartWFR,
	REQReceivedSendACK,
	RESPSentStartWFA
}IPCP_states;

/*IP Command Protocol Socket Configuration Structure */
typedef struct
{
	SOCKET sock;
	SOCKADDR_IN Client;
	SOCKADDR_IN Server;
	socklen_t slen;
	mqd_t ipcp_adasis_tx;
	mqd_t ipcp_adasis_rx;
	pthread_mutex_t ipcpMutex;
	int16_t ipcp_TxPktCount;
}IPCP_SockCfg;

/*IP Command Protocol Header Structure */
typedef struct
{
	uint16_t serviceID; 	/*to be provided by Application*/
	uint16_t operationID; /*to be provided by Application*/
	uint32_t length;
	uint32_t SenderHandleID; /*to be provided by Application*/
	uint8_t  ProtocolVer;
	uint8_t  operationType;  /*to be provided by Application*/
	uint8_t  datatype;
#if __BTYE_ORDER == __LITTLE_ENDIAN
	uint8_t  proc:1;
	uint8_t  reserved:7;
#elif __BYTE_ORDER == __BIG_ENDIAN
	uint8_t  reserved:7;
	uint8_t  proc:1;
#endif
}IPCP_Hdr;

/*------------------------------------------------------------------------------------------------*/
/**
 * Standard result codes.
 *
 * @note All error codes are negative integers. They allow functions with signed
 *       integers to return non-negative values when successful or standard error codes on failure.
 * @TODO Define more required error cases and Remove not required error cases.
 * @TODO To move to common header file.
 */
/*------------------------------------------------------------------------------------------------*/
typedef enum
{
	IPCP_OK = 0,                      /* Successful. */
	IPCP_FAULT = -1,                  /* Unspecified internal error. */
	IPCP_INVALID_SENDERHANDLEID = -2, /* Invalid SenderHandleId. */
	IPCP_INVALID_HEADER = -3,         /* Invalid IPCP header. */
	IPCP_NO_MEMORY = -4,              /* Insufficient memory is available. */
	IPCP_NOT_PERMITTED = -5,          /* Does not have permission to perform requested action. */
	IPCP_NOT_FOUND = -6,              /* Referenced item does not exist or could not be found. */
	IPCP_COMM_ERROR = -7,             /* Communications error.*/
	IPCP_TIMEOUT = -8,                /* A time-out occurred. */
	IPCP_OVERFLOW = -9,               /* An overflow occurred or would have occurred. */
	IPCP_UNDERFLOW = -10,             /* An underflow occurred or would have occurred. */
	IPCP_WOULD_BLOCK = -11,           /* Would have blocked if non-blocking behaviour was not requested.*/
	IPCP_DEADLOCK = -12,              /* Would have caused a deadlock. */
	IPCP_FORMAT_ERROR = -13,          /* Format error. */
	IPCP_DUPLICATE = -14,             /* Duplicate entry found or operation already performed. */
	IPCP_BAD_PARAMETER = -15,         /* Parameter is invalid. */
	IPCP_CLOSED = -16,                /* The resource is closed. */
	IPCP_BUSY = -17,                  /* The resource is busy. */
	IPCP_UNSUPPORTED = -18,           /* The underlying resource does not support this operation.*/
	IPCP_IO_ERROR = -19,              /* An IO operation failed. */
	IPCP_NOT_IMPLEMENTED = -20,       /* Unimplemented functionality. */
	IPCP_UNAVAILABLE = -21,           /* A transient or temporary loss of a service or resource. */
	IPCP_TERMINATED = -22,            /* The process, operation, data stream, session, etc. has stopped. */
	IPCP_OUT_OF_RANGE = -23,          /* An index or other value is out of range. */
	IPCP_DATACORRUPTED_ON_TX = -24,   /* Data is corrupted on transmission channel. */
	IPCP_APPLICATION_ERROR = -25,     /* Upper layer related error */
	IPCP_SOCKETERROR = -26,		  /* unable to created socket file*/
	IPCP_BINDERROR = -27,		  /* bind error */
	IPCP_SETSOCKOPTERROR = -28,       /* setsockopt failure */
	IPCP_MQ_OPEN_FAILED = -29,        /* mq_open failed */
	IPCP_MUTEX_ERROR = -30,           /* mutex error */
    IPCP_FAULTY_PAYLOAD_ERROR = -31,   /* Faulty payload error received from server */
	IPCP_MSGQ_SEND_FAILURE	= -32
}
ipcp_result_t;


/*IPCP Pkt Strucuture*/
typedef struct
{
	IPCP_Hdr hdr;
	uint8_t payLoad[IPCP_MAX_PAYLOAD_LEN];
}IPCP_Pkt;

typedef struct  __attribute__((__packed__))
{
	float32_t  IPCP_WFA_prev_timestamp;  /* Previous ACK timestamp of the packet sent */
	float32_t  IPCP_WFR_prev_timestamp;  /* Previous Response timestamp of the packet sent */
	float64_t IPCP_WFA_cur_starttime;   /* Present ACK timestamp of the packet */
	float64_t IPCP_WFR_cur_starttime;   /* Present Response timestamp of the packet */
	uint8_t   IPCP_Resp_RetryCnt;       /* Present Response Retry count */
	uint8_t   IPCP_Ack_RetryCnt;        /* Present ACK retry count */
	uint16_t  IPCP_Plen;
}IPCP_RTT_Timer;

typedef enum
{
	SUCCESS = 0,
	IPCP_REXMT_EXPIRED,
	IPCP_REXMT_TIMEOUT,
	IPCP_REXMT
}RTT_State;

/* IPCP Pkt Configuration structure */
typedef struct
{
	IPCP_RTT_Timer IPCP_Timer; /* Packet timeout and retransmission counter of any sent request */
	IPCP_Pkt ipcpPkt;
	IPCP_states state;
	uint32_t pktLen;
}IPCP_Pkt_Cfg;
/*==================[external function declarations]========================*/
void CloseConnection(IPCP_SockCfg *pCfg);
int32_t IPCP_Init(IPCP_SockCfg *pCfg);
void *IPCP_ReceiveTask(void *pCfg);
void *IPCP_TransmitTask(void *pCfg);
int32_t IPCP_Send(IPCP_SockCfg *pCfg, IPCP_Pkt *pIpcpHdr, uint32_t len);
void IPCP_add_time_stamp(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg, uint16_t plen);
void IPCP_add_time_stamp_WFR(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
void IPCP_update_time_stamp(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
int16_t IPCP_Rexmt_pkt(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
void IPCP_modify_timestamp_WFR_retry (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
void IPCP_modify_timestamp_WFA_retry (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
int16_t IPCP_is_WFA_timeout (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
int16_t IPCP_is_WFR_timeout (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg);
float64_t IPCP_time_stamp(void);
/*==================[external data]=========================================*/
/*==================[end of file]===========================================*/
