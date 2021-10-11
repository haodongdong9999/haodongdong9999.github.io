/**
 * \file        ipcp.c
 *
 * \brief       Implementation of IPCP Protocol with Receive and
 * Transmit Task, functionalities including Parsing, Processing of
 * IPCP Packets, ACK Handling and Socket, Message Queue
 * Fault Recovery mechanism
 *
 * \author $Author: Sunil Kumar Nagaborappa (sunilkumar.nagaborappa) $
 *
 * Revision:    $Revision: 1.1 $
 * Created on:  $Date: 2019/01/06 22:21:35IST $
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
#include "ipcp.h"
#include "ipcp_error.h"
/*==================[internal constants]====================================*/
/*==================[internal macros]=======================================*/
#define IPCP_RCV_TIMEOUT 50000u
/*==================[module identification]=================================*/
/*==================[internal type definitions]=============================*/
/*==================[internal function declarations]========================*/
STATIC int32_t IPCP_CmpHdr(IPCP_Hdr *req, IPCP_Pkt_Cfg *resp);
STATIC void convertIPCPHdrToNetworkByteOrder(IPCP_Hdr *pIpcpHdr);
STATIC int32_t SendACK(IPCP_SockCfg *pCfg, IPCP_Hdr *pIpcpHdr);
STATIC int32_t SendUDPPacket(IPCP_SockCfg *pCfg, IPCP_Pkt *pIpcpPkt, uint32_t pLen);
STATIC int32_t isIPCPBufAvailable(IPCP_TYPE mode);
STATIC void open_IPCP_adasis_MQueue(IPCP_SockCfg *pCfg, IPCP_TYPE type);
STATIC void ResetTxRxBuffers(void);
STATIC void ReInitialize_IPCP(IPCP_SockCfg *pCfg);
STATIC void convertPayloadToNetworkByteOrder(uint32_t *pbuf, uint16_t len);
STATIC int16_t AppMsgQSend(IPCP_SockCfg *pCfg, IPCP_Pkt *pkt, int32_t pktlen);
STATIC void IPCP_Check_timeout_Rexmt(IPCP_SockCfg *pCfg );
/*==================[internal data]=========================================*/
STATIC IPCP_Pkt_Cfg IPCP_RxPktCfg[MAX_NUM_RX_PKT];
STATIC IPCP_Pkt_Cfg IPCP_TxPktCfg[MAX_NUM_RX_PKT];
STATIC int8_t mQPath[2][30] = {"/ipcp_adasis_tx", "/ipcp_adasis_rx"};
/*==================[external function definitions]=========================*/
/*****************************************************************************
   FUNCTION NAME:       CloseConnection
   PURPOSE:				Terminate the socket associated with provided socket
   	   	   	   	   	   	   file descriptor and existing message queues
   RETURN VALUE:        -

   INPUT PARAMETERS:    - pCfg: pointer to IPCP Configuration structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
void CloseConnection(IPCP_SockCfg *pCfg)
{
	/*Close the socket and Message queue created*/
	close(pCfg->sock);
	mq_close(pCfg->ipcp_adasis_tx);
	pthread_mutex_destroy(&pCfg->ipcpMutex);
	pr_debug("Closing Socket and MQ\n");
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_Init
   PURPOSE:				Create Socket and Message Queues for IPCP Client
   RETURN VALUE:        -TRUE if success; FALSE otherwise
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP configuration structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
int32_t IPCP_Init(IPCP_SockCfg *pCfg)
{
	int32_t retval = IPCP_OK;
	struct mq_attr msg = {0};
	struct ifreq interface;
	if(pCfg != NULL)
	{
		memset(pCfg, 0, sizeof(IPCP_SockCfg));
		/*Create Socket*/
		if((pCfg->sock = socket(AF_INET , SOCK_DGRAM , 0 )) == SOCKET_ERROR)
		{
			pr_debug("socket failure %d\n",errno);
			retval  = IPCP_SOCKETERROR;
		}
		else
		{
			pr_debug("Socket initialized %d\n",pCfg->sock);

			/* Prepare the sockaddr_in structure*/
			pCfg->Client.sin_family = AF_INET;
			pCfg->Client.sin_addr.s_addr = inet_addr(ASDM_IPADDRESS);
			pCfg->Client.sin_port = htons( EHP_IHU_PORT );
			pCfg->slen = sizeof(struct sockaddr);

			/*Bind the Socket */
			if(bind(pCfg->sock, (SOCKADDR *)&pCfg->Client, sizeof(pCfg->Client)) == SOCKET_ERROR)
			{
				pr_debug("Bind error %d\n",errno);
				retval = IPCP_BINDERROR;
			}
			else
			{
				pr_debug("Bind successful \n");

				strncpy(interface.ifr_name, ASDM_IPCP_NAME,sizeof(ASDM_IPCP_NAME));

				if (setsockopt(pCfg->sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface,sizeof(interface)) < 0)
				{
					pr_debug("VLAN interface bind failed \n");
				}
				else
				{
					pr_debug("VLAN interface bind sucess \n");
				}

				/*Configure Message Queue parameters*/
				/*Retrieve existing MQ attributes */
				msg.mq_flags = 0;
				msg.mq_maxmsg = MAX_NUM_RX_PKT;
				msg.mq_msgsize = IPCP_PKT_MAX_LEN;

				/*Create and open Message queue*/
				pCfg->ipcp_adasis_tx = mq_open((char *)mQPath[0], O_RDONLY | O_CREAT , 0x0666, &msg);
				if( pCfg->ipcp_adasis_tx > 0)
				{
					pr_debug("ipcp_adasis_tx Created %d \n", pCfg->ipcp_adasis_tx);
					/*Create Mutex for Re-initialization routine. Mutex is used to avoid collision between UDP Transmit and Receive Failure*/
					if(pthread_mutex_init(&((IPCP_SockCfg *)pCfg)->ipcpMutex, NULL) < 0)
					{
						pr_debug("ipcpMutex failure %d\n",errno);
						retval = IPCP_MUTEX_ERROR;
					}
				}
				else
				{
					pr_debug("mq_open failure %d\n",errno);
					retval = IPCP_MQ_OPEN_FAILED;
				}
				/*Configuring EHP IP configuration*/
				pCfg->Server.sin_port = htons(EHP_IHU_PORT);
				pCfg->Server.sin_family = AF_INET;
				pCfg->Server.sin_addr.s_addr = inet_addr(EHP_IHU_IP_ADDRESS);
			}
		}
	}
	else
	{
		retval = IPCP_BAD_PARAMETER;
	}

	return retval;
}

