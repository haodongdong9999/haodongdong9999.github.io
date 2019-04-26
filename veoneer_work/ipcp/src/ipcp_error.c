/**
 * \file        errorhandling.c
 *
 * \brief Validates the Ipcp header and payload for any error.
 * Describes the error code and error description.
 *
 * See @ref errorhandling.c for general error code and error description.
 *
 * \author $Author: Kiran Hemmadi (Kiran.shankara-poojary@veoneer.com) $
 *
 * Revision:    $Revision: 1.1 $
 * Created on:  $Date: 2018/11/27 22:56:35IST $
 *
 * <br>=====================================================<br>
 * <b>Copyright 2012 by Veoneer India Pvt Ltd.</b>
 *
 *  All rights reserved. Property of Veoneer India Pvt Ltd.<br>
 *  Restricted rights to use, duplicate or disclose of this code<br>
 *  are granted through contract.
 * <br>=====================================================<br>
 */

#include "ipcp.h"
#include "ipcp_error.h"


/*------------------------------------------------------------------------------------------------*/
/* Symbol and Enum definitions.
 *------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------*/
/**
 * Define the length which includes SenderHandleId
 */
/*------------------------------------------------------------------------------------------------*/
#define LENGTH_VALID_SENDERHANDLEID    4

/*------------------------------------------------------------------------------------------------*/
/**
 * Error message length
 */
/*------------------------------------------------------------------------------------------------*/
#define IPCP_ERROR_MSG_SIZE    8 + sizeof(ipcp_error_msg_payload_t)


/*------------------------------------------------------------------------------------------------*/
/* Data structures.
 *------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/* Static declarations.
 *------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/**
 * Supported service Ids.
 * @TODO Real info to come from binded services or local configuration file.
 */
/*------------------------------------------------------------------------------------------------*/
static uint16_t ServiceId[SERVICE_ID_MAX_NUM] = {
		TRAFFIC_JAM_PILOT,
		IP_LINK_MANAGER
};


/*------------------------------------------------------------------------------------------------*/
/**
 * Supported operation Ids.
 * @TODO Real info to come from binded servicess or local configuration file.
 *
 */
/*------------------------------------------------------------------------------------------------*/
static uint16_t OperationId[SERVICE_ID_MAX_NUM][OPERATION_ID_MAX_NUM] = {
		/* Operation Ids supported by service TRAFFIC_JAM_PILOT */
		{
				TJP_GLOBAL_DATA_MESSAGE,
				TJP_PATH_CONTROL_MESSAGE,
				TJP_POSITION_MESSAGE,
				TJP_PROFILE_MESSAGE,
				TJP_PROFILE_CONTROL_MESSAGE,
				TJP_EHR_STATUS_MESSAGE,
				TJP_PROBE_MESSAGE
		},
		/* Operation Ids supported by service IPLM */
		{
				IPLM_IP_ACTIVITY
		}
};


/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the SenderHandle Id.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_SENDERHANDLEID on length < 4 bytes, Discard the PDU.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateSenderHandleId
(
		const IPCP_Hdr *hdr   /* [IN] Ipcp Header */
)
{
	ipcp_result_t result = IPCP_OK;

	if (LENGTH_VALID_SENDERHANDLEID > hdr->length)
	{
		pr_debug("WARN: Invalid SenderHandle Id, Default action : Discard packet\n");
		result = IPCP_INVALID_SENDERHANDLEID;
	}

	return result;
}


