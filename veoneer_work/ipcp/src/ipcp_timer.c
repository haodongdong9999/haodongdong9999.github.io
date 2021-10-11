/**
 * \file :       ipcp_timer.c
 *
 * \brief : This file contains  IPCP timer,  updating timer and packet retransmitting functionalities.
 *
 * See @ref : ipcp_timer.h for general timer code and description.
 *
 * \author $Author: Lingaraj Marapli (lingaraju.hanumanthappa.external@veoneer.com) $
 *
 * Revision :    $Revision: 1.1 $
 * Created on :  $Date: 2018/15/12 12:56:35 IST $ 
 *
 * <br>=====================================================<br>
 * <b>Copyright 2019 by Veoneer India Pvt Ltd.</b>
 *
 *  All rights reserved. Property of Veoneer India Pvt Ltd.<br>
 *  Restricted rights to use, duplicate or disclose of this code<br>
 *  are granted through contract.
 * <br>=====================================================<br>
 */

#include "ipcp.h"

/*****************************************************************************
   FUNCTION NAME:       IPCP_add_time_stamp_WFA
   PURPOSE:
   RETURN VALUE:        -
   INPUT PARAMETERS:    Index to send packet
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
void IPCP_add_time_stamp(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg, uint16_t plen)
{
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_prev_timestamp = IPCP_DEFAULT_TIMEOUT_WFA;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_prev_timestamp = IPCP_DEFAULT_TIMEOUT_WFR;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime = IPCP_time_stamp() + IPCP_DEFAULT_TIMEOUT_WFA;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime = 0;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Resp_RetryCnt = 0;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt  = 0;
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Plen = plen;
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_add_time_stamp_WFR
   PURPOSE:
   RETURN VALUE:        -
   INPUT PARAMETERS:    Index to send packet
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
void IPCP_add_time_stamp_WFR(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg)
{
	pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime = IPCP_time_stamp();
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_time_stamp
   PURPOSE:             Compute the current time and convert to millisecs
   RETURN VALUE:        - current time in millisecs
   INPUT PARAMETERS:    
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli 
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
float64_t IPCP_time_stamp()
{
	float64_t IPCP_time_in_millisecs = 0;
	IPCP_Timer IPCP_start;
	memset (&IPCP_start, 0x00, sizeof(IPCP_Timer));
	clock_gettime(CLOCK_REALTIME, &IPCP_start);
	IPCP_time_in_millisecs =(float64_t) (( IPCP_start.tv_sec * 1000 )  + (float64_t)( IPCP_start.tv_nsec / 1000000.0));
	return ( IPCP_time_in_millisecs );
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_update_time_stamp
   PURPOSE:             Update the timestamp if timeout occurs.
   RETURN VALUE:        - 
   INPUT PARAMETERS:    
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli 
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
void IPCP_update_time_stamp(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg)
{

	float64_t  IPCP_time_in_millisecs = 0;
	if(pIPCP_Pkt_Cfg->state != Empty)
	{
		if (  (REQSentStartWFA == pIPCP_Pkt_Cfg->state) || ( RESPSentStartWFA == pIPCP_Pkt_Cfg->state) )
		{
			IPCP_time_in_millisecs = IPCP_time_stamp();
			if ( (pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime - IPCP_time_in_millisecs) <= 0)  
				pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime = 0;
		}
		else if ( ACKReceivedStartWFR == pIPCP_Pkt_Cfg->state)
		{
			IPCP_time_in_millisecs = IPCP_time_stamp();
			if ((pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime - IPCP_time_in_millisecs)  <= 0)
				pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime = 0;//IPCP_REXMT_TIMEOUT;
		}
	}
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_Rexmt_pkt
   PURPOSE:             Resend the packet which ever timeout occured.
   RETURN VALUE:        - 
   INPUT PARAMETERS:    Index to packet 
   OUTPUT PARAMETERS:   -
   ORIGINATOR:          - Lingaraj Marapli 
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
int16_t IPCP_Rexmt_pkt(IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg)
{
	if(pIPCP_Pkt_Cfg->state != Empty)
	{
		if (  REQSentStartWFA == pIPCP_Pkt_Cfg->state)
		{
			if( IPCP_is_WFA_timeout(pIPCP_Pkt_Cfg) )
			{	//TBD
				//update to app about timeout
				//Clean sent buffer
				pr_debug("Exceeded the packet retries\n" );
				//	IPCP_TxPktCfg[idx].state = Empty;
				return IPCP_REXMT_EXPIRED;

			}
			if( ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime <= (float64_t) 0.0 ) && ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt <= IPCP_MAX_RETRY_CNT_ACK) )
			{
				pr_debug ( "Retransmitting the request\n" );
				IPCP_modify_timestamp_WFA_retry(pIPCP_Pkt_Cfg);
				return IPCP_REXMT;//Re send packet    TBD
			}
		}
		else if ( RESPSentStartWFA == pIPCP_Pkt_Cfg->state)
		{
			if (IPCP_is_WFR_timeout(pIPCP_Pkt_Cfg) )
			{
				pr_debug ( "Exceeded the response timeout\n" );
				return IPCP_REXMT_EXPIRED;
			}
			else if( ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime <=  0 ) && ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Resp_RetryCnt < IPCP_MAX_RETRY_CNT_RES) )
			{
				pr_debug ("Timeout for Resp= %f ms", pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_prev_timestamp );
				pr_debug (",  Retry count = %d\n",pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt);
				IPCP_modify_timestamp_WFR_retry(pIPCP_Pkt_Cfg);
				pr_debug ( "Retransmitting the request\n" );
				return IPCP_REXMT;
				//Resend packet TBD
			}
		}

	}
	return SUCCESS;
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_modify_timestamp_WFR_retry
   PURPOSE:             Update the Response timeout and retry counter for every packet.
   RETURN VALUE:        - 
   INPUT PARAMETERS:    Index to packet 
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli 
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
void IPCP_modify_timestamp_WFR_retry (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg )
{
	float64_t  IPCP_time_in_millisecs = 0;
	if(pIPCP_Pkt_Cfg->state != Empty)
	{
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Resp_RetryCnt++;
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_prev_timestamp *= IPCP_DEFAULT_TIMEOUT_WFR;
		IPCP_time_in_millisecs = IPCP_time_stamp();
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime =(float64_t) IPCP_time_in_millisecs + (float64_t) pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_prev_timestamp;
	}
	return;
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_modify_timestamp_WFA_retry
   PURPOSE:             Update the ACK timeout and retry counter for every packet.
   RETURN VALUE:        - 
   INPUT PARAMETERS:    Index to packet 
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli 
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
void IPCP_modify_timestamp_WFA_retry (IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg )
{
	float64_t IPCP_time_in_millisecs = 0;
	if(pIPCP_Pkt_Cfg->state != Empty)
	{
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt++;
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_prev_timestamp *= IPCP_INC_TIMER_VALUE_WFA;
		IPCP_time_in_millisecs = IPCP_time_stamp();
		pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime = (float64_t)IPCP_time_in_millisecs + (float64_t)pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_prev_timestamp;
	}
	pr_debug ("Timeout for ACK = %.3f ms", pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_prev_timestamp );
	pr_debug (",  Retry count = %d\n",pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt);
	return;
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_is_WFA_timeout
   PURPOSE:             Check timeout and maximum retry count for specific packet.
   RETURN VALUE:        -
   INPUT PARAMETERS:    Index to packet
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
int16_t  IPCP_is_WFA_timeout ( IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg )
{
	if ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFA_cur_starttime <=  (float64_t) 0.0 )
	{ 
		if ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Ack_RetryCnt > IPCP_MAX_RETRY_CNT_ACK )
		{
			return IPCP_REXMT_TIMEOUT;
		}
	}
	return SUCCESS; 
}

/*****************************************************************************
   FUNCTION NAME:       IPCP_is_WFR_timeout
   PURPOSE:             Check timeout and maximum retry count for specific packet.
   RETURN VALUE:        -
   INPUT PARAMETERS:    Index to packet
   OUTPUT PARAMETERS:   -

   ORIGINATOR:          - Lingaraj Marapli
   DATE OF CREATION:    - DEC  2018
 *****************************************************************************/
int16_t IPCP_is_WFR_timeout ( IPCP_Pkt_Cfg *pIPCP_Pkt_Cfg )
{
	if ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_WFR_cur_starttime <=  0 ) 
	{ 
		if ( pIPCP_Pkt_Cfg->IPCP_Timer.IPCP_Resp_RetryCnt > IPCP_MAX_RETRY_CNT_RES  )
		{
			return IPCP_REXMT_TIMEOUT;
		}
	}
	return SUCCESS;
}