/*****************************************************************************
FUNCTION NAME:       IPCP_TransmitTask
PURPOSE:				Packetize and Send the received IPCP Data, Manage
						mq_receive failure and socket failure
RETURN VALUE:        -
INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure

OUTPUT PARAMETERS:   -

ORIGINATOR:          - Sunil Kumar Nagaborappa
DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
void *IPCP_TransmitTask(void *pCfg)
{
	int32_t retval = 0;
	int32_t idx = 0;
	int32_t txlen = 0;
	IPCP_Pkt ipcpPkt = {0};
	struct mq_attr msg;
	int16_t flag =0;

	msg.mq_flags = 0;
	msg.mq_maxmsg = MAX_NUM_RX_PKT;
	msg.mq_msgsize = IPCP_PKT_MAX_LEN;

	while(ENDLESS_LOOP)
	{
		retval = IPCP_OK;

		/*Wait for IPCP Message from Application*/
		if(((txlen = mq_receive(((IPCP_SockCfg *)pCfg)->ipcp_adasis_tx, (char *)&ipcpPkt, IPCP_PKT_MAX_LEN, NULL)) > 0))
		{
			/*If message is EHRStatus message*/
			if(ipcpPkt.hdr.operationID == TJP_EHR_STATUS_MESSAGE)
			{
				/*Open RX message queue*/
				open_IPCP_adasis_MQueue(pCfg, RX);
			}
			/*If RX message queue is opened successfully, and if IPCP buf is available*/
			if((((IPCP_SockCfg *)pCfg)->ipcp_adasis_rx > 0) && (((idx = isIPCPBufAvailable(TX)) >= 0)))
			{
				IPCP_TxPktCfg[idx].pktLen = txlen;
				memcpy(&IPCP_TxPktCfg[idx].ipcpPkt, &ipcpPkt, IPCP_TxPktCfg[idx].pktLen);

				switch(IPCP_TxPktCfg[idx].ipcpPkt.hdr.operationType)
				{
#if defined(NOT_USED_FOR_E2U2)
				case REQUEST:
				case SETREQUEST:
				case NOTIFICATION_REQUEST:
					IPCP_TxPktCfg[idx].state = REQSentStartWFA;
					IPCP_add_time_stamp(&IPCP_TxPktCfg[idx] , txlen );
					break;
				case RESPONSE:
				case NOTIFICATION:
					IPCP_TxPktCfg[idx].state = RESPSentStartWFA;
					/*Check if Notification Request was received*/
					if((retval = IPCP_CmpHdr(&IPCP_TxPktCfg[idx].ipcpPkt.hdr, &IPCP_RxPktCfg[0])) >= 0)
					{
						memset(&IPCP_RxPktCfg[retval], 0, sizeof(IPCP_RxPktCfg[0]));
					}
					IPCP_add_time_stamp(&IPCP_TxPktCfg[idx] , txlen );
					break;
#endif
				case SETREQUEST_NORETURN:
					IPCP_add_time_stamp(&IPCP_TxPktCfg[idx] , txlen );
					IPCP_TxPktCfg[idx].state = REQSentStartWFA;
					break;
#if defined(NOT_USED_FOR_E2U2)
				case ERROR:
				case NOTIFICATION_CYCLIC:
					break;
#endif
				default:
					pr_debug("Invalid Operation Type\n");
					memset(&IPCP_TxPktCfg[idx], 0, sizeof(IPCP_TxPktCfg[0]));
					retval = IPCP_UNSUPPORTED;
					break;
				}
				if(retval != IPCP_UNSUPPORTED)
				{
					/*convert IPCP Packet to network byte order*/
#if __BTYE_ORDER == __LITTLE_ENDIAN
					convertIPCPHdrToNetworkByteOrder(&IPCP_TxPktCfg[idx].ipcpPkt.hdr);
					if(ipcpPkt.hdr.length > DEFAULT_IPCP_HDR_LENGTH && ipcpPkt.hdr.length <= IPCP_PKT_MAX_LEN)
					{
						convertPayloadToNetworkByteOrder((uint32_t *)&IPCP_TxPktCfg[idx].ipcpPkt.payLoad[0], (ipcpPkt.hdr.length - DEFAULT_IPCP_HDR_LENGTH));
					}
#endif
					/*Increment the Tx Packet counter*/
					((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount++;
					if(SendUDPPacket(pCfg, &IPCP_TxPktCfg[idx].ipcpPkt, txlen) < 0)
					{
						pr_debug("sendto failure %d\n",errno);
						/*If sendto is failed then, ReInitialize the IPCP*/
						if(errno != EAGAIN)
						{
							ReInitialize_IPCP(pCfg);
						}
					}
				}
			}
		}
		else
		{
			/*If sendto is failed then, ReInitialize the IPCP*/
			if(errno == EBADF)
			{
				open_IPCP_adasis_MQueue(pCfg, TX);
			}
		}
		if(((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount > 0)
		{
			if(flag == IPCP_OK)
			{
				flag = 1;
				/*If there are any packet existing in the IPCP_TxPktCfg buffer. Make mq_receive nonblock to perform retransmission*/
				msg.mq_flags = O_NONBLOCK;
				mq_setattr(((IPCP_SockCfg *)pCfg)->ipcp_adasis_tx, &msg, NULL);\
			}
			/*100 milliseconds waiting for timeout*/
			usleep(100000);
			/*Handling Retranmission of existing IPCP Message*/
			IPCP_Check_timeout_Rexmt((IPCP_SockCfg *)pCfg);
		}
		else
		{
			flag = IPCP_OK;
			/*when there are no packets in IPCP_TxPktCfg buffer. make mq_receive blocking*/
			msg.mq_flags = 0;
			mq_setattr(((IPCP_SockCfg *)pCfg)->ipcp_adasis_tx, &msg, NULL);
		}
	}
	return NULL;
}

/*****************************************************************************|
 * STATIC FUNCTIONS
 *
 ****************************************************************************/
/*****************************************************************************
   FUNCTION NAME:       SendUDPPacket
   PURPOSE:				Send UDP packet
   RETURN VALUE:        -number of bytes sent if send sucess;
   	   	   	   	   	   	 Otherwise, -1
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
   	   	   	   	   	    -pIpcpPkt: Pointer Payload
   	   	   	   	   	    -pLen: length of Payload
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC int32_t SendUDPPacket(IPCP_SockCfg *pCfg, IPCP_Pkt *pIpcpPkt, uint32_t pLen)
{
	return(sendto(pCfg->sock, (void *)pIpcpPkt, pLen, 0, (SOCKADDR *)&pCfg->Server, pCfg->slen));
}
/*****************************************************************************
   FUNCTION NAME:       SendACK
   PURPOSE:				Packetize and Send Acknowledgement
   RETURN VALUE:        -number of bytes sent if send success;
   	   	   	   	   	   	 Otherwise, Error code
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
   	   	   	   	   	    -pIpcpHdr: Pointer to IPCP Header structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC int32_t SendACK(IPCP_SockCfg *pCfg, IPCP_Hdr *pIpcpHdr)
{
	int32_t retval = 0;
	IPCP_Hdr IpcpAckMsg = {0};

	/*Copy the Received message IPCP Header*/
	memcpy((int8_t *)&IpcpAckMsg, pIpcpHdr, IPCP_TOTAL_HDR_LENGTH);

	/*Configure the operation type to ACK and length of the packet*/
	IpcpAckMsg.operationType = ACK;
	IpcpAckMsg.length = DEFAULT_IPCP_HDR_LENGTH;

	/*Convert the data to Network byte order*/
#if __BTYE_ORDER == __LITTLE_ENDIAN
	convertIPCPHdrToNetworkByteOrder(&IpcpAckMsg);
#endif

	retval = SendUDPPacket(pCfg, (IPCP_Pkt *)&IpcpAckMsg, IPCP_TOTAL_HDR_LENGTH);
	if(retval < 0)
	{
		pr_debug("sendto failure\n");
	}
	return retval;
}

/*****************************************************************************
FUNCTION NAME:       IPCP_ReceiveTask
PURPOSE:			Receive & processes IPCP Messages from IHU, Manage mq_send,
					socket failure
RETURN VALUE:        -
INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
OUTPUT PARAMETERS:   -

ORIGINATOR:          - Sunil Kumar Nagaborappa
DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
void *IPCP_ReceiveTask(void *pCfg)
{
	int32_t retval = IPCP_OK;
	int32_t Rxlen = 0;
	uint32_t idx = 0;
	ipcp_error_msg_t errMsg = {0};
	ipcp_result_t result = IPCP_OK;

	while(ENDLESS_LOOP)
	{
		/*Receive IPCP packets from Server*/
		if(((idx = isIPCPBufAvailable(RX)) >= 0) &&
				(((Rxlen = recvfrom(
						((IPCP_SockCfg *)pCfg)->sock,
						(void *)&IPCP_RxPktCfg[idx].ipcpPkt,
						IPCP_PKT_MAX_LEN,
						0,
						(SOCKADDR *)&((IPCP_SockCfg *)pCfg)->Server,
						&(((IPCP_SockCfg *)pCfg)->slen)))) > 0) &&
						(((IPCP_SockCfg *)pCfg)->ipcp_adasis_rx > 0))
		{

			pr_debug("That's great ,we receive the vlan 6 message\n");
#if __BTYE_ORDER == __LITTLE_ENDIAN
			convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
#endif
			retval = ipcp_verifyHeader(&IPCP_RxPktCfg[idx].ipcpPkt.hdr, Rxlen, &errMsg);
			if(retval == IPCP_OK)
			{
				/*Payload should be converted to Little endian only if length is valid*/
#if __BTYE_ORDER == __LITTLE_ENDIAN
				convertPayloadToNetworkByteOrder((uint32_t *)&IPCP_RxPktCfg[idx].ipcpPkt.payLoad[0], (IPCP_RxPktCfg[idx].ipcpPkt.hdr.length - DEFAULT_IPCP_HDR_LENGTH));
#endif
				switch(IPCP_RxPktCfg[idx].ipcpPkt.hdr.operationType)
				{
#if defined(NOT_USED_FOR_E2U2)
				case REQUEST :
				case SETREQUEST :
				case NOTIFICATION_REQUEST:
					pr_debug("Received A REQUEST ");
					/*Compare Header if the packet already exists*/
					retval = IPCP_CmpHdr(&IPCP_RxPktCfg[idx].ipcpPkt.hdr, &IPCP_RxPktCfg[0]);
					if(retval < 0)
					{
						if(AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen) == IPCP_OK)
						{
							pr_debug("and Sending ACK\n");
							IPCP_RxPktCfg[idx].state = REQReceivedSendACK;
							SendACK((IPCP_SockCfg *)pCfg, &IPCP_RxPktCfg[idx].ipcpPkt.hdr);
						}
						else
						{
							memset((char *)&IPCP_RxPktCfg[idx], 0, sizeof(&IPCP_RxPktCfg[0]));
						}
					}
					break;
				case SETREQUEST_NORETURN:
					pr_debug("Received A SETREQUEST_NORETURN");
					if(AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen) == IPCP_OK)
					{
						pr_debug("and Sending ACK\n");
						SendACK((IPCP_SockCfg *)pCfg, &IPCP_RxPktCfg[idx].ipcpPkt.hdr);
					}
					memset((char *)&IPCP_RxPktCfg[idx], 0, sizeof(&IPCP_RxPktCfg[0]));
					break;
#endif
				case NOTIFICATION :
					pr_debug("Received A NOTIFICATION ");
					if(AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen) == IPCP_OK)
					{
						pr_debug("and Sending ACK\n");
						SendACK((IPCP_SockCfg *)pCfg, &IPCP_RxPktCfg[idx].ipcpPkt.hdr);
					}
					memset((char *)&IPCP_RxPktCfg[idx], 0, sizeof(&IPCP_RxPktCfg[0]));
					break;
#if defined(NOT_USED_FOR_E2U2)
				case RESPONSE:
					pr_debug("Received A RESPONSE");
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
					retval = IPCP_CmpHdr(&IPCP_RxPktCfg[idx].ipcpPkt.hdr, &IPCP_TxPktCfg[0]);
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
					if(retval >= 0 && retval <= MAX_NUM_RX_PKT)
					{
						IPCP_TxPktCfg[retval].state = Empty;
						/*Decrement the Tx Packet counter*/
						((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount--;
						memset(&IPCP_TxPktCfg[retval], 0, sizeof(IPCP_TxPktCfg[0]));
						if(AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen) == IPCP_OK)
						{
							pr_debug("and Sending ACK\n");
							SendACK((IPCP_SockCfg *)pCfg, &IPCP_RxPktCfg[idx].ipcpPkt.hdr);
						}
					}
					memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
					break;
#endif
				case ERROR:
					pr_debug("Received A IPCP_ERROR\n");
#if __BTYE_ORDER == __LITTLE_ENDIAN
					/*Converted back to Big Endian to compare with TxPkt Buffer which are in Big Endian*/
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
#endif
					retval = IPCP_CmpHdr(&IPCP_RxPktCfg[idx].ipcpPkt.hdr, &IPCP_TxPktCfg[0]);
#if __BTYE_ORDER == __LITTLE_ENDIAN
					/*Converted back to Little Endian for processing*/
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
#endif
					if(retval >= 0 && retval <= MAX_NUM_RX_PKT)
					{
						IPCP_TxPktCfg[retval].state = Empty;
						/*Decrement the Tx Packet counter*/
						((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount--;
						result = ipcp_analyzeErrorMsg(&IPCP_TxPktCfg[retval].ipcpPkt.hdr, sizeof(ipcp_error_msg_payload_t),
								(const ipcp_error_msg_payload_t *)&IPCP_RxPktCfg[idx].ipcpPkt.payLoad);
						(void)AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen);
						pr_debug("Clearing Request\n");
						memset(&IPCP_TxPktCfg[retval], 0, sizeof(IPCP_TxPktCfg[0]));
					}
					else
					{
						pr_debug("Received IPCP_ERROR pkt not matching any senderhandleId.\n");
						result = ipcp_analyzeErrorMsg(NULL, sizeof(ipcp_error_msg_payload_t),
								(const ipcp_error_msg_payload_t *)&IPCP_RxPktCfg[idx].ipcpPkt.payLoad);
						if (IPCP_FAULTY_PAYLOAD_ERROR == result)
						{
							if(AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen) == IPCP_OK)
							{
								pr_debug("INFO: Faulty Payload Error Received, Sending ACK\n");
								SendACK((IPCP_SockCfg *)pCfg, &IPCP_RxPktCfg[idx].ipcpPkt.hdr);
							}
						}
					}
					memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
					break;
				case ACK:
					pr_debug("Received A ACK");
#if __BTYE_ORDER == __LITTLE_ENDIAN
					/*Converted back to Big Endian to compare with TxPkt Buffer which are in Big Endian*/
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
#endif
					retval = IPCP_CmpHdr(&IPCP_RxPktCfg[idx].ipcpPkt.hdr, &IPCP_TxPktCfg[0]);
#if __BTYE_ORDER == __LITTLE_ENDIAN
					/*Converted back to Little Endian for processing*/
					convertIPCPHdrToNetworkByteOrder(&IPCP_RxPktCfg[idx].ipcpPkt.hdr);
#endif
					if(retval >= 0)
					{
						switch(IPCP_TxPktCfg[retval].ipcpPkt.hdr.operationType)
						{
						case RESPONSE:
						case SETREQUEST_NORETURN:
						case NOTIFICATION:
							IPCP_TxPktCfg[retval].state = Empty;
							/*Decrement the Tx Packet counter*/
							((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount--;
							memset(&IPCP_TxPktCfg[retval], 0, sizeof(IPCP_TxPktCfg[0]));
							pr_debug("Clearing the buffer\n");
							break;
						case REQUEST:
						case SETREQUEST:
						case NOTIFICATION_REQUEST:
							IPCP_TxPktCfg[retval].state = ACKReceivedStartWFR;
							break;
						}
					}
					memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
					break;
#if defined(NOT_USED_FOR_E2U2)
				case NOTIFICATION_CYCLIC:
					pr_debug("Received A NOTIFICATION_CYCLIC\n");
					(void)AppMsgQSend(((IPCP_SockCfg *)pCfg), &IPCP_RxPktCfg[idx].ipcpPkt, Rxlen);
					memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
					break;
#endif
				default:
					memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
					break;
				}
			}
			else
			{
				/*Header Related Error */
				pr_debug("WARN: Message received with invalid IPCP header\n");
#if defined(SERVER)
				/* For invalid senderhandle id, don't send error message */
				if (retval != IPCP_INVALID_SENDERHANDLEID)
				{
#if __BTYE_ORDER == __LITTLE_ENDIAN
					htons(errMsg.errPayload.errorInfo);
#endif
					SendUDPPacket((IPCP_SockCfg *)pCfg, (IPCP_Pkt *)&errMsg, IPCP_ERROR_PKT_LEN);
				}
#endif
				memset(&IPCP_RxPktCfg[idx], 0, sizeof(IPCP_RxPktCfg[0]));
			}
		}
		else
		{
			if((errno == EBADF) && (Rxlen < 0))
			{
				pr_debug("recvfrom failure %d\n",errno);
				ReInitialize_IPCP(pCfg);
			}
		}
	}
}