/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the PDU length.
 *
 * @TODO To handle if payload size is not matching with length field value.
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_HEADER on invalid length, send the error message.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateLength
(
		const IPCP_Hdr *hdr,              /* [IN] Ipcp Header */
		size_t payloadSize,               /* [IN] Payload size */
		ipcp_error_msg_payload_t *errPl   /* [OUT] Error msg payload */
)
{
	ipcp_result_t result = IPCP_OK;

	if ((DEFAULT_IPCP_HDR_LENGTH > hdr->length) ||
			(payloadSize < (hdr->length - DEFAULT_IPCP_HDR_LENGTH)))
	{
		pr_debug("WARN: Invalid length, length: %u, payloadSize: %lu\n", hdr->length, payloadSize);
		errPl->errorCode = (uint8_t)IPCP_ERR_INVALID_LENGTH;
		errPl->errorInfo = (uint16_t)0x00;
		result = IPCP_INVALID_HEADER;
	}

	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the Protocol Version.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_HEADER on invalid protocol version.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateProtocolVersion
(
		const IPCP_Hdr *hdr,              /* [IN] Ipcp Header */
		ipcp_error_msg_payload_t *errPl   /* [OUT] Error msg payload */
)
{
	ipcp_result_t result = IPCP_OK;

	if (DEFAULT_PROTOCOL_VER != hdr->ProtocolVer)
	{
		pr_debug("WARN: Invalid Protocol Version %u\n", hdr->ProtocolVer);
		errPl->errorCode = (uint8_t)IPCP_ERR_INVALID_PROTOCOL_VER;
		errPl->errorInfo = (uint16_t)DEFAULT_PROTOCOL_VER;
		result = IPCP_INVALID_HEADER;
	}

	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Get the supported Service ID
 * @TODO: Get the Supported Service Ids from Application specific configuration file.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_FAULT on error.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t GetSupportedServiceId
(
		uint16_t *buf,        /* [OUT] Buffer to store Service Id */
		uint32_t *numElements  /* [IN/OUT] Number of elements */
)
{
	ipcp_result_t result = IPCP_OK;

	if (SERVICE_ID_MAX_NUM > *numElements)
	{
		result = IPCP_FAULT;
	}
	else
	{
		memcpy(buf, ServiceId, SERVICE_ID_MAX_NUM);
		*numElements = NUM_OF_ACTIVE_SERVICE;
		result = IPCP_OK;
	}

	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the Service Id.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_HEADER on invalid Service Id.
 *      - IPCP_FAULT o other failures.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateServiceId
(
		const IPCP_Hdr *hdr,            /* [IN] Ipcp Header */
		ipcp_error_msg_payload_t *errPl   /* [OUT] Error msg payload */
)
{
	ipcp_result_t result = IPCP_INVALID_HEADER;
	uint16_t serviceId = hdr->serviceID;
	uint16_t buf[SERVICE_ID_MAX_NUM] = {0};
	uint32_t numEle = SERVICE_ID_MAX_NUM;

	if (IPCP_OK != GetSupportedServiceId(buf, &numEle))
	{
		pr_debug("ERROR: Not able to fetch supported Service Ids\n");
		return IPCP_FAULT;
	}

	for (int i=0; i < numEle; i++)
	{
		if(buf[i] == serviceId)
		{
			result = IPCP_OK;
			break;
		}
	}

	if (IPCP_OK != result)
	{
		pr_debug("WARN: Invalid Service Id %d\n",serviceId);
		errPl->errorCode = (uint8_t)IPCP_ERR_INVALID_SERVICEID;
		errPl->errorInfo = serviceId;
	}

	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Convert service Id to corresponding array index.
 * @TODO: To implement as per configuration file or binded service.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_BAD_PARAMETER on bad parameter.
 *      - IPCP_FAULT on error.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ConvertServiceIdToIndex
(
		uint16_t serviceId,    /* [IN] Service Id */
		uint32_t *index       /* [OUT] index */
)
{
	ipcp_result_t result = IPCP_OK;
	switch (serviceId)
	{
	case TRAFFIC_JAM_PILOT:
		*index = IPCP_INDEX_TRAFFIC_JAM_PILOT;
		break;

	case IP_LINK_MANAGER:
		*index = IPCP_INDEX_IP_LINK_MANAGER;
		break;
	default:
		result = IPCP_FAULT;
		break;
	}
	return result;
}


/*------------------------------------------------------------------------------------------------*/
/**
 * Get the supported Operation ID
 * @TODO: Get the Supported Operation Ids from Application specific configuration file.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_FAULT on error.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t GetSupportedOperationId
(
		uint16_t serviceId,     /* [IN] Service Id */
		uint16_t *buf,         /* [OUT] Buffer to store Service Id */
		uint32_t *numElements  /* [IN/OUT] Number of elements */
)
{
	ipcp_result_t result = IPCP_OK;
	uint32_t index;

	if (OPERATION_ID_MAX_NUM > *numElements)
	{
		result = IPCP_FAULT;
	}
	else
	{
		ConvertServiceIdToIndex(serviceId, &index);
		*numElements = TJP_NUM_OF_OPERATION_ID; /* Hardcoded for TJP, get the number from funtion*/
		memcpy(buf, &OperationId[index][0], sizeof(OperationId)/SERVICE_ID_MAX_NUM);
		result = IPCP_OK;
	}
	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the Operation Id.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_HEADER on invalid Operation Id.
 *      - IPCP_FAULT o other failures.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateOperationId
(
		const IPCP_Hdr *hdr,            /* [IN] Ipcp Header */
		ipcp_error_msg_payload_t *errPl   /* [OUT] Error msg payload */
)
{
	ipcp_result_t result = IPCP_INVALID_HEADER;
	uint16_t operationId = hdr->operationID;
	uint16_t buf[OPERATION_ID_MAX_NUM] = {0};
	uint32_t numEle = OPERATION_ID_MAX_NUM;

	if (IPCP_OK != GetSupportedOperationId(hdr->serviceID,buf, &numEle))
	{
		pr_debug("ERROR: Not able to fetch supported Operation Ids\n");
		return IPCP_FAULT;
	}

	for (int i=0; i < numEle; i++)
	{
		if (buf[i] == operationId)
		{
			result = IPCP_OK;
			break;
		}
	}

	if ( IPCP_OK != result)
	{
		pr_debug("WARN: Invalid Operation Id %u\n", operationId);
		errPl->errorCode = (uint8_t)IPCP_ERR_INVALID_OPERATIONID;
		errPl->errorInfo = operationId;
	}
	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Validate the Operation Type.
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_INVALID_HEADER on invalid Operation Id.
 *      - IPCP_FAULT on other failures.
 */
/*------------------------------------------------------------------------------------------------*/
static ipcp_result_t ValidateOperationType
(
		const IPCP_Hdr *hdr,            /* [IN] Ipcp Header */
		ipcp_error_msg_payload_t *errPl   /* [OUT] Error msg payload */
)
{
	ipcp_result_t result = IPCP_OK;
	IPCP_OpType opType = (IPCP_OpType)hdr->operationType;

	switch(opType)
	{
	case REQUEST:
	case SETREQUEST_NORETURN:
	case SETREQUEST:
	case NOTIFICATION_REQUEST:
	case RESPONSE:
	case NOTIFICATION:
	case NOTIFICATION_CYCLIC:
	case ACK:
	case ERROR:
	{
		result = IPCP_OK;
	}
	break;

	default:
	{
		pr_debug("WARN: Invalid Operation Type %u.\n", opType);
		errPl->errorCode = (uint8_t)IPCP_ERR_INVALID_OPERATIONTYPE;
		errPl->errorInfo = opType;
		result = IPCP_INVALID_HEADER;
	}
	break;
	}
	return result;
}


/*------------------------------------------------------------------------------------------------*/
/**
 * Verifies the header for errors and fills appropriate error code and information.
 * The same function can be used to validate message received from application. Pass argument errorMsg
 * as NULL as we are not interested on error code.
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
		const IPCP_Hdr *hdr,  /* [IN] Ipcp Header */
		size_t payloadSize,     /* [IN] Payload size */
		ipcp_error_msg_t *errorMsg   /* [OUT] Error message */
)
{
	ipcp_result_t result = IPCP_OK;
	ipcp_error_msg_payload_t errPl;

	if (NULL == hdr)
	{
		return IPCP_BAD_PARAMETER;
	}

	memset(&errPl, 0, sizeof(ipcp_error_msg_payload_t));

	if (IPCP_OK != (result = ValidateSenderHandleId(hdr)));
	else if (IPCP_OK != (result = ValidateProtocolVersion(hdr, &errPl)));
	else if (IPCP_OK != (result = ValidateServiceId(hdr, &errPl)));
	else if (IPCP_OK != (result = ValidateOperationId(hdr, &errPl)));
	else if (IPCP_OK != (result = ValidateOperationType(hdr, &errPl)));
	else if (IPCP_OK != (result = ValidateLength(hdr, payloadSize, &errPl)));

	if (IPCP_INVALID_HEADER == result && NULL != errorMsg)
	{
		memset(errorMsg, 0, sizeof(ipcp_error_msg_t));

		/* Fill the header info for error message, byte order to be taken care in message handling. */
		if (errPl.errorCode == (uint8_t)IPCP_ERR_INVALID_SERVICEID)
		{
			errorMsg->hdr.serviceID = (uint16_t)TRAFFIC_JAM_PILOT; /* Hardcoded for TJP service. TBD */
		}
		else if (errPl.errorCode == (uint8_t)IPCP_ERR_INVALID_OPERATIONID)
		{
			errorMsg->hdr.operationID = (uint16_t)TJP_EHR_STATUS_MESSAGE; /* Hardcoded for EHR Req operation Id. TBD */
		}
		else
		{
			errorMsg->hdr.serviceID = hdr->serviceID;
			errorMsg->hdr.operationID = hdr->operationID;
		}

		errorMsg->hdr.length = IPCP_ERROR_MSG_SIZE;
		errorMsg->hdr.SenderHandleID = (uint16_t)hdr->SenderHandleID;
		errorMsg->hdr.ProtocolVer = (uint8_t)DEFAULT_PROTOCOL_VER;
		errorMsg->hdr.operationType = (uint8_t) ERROR;
		errorMsg->hdr.datatype = (uint8_t)0x01;

		/* Don't convert errorpayload to network byte order, to be taken care in message handling. */
		errorMsg->errPayload.errorCode = errPl.errorCode;
		errorMsg->errPayload.errorInfo = errPl.errorInfo;
	}
	return result;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * Interprets the error message received for any data corruption during transmission.
 * This function should be used when error message received from server.
 * @note This is client only function. 
 *
 * @return
 *      - IPCP_OK on success.
 *      - IPCP_DATACORRUPTED_ON_TX on original data is not matching with data received on other end.
 *      - IPCP_BUSY on Busy error message from other end. Retransmit the original pkt after waiting for amount of time
 *      - IPCP_BAD_PARAMETER on bad arguments.
 *      - IPCP_FAULT on other failures.
 */
/*-----------------------------------------------------------------------------------------------*/
ipcp_result_t ipcp_analyzeErrorMsg
(
		const IPCP_Hdr *hdr,                   /* [IN] Ipcp Header */
		size_t payloadSize,                    /* [IN] Payload size */
		const ipcp_error_msg_payload_t *errPl  /* [IN] Error message */
)
{
	ipcp_result_t result = IPCP_OK;

	if (NULL == errPl)
	{
		return IPCP_BAD_PARAMETER;
	}

	pr_debug("INFO: Error message with error code %u is received.\n", errPl->errorCode);

	if (NULL != hdr)
	{
		switch (errPl->errorCode)
		{
		case IPCP_ERR_INVALID_PROTOCOL_VER:
			if (errPl->errorInfo != hdr->ProtocolVer)
			{
				result = IPCP_DATACORRUPTED_ON_TX;
			}
			break;

		case IPCP_ERR_INVALID_SERVICEID:
			if (errPl->errorInfo != hdr->serviceID)
			{
				result = IPCP_DATACORRUPTED_ON_TX;
			}
			break;

		case IPCP_ERR_INVALID_OPERATIONID:
			if (errPl->errorInfo != hdr->operationID)
			{
				result = IPCP_DATACORRUPTED_ON_TX;
			}
			break;

		case IPCP_ERR_INVALID_OPERATIONTYPE:
			if (errPl->errorInfo != hdr->operationType)
			{
				result = IPCP_DATACORRUPTED_ON_TX;
			}
			break;

		case IPCP_ERR_BUSY:
			result = IPCP_BUSY;
			break;
		case IPCP_ERR_INVALID_LENGTH:
			if (errPl->errorInfo != hdr->length)
			{
				result = IPCP_DATACORRUPTED_ON_TX;
			}
			break;
		default:
			pr_debug("WARN: Error message with error code %u is not handled.\n", errPl->errorCode);
			break;
		}
	}
	else if (IPCP_ERR_PARAMETER_INVALID == errPl->errorCode)
	{
		result = IPCP_FAULTY_PAYLOAD_ERROR;
	}

	return result;
}
