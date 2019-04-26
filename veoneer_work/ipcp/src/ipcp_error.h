/**
 * \file        ipcp_error.h
 *
 * \brief       A brief description (one line).
 *
 * [Optional] A detailed description of the function, purpose
 * and usage of this file (more lines).
 *
 * \author $Author: Kiran Hemmadi (kiran.shankara-poojary@veoneer.com) $
 *
 * Revision:    $Revision: 1.1 $
 * Created on:  $Date: 2018/12/1 22:56:35IST $
 *
 * <br>=====================================================<br>
 * <b>Copyright 2012 by Veoneer India Pvt Ltd.</b>
 *
 *  All rights reserved. Property of Veoneer India Pvt Ltd.<br>
 *  Restricted rights to use, duplicate or disclose of this code<br>
 *  are granted through contract.
 * <br>=====================================================<br>
 */

#ifndef IPCP_ERROR_H
#define IPCP_ERROR_H

#ifdef  __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------------------------*/
/**
 * Maximum number of the Service Id defined.
 */
/*------------------------------------------------------------------------------------------------*/
#define SERVICE_ID_MAX_NUM    32   /* TBD */

/*------------------------------------------------------------------------------------------------*/
/**
 * Maximum number of the Operation Id defined for each service.
 */
/*------------------------------------------------------------------------------------------------*/
#define OPERATION_ID_MAX_NUM    8   /* TBD */

/*------------------------------------------------------------------------------------------------*/
/**
 * Number of Supported Operation Id for IPLM service.
 */
/*------------------------------------------------------------------------------------------------*/
#define IPLM_NUM_OF_OPERATION_ID  1

/*------------------------------------------------------------------------------------------------*/
/**
 * Number of Supported Operation Id for IPLM service.
 */
/*------------------------------------------------------------------------------------------------*/
#define TJP_NUM_OF_OPERATION_ID  7

/*------------------------------------------------------------------------------------------------*/
/**
 * Number of Active supported Services.
 */
/*------------------------------------------------------------------------------------------------*/
#define NUM_OF_ACTIVE_SERVICE  2


/*------------------------------------------------------------------------------------------------*/
/**
 * Supported Operation Id for IPLM service.
 */
/*------------------------------------------------------------------------------------------------*/
#define IPLM_IP_ACTIVITY  0xFF01

#define IPCP_ERROR_PAYLOAD_LEN	(3u)

#define IPCP_ERROR_PKT_LEN	(IPCP_TOTAL_HDR_LENGTH + IPCP_ERROR_PAYLOAD_LEN)

/*------------------------------------------------------------------------------------------------*/
/**
 * Equivalent index to service Id
 */
/*------------------------------------------------------------------------------------------------*/
typedef enum
{
    IPCP_INDEX_TRAFFIC_JAM_PILOT = 0,
    IPCP_INDEX_IP_LINK_MANAGER = 1
}
ipcp_serviceIdToIndex_t;


/*------------------------------------------------------------------------------------------------*/
/**
 * Error message payload
 */
/*------------------------------------------------------------------------------------------------*/
typedef struct
{
    uint8_t errorCode;    /* Ipcp error code */
    uint16_t errorInfo;   /* Ipcp error information */
}
ipcp_error_msg_payload_t;


/*------------------------------------------------------------------------------------------------*/
/**
 * Error message
 */
/*------------------------------------------------------------------------------------------------*/
typedef struct
{
    IPCP_Hdr hdr;   /* Ipcp header */
    ipcp_error_msg_payload_t errPayload; /* error payload */
}
ipcp_error_msg_t;


/*------------------------------------------------------------------------------------------------*/
/**
 * Ipcp error codes.
 */
/*------------------------------------------------------------------------------------------------*/
typedef enum
{
    IPCP_ERR_NOT_OK                = 0x00,  /* An unspecified error occured. */
    IPCP_ERR_INVALID_SERVICEID     = 0x01,  /* The Service is not available or does not exist. */
    IPCP_ERR_INVALID_OPERATIONID   = 0x02,  /* The Operation is not available or does not exist. */
    IPCP_ERR_INVALID_OPERATIONTYPE = 0x03,  /* Invalid Operation type. */
    IPCP_ERR_INVALID_PROTOCOL_VER  = 0x04,  /* Invalid Protocol version. */
    IPCP_ERR_PROCESSING            = 0x05,  /* Requested Operation id is already processing a previous request.*/
    IPCP_ERR_INVALID_LENGTH        = 0x06,  /* The length of the message is not correct. */
    IPCP_ERR_APPLICATION_ERROR     = 0x07,  /* Application not ready. */
    IPCP_ERR_TIMEOUT_ERROR         = 0x08,  /* Timeout occured. */
    IPCP_ERR_BUSY                  = 0x09,  /* Server is concurrently handling maximum number of messages.*/
    IPCP_ERR_INCORRECT_STATE       = 0x20,  /* Operation or OpType is not valid in current state.*/
    IPCP_ERR_ASN1_DECODE_ERROR     = 0x23,  /* ASN1 Could not be decoded.*/
    IPCP_ERR_PARAMETER_INVALID     = 0x24   /* Invalid parameters.*/
}
ipcp_errorCodes_t;



/*------------------------------------------------------------------------------------------------*/
/**
 * Verifies the header for errors
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_SENDERHANDLEID on invalid senderhandle Id, discard the Pkt, donot send the error msg.
 *      - IPCP_INVALID_HEADER on invalid header, send the error msg.
 *      - IPCP_BAD_PARAMETER on bad arguments.
 *      - IPCP_FAULT on other failures.
 */
/*------------------------------------------------------------------------------------------------*/
ipcp_result_t ipcp_verifyHeader
(
    const IPCP_Hdr *hdr,      /* [IN] Ipcp Header */
    size_t payloadSize,       /* [IN] Payload size */
    ipcp_error_msg_t *errorMsg   /* [OUT] Error message */
);

/*------------------------------------------------------------------------------------------------*/
/**
 * Interprets the error message received for any data corruption during transmission.
 * This function should be used only when error message received from server.
 * @note This is client only function. 
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_DATACORRUPTED_ON_TX on original data is not matching with data received on other end.
 *      - IPCP_BUSY on Busy error message from other end.
 *      - IPCP_FAULTY_PAYLOAD_ERROR on payload related error.
 *      - IPCP_BAD_PARAMETER on bad arguments.
 *      - IPCP_FAULT on other failures.
 */
/*------------------------------------------------------------------------------------------------*/
ipcp_result_t ipcp_analyzeErrorMsg
(
    const IPCP_Hdr *hdr,               /* [IN] Ipcp Header */
    size_t payloadSize,                     /* [IN] Payload size */
    const ipcp_error_msg_payload_t *errPl   /* [IN] Error message */
);

#endif  /* IPCP_ERROR_H */