/*==================[internal function definitions]=========================*/
/*****************************************************************************
   FUNCTION NAME:       IPCP_CmpHdr
   PURPOSE:				Compare the IPCP Header parameters for duplication
   RETURN VALUE:        - Index of IPCP Buffer if the given req exists in
   	   	   	   	   	    resp buffer list. Otherwise, return -1
   INPUT PARAMETERS:    -req : Pointer to the IPCP Header
      	  	  	  	  	-resp: Pointer to the set of IPCP Pkt buffer
   OUTPUT PARAMETERS:
   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC int32_t IPCP_CmpHdr(IPCP_Hdr *req, IPCP_Pkt_Cfg *resp)
{
	int32_t retval = -1;
	uint32_t idx = 0;

	/*Loop in all the buffers to find the existing request*/
	for(idx = 0; idx<MAX_NUM_RX_PKT; idx++)
	{
		if((resp[idx].state != Empty) &&
				(req->SenderHandleID == resp[idx].ipcpPkt.hdr.SenderHandleID))
		{
			/*if existing message is same as searching message, loop for rest of the buffers*/
			if(req != &resp[idx].ipcpPkt.hdr)
			{
				retval = idx;
				break;
			}
		}
	}
	return retval;
}
/*****************************************************************************
   FUNCTION NAME:       isIPCPBufAvailable
   PURPOSE:				Return the idx of available buffer
   RETURN VALUE:        -Return the idx of available buffer
   INPUT PARAMETERS:    -mode: type of buffer to search
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC int32_t isIPCPBufAvailable(IPCP_TYPE mode)
{
	int32_t idx = IPCP_BUF_NOT_AVAILABLE;
	switch(mode)
	{
	case TX:
		for(idx = 0; idx<MAX_NUM_RX_PKT; idx++)
		{
			if(IPCP_TxPktCfg[idx].state == Empty)
			{
				break;
			}
		}
		break;
	case RX:
		for(idx = 0; idx<MAX_NUM_RX_PKT; idx++)
		{
			if(IPCP_RxPktCfg[idx].state == Empty)
			{
				break;
			}
		}
		break;
	default:
		break;
	}
	/*if all buffer are busy*/
	if(idx >= MAX_NUM_RX_PKT)
	{
		idx = IPCP_BUF_NOT_AVAILABLE;
	}
	return idx;
}

