/**
 * \file        main.c
 *
 * \brief       Implementation of IPCP Process initialization
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
/*==================[internal constants]====================================*/
/*==================[internal macros]=======================================*/
/*==================[module identification]=================================*/
/*==================[internal type definitions]=============================*/
/*==================[internal function declarations]========================*/
int main(void)
{
	int32_t retval = IPCP_OK;
	pthread_t task[3] = {0};
	int8_t **ptask[3] = {NULL};
	IPCP_SockCfg IPCP_ClientCfg;

	/* Clear IPCP Client Configuration Structure */
	memset(&IPCP_ClientCfg, 0, sizeof(IPCP_ClientCfg));

	/*IPCP Initialization*/
	if((retval = IPCP_Init(&IPCP_ClientCfg)) == IPCP_OK)
	{
		/*Creating Receive Thread*/
		if(pthread_create(&task[0], NULL, IPCP_TransmitTask, (void *)&IPCP_ClientCfg) == 0)
		{
			/*Creating Receive Thread*/
			if(pthread_create(&task[1], NULL, IPCP_ReceiveTask, (void *)&IPCP_ClientCfg) == 0)
			{
				pthread_join(task[1], (void **)ptask[1]);
				pthread_join(task[0], (void **)ptask[0]);
				while(ENDLESS_LOOP);
			}
		}
	}
	else
	{
		switch(retval)
		{
		case IPCP_SOCKETERROR:
			close(IPCP_ClientCfg.sock);
			break;
		case IPCP_MQ_OPEN_FAILED:
			close(IPCP_ClientCfg.sock);
			mq_close(IPCP_ClientCfg.ipcp_adasis_tx);
			break;
		case IPCP_MUTEX_ERROR:
			CloseConnection(&IPCP_ClientCfg);
			break;
		default:
			break;
		}
	}
}

/*==================[end of file]===========================================*/

/*****************************************************************************
ARCHIVENAME:       $Source: main.c $
VERSION NUMBER:    $Revision: 1.1 $
EDITOR NAME:       $Author: Sunil Kumar Nagaborappa (sunilkumar.nagaborappa) $
DATE OF LAST EDIT: $Date: 2019/01/06 22:21:35IST $

CHANGE:  $Log: main.c  $
CHANGE:  Revision 1.1 2019/01/06 22:21:35IST Sunil Kumar Nagaborappa (sunilkumar.nagaborappa)
CHANGE:  Initial Revision
 *****************************************************************************/