/*****************************************************************************
   FUNCTION NAME:       open_IPCP_adasis_MQueue
   PURPOSE:				open the ipcp_adasis Tx and Rx Queue
   RETURN VALUE:        -
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
   	   	   	   	   	    -type: IPCP Type, if TX or RX
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void open_IPCP_adasis_MQueue(IPCP_SockCfg *pCfg, IPCP_TYPE type)
{
	mqd_t *pMqPtr = NULL;
	int32_t flags = 0;
	switch(type)
	{
	case TX:
		pMqPtr = &pCfg->ipcp_adasis_tx;
		flags = (O_RDONLY);
		break;
	case RX:
		pMqPtr = &pCfg->ipcp_adasis_rx;
		flags = (O_WRONLY);
		break;
	default:
		break;
	}
	if(*pMqPtr <= 0)
	{
		/*Create and open Message queue*/
		*pMqPtr = mq_open((char *)mQPath[type], flags, 0x0666, NULL);
		if(*pMqPtr > 0)
		{
			pr_debug("mq_open success \n");
		}
		else
		{
			pr_debug("mq_open failure %d\n",errno);
		}
	}
}
/*****************************************************************************
   FUNCTION NAME:       convertIPCPHdrToNetworkByteOrder
   PURPOSE:				convert IPCP Header parameter to network byte order
   RETURN VALUE:        -
   INPUT PARAMETERS:    -pIpcpHdr:Pointer to IPCP Header
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void convertIPCPHdrToNetworkByteOrder(IPCP_Hdr *pIpcpHdr)
{
	/*Convert IPCP parameters from little endian to network byte order*/
	pIpcpHdr->SenderHandleID  = htonl(pIpcpHdr->SenderHandleID);
	pIpcpHdr->length = htonl(pIpcpHdr->length);
	pIpcpHdr->operationID = htons(pIpcpHdr->operationID);
	pIpcpHdr->serviceID = htons(pIpcpHdr->serviceID);
}
/*****************************************************************************
   FUNCTION NAME:       ReInitialize_IPCP
   PURPOSE:				Re Initialize the IPCP Protocol, Close socket and message
   	   	   	   	   	   	Queue and create new socket and message Queue
   RETURN VALUE:        -
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void ReInitialize_IPCP(IPCP_SockCfg *pCfg)
{
	if(pthread_mutex_trylock(&((IPCP_SockCfg *)pCfg)->ipcpMutex) == EOK)
	{
		/*Close existing connection and open new connection*/
		CloseConnection(pCfg);
		ResetTxRxBuffers();
		if(IPCP_Init(pCfg) <= 0)
		{
			pr_debug("Re Initialization failed\n");
		}
		else
		{
			pr_debug("Re Initialization successfull\n");
		}
		pthread_mutex_unlock(&((IPCP_SockCfg *)pCfg)->ipcpMutex);
	}
}
/*****************************************************************************
   FUNCTION NAME:       ResetTxRxBuffers
   PURPOSE:				convert IPCP Header parameter to network byte order
   RETURN VALUE:        -
   INPUT PARAMETERS:    -
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void ResetTxRxBuffers(void)
{
	/*Clear Rx and Tx Buffer*/
	memset(&IPCP_TxPktCfg[0], 0, (sizeof(IPCP_Pkt_Cfg) * MAX_NUM_RX_PKT));
	memset(&IPCP_RxPktCfg[0], 0, (sizeof(IPCP_Pkt_Cfg) * MAX_NUM_RX_PKT));
}
/*****************************************************************************
   FUNCTION NAME:       convertPayloadToNetworkByteOrder
   PURPOSE:				convert payload to network by order
   RETURN VALUE:        -
   INPUT PARAMETERS:    -pbuf:pointer to the buffer to be converted
   	   	   	   	   	   	-len: length of the buffer
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void convertPayloadToNetworkByteOrder(uint32_t *pbuf, uint16_t len)
{
	uint16_t idx;
	/*Convert all the Payload data to Network byte order*/
	for(idx = 0; idx < len; idx+=4)
	{
		pbuf[idx] = htonl(pbuf[idx]);
	}
}
/*****************************************************************************
   FUNCTION NAME:       AppMsgQSend
   PURPOSE:				Send IPCP packet to application, handle mq_send failure
   RETURN VALUE:        - IPCP_OK if success;
   	   	   	   	   	    - IPCP_MSGQ_SEND_FAILURE otherwise
   INPUT PARAMETERS:    -pCfg: pointer to IPCP Configuration Structure
   	   	   	   	   	    -pkt: pointer to IPCP Packet structure
   	   	   	   	   	    -pktlen: length of the IPCP Packet structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Sunil Kumar Nagaborappa
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC int16_t AppMsgQSend(IPCP_SockCfg *pCfg, IPCP_Pkt *pkt, int32_t pktlen)
{
	int32_t retval = IPCP_MSGQ_SEND_FAILURE;
	if((mq_send(pCfg->ipcp_adasis_rx, (char *)pkt, pktlen, 0) < 0) && (errno != EAGAIN))
	{
		pr_debug("AppMsgQSend failure %d\n", errno);
		if(errno == EBADF)
		{
			pCfg->ipcp_adasis_rx = 0;
			open_IPCP_adasis_MQueue(pCfg, RX);
			if((mq_send(pCfg->ipcp_adasis_rx, (char *)pkt, pktlen, 0) > 0) && (errno == EAGAIN))
			{
				retval = IPCP_OK;
			}
		}
	}
	else
	{
		retval = IPCP_OK;
	}
	return retval;
}
/*****************************************************************************
   FUNCTION NAME:       IPCP_Check_timeout_Rexmt
   PURPOSE:				retransmission module
   RETURN VALUE:        -
   INPUT PARAMETERS:    -pCfg:Pointer to IPCP Client socket structure
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli
   DATE OF CREATION:    - DEC 2018
 *****************************************************************************/
STATIC void IPCP_Check_timeout_Rexmt(IPCP_SockCfg *pCfg)
{
	uint16_t index = 0;
	int32_t retval = 0;

	// update timestamp and if any timeout retransmit the packet
	for (index  = 0; index < MAX_NUM_RX_PKT; index++ )
	{
		if ( IPCP_TxPktCfg[index].state != Empty )
		{
			IPCP_update_time_stamp(&IPCP_TxPktCfg[index]);
			retval = IPCP_Rexmt_pkt(&IPCP_TxPktCfg[index]);
			if (( retval == IPCP_REXMT_TIMEOUT)  || (retval == IPCP_REXMT))
			{
				if(SendUDPPacket((IPCP_SockCfg *)pCfg, &IPCP_TxPktCfg[index].ipcpPkt, IPCP_TxPktCfg[index].IPCP_Timer.IPCP_Plen) < 0)
				{
					pr_debug("sendto failure %d\n",errno);
					if(errno != EAGAIN)
					{
						ReInitialize_IPCP(pCfg);
					}
				}
			}
			else if ( retval == IPCP_REXMT_EXPIRED )
			{
				pr_debug( "Exceeded the packet retransmission counts\n" );
				memset ( &IPCP_TxPktCfg[index], 0x00, sizeof(IPCP_Pkt_Cfg));
				IPCP_TxPktCfg[index].state = Empty;
				((IPCP_SockCfg *)pCfg)->ipcp_TxPktCount--;
			}
		}
	}
}
/*==================[end of file]===========================================*/
/*****************************************************************************
ARCHIVENAME:       $Source: ipcp.c $
VERSION NUMBER:    $Revision: 1.1 $
EDITOR NAME:       $Author: Sunil Kumar Nagaborappa (sunilkumar.nagaborappa) $
DATE OF LAST EDIT: $Date: 2019/01/06 22:21:35IST $

CHANGE:  $Log: ipcp.c  $
CHANGE:  Revision 1.1 2019/01/06 22:21:35IST Sunil Kumar Nagaborappa (sunilkumar.nagaborappa)
CHANGE:  Initial Revision
 *****************************************************************************/

