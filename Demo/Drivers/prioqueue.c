#include <queue.h>
#include "prioqueue.h"
#include "FreeRTOS.h"
#include "uart.h"
#include <stdlib.h>
#include <string.h>

typedef struct PrioElement_
{
	unsigned portCHAR ucPrio;
	void* vData;
} PrioElement;


typedef struct PrioQueue
{
	xQueueHandle xQueue;
	unsigned portBASE_TYPE uxItemSize;
	unsigned portCHAR ucHPrio;
	unsigned portCHAR ucLPrio;
	unsigned portBASE_TYPE uxElement;
	unsigned portBASE_TYPE uxMaxElement;
	
} xPRIOQUEUE;

typedef xPRIOQUEUE * xPrioQueueHandle;

xPrioQueueHandle xPrioQueueCreate(unsigned portBASE_TYPE uxQueueLength,unsigned portBASE_TYPE uxItemSize);

void vPrioQueueDelete( xPrioQueueHandle pxQueue );

portBASE_TYPE xPrioQueuePeekHigh( xPrioQueueHandle xQueue, void *pvBuffer, portTickType xTicksToWait );
portBASE_TYPE xPrioQueueReceiveHigh(xPrioQueueHandle pxPrioQueue,void *pvBuffer, portTickType xTicksToWait);
portBASE_TYPE xPrioQueueReceiveLow(xPrioQueueHandle pxPrioQueue,void *pvBuffer, portTickType xTicksToWait);
portBASE_TYPE xPrioQueueSend(xPrioQueueHandle pxPrioQueue, void * pvItemToQueue, unsigned portCHAR prio, portTickType xTicksToWait);
portBASE_TYPE xPrioQueueIsEmpty(xPrioQueueHandle pxPrioQueue);

portBASE_TYPE xPrioQueueReceiveHighFromISR(xPrioQueueHandle pxPrioQueue,void *pvBuffer,  signed portBASE_TYPE *pxHigherPriorityTaskWoken);
portBASE_TYPE xPrioQueueReceiveLowFromISR(xPrioQueueHandle pxPrioQueue,void *pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
portBASE_TYPE xPrioQueueSendFromISR(xPrioQueueHandle pxPrioQueue, void * pvItemToQueue, unsigned portCHAR prio,  signed portBASE_TYPE *pxHigherPriorityTaskWoken);
portBASE_TYPE xPrioQueueIsEmptyFromISR(xPrioQueueHandle pxPrioQueue);


xPrioQueueHandle xPrioQueueCreate(unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize)
{
	xPRIOQUEUE *pxNewPrioQueue;
	pxNewPrioQueue = ( xPRIOQUEUE * ) pvPortMalloc( sizeof( xPRIOQUEUE ) );
	pxNewPrioQueue->xQueue=xQueueCreate(uxQueueLength,sizeof(PrioElement));
	pxNewPrioQueue->uxItemSize = uxItemSize;
	pxNewPrioQueue->ucHPrio=0;
	pxNewPrioQueue->ucLPrio=0;
	pxNewPrioQueue->uxElement=0;
	pxNewPrioQueue->uxMaxElement=uxQueueLength;
	return pxNewPrioQueue;
}

void vPrioQueueDelete( xPrioQueueHandle pxPrioQueue )
{
	vQueueDelete( pxPrioQueue->xQueue );
	vPortFree(pxPrioQueue);
}

portBASE_TYPE xPrioQueuePeekHigh( xPrioQueueHandle pxPrioQueue, void *pvBuffer, portTickType xTicksToWait )
{
	if(pxPrioQueue->uxElement==0)
		return pdFALSE;

	portBASE_TYPE xReturn=pdPASS; //==pdTrue
	PrioElement xElement;
	int i;
	for(i=0; i<pxPrioQueue->uxElement;i++)
	{
		xReturn &= xQueueReceive( pxPrioQueue->xQueue, &xElement, xTicksToWait );
		xReturn &= xQueueSend(pxPrioQueue->xQueue,&xElement,xTicksToWait);
		if(xReturn==pdFALSE || (xElement.ucPrio == pxPrioQueue->ucHPrio))
		{
			break;
		}
	}
	if(xReturn == pdPASS)
	{
		memcpy( ( void * ) pvBuffer, ( void * ) xElement.vData, ( unsigned ) pxPrioQueue->uxItemSize );
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueReceiveHigh(xPrioQueueHandle pxPrioQueue,void *pvBuffer, portTickType xTicksToWait)
{
	if(pxPrioQueue->uxElement==0)
		return pdFALSE;

	portBASE_TYPE xReturn=pdPASS;//==pdTRUE
	PrioElement xElement;
	PrioElement xElementOut;
	portBASE_TYPE xFound=pdFALSE;
	unsigned portCHAR ucNextHigh=pxPrioQueue->ucLPrio;
	int i;
	for(i=0; i<pxPrioQueue->uxElement;i++)
	{
		xReturn &= xQueueReceive( pxPrioQueue->xQueue, &xElement, xTicksToWait );
		if(xReturn==pdFALSE)
			break;
		if(xFound==pdFALSE && xElement.ucPrio == pxPrioQueue->ucHPrio)
		{
			xElementOut=xElement;
			xFound=pdTRUE;
		}
		else
		{
			if(ucNextHigh<xElement.ucPrio)
				ucNextHigh=xElement.ucPrio;

			xReturn &= xQueueSend(pxPrioQueue->xQueue,&xElement,xTicksToWait);
			if(xReturn==pdFALSE)
				break;
		}
	}
	
	if(xReturn == pdPASS)
	{
		pxPrioQueue->ucHPrio=ucNextHigh;
		memcpy( ( void * ) pvBuffer, ( void * ) xElementOut.vData, ( unsigned ) pxPrioQueue->uxItemSize );
		pxPrioQueue->uxElement--;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueReceiveLow(xPrioQueueHandle pxPrioQueue,void *pvBuffer, portTickType xTicksToWait)
{
	if(pxPrioQueue->uxElement==0)
		return pdFALSE;

	portBASE_TYPE xReturn=pdPASS;
	PrioElement xElement;
	PrioElement xElementOut;
	portBASE_TYPE xFound=pdFALSE;
	unsigned portCHAR ucNextLow=pxPrioQueue->ucHPrio;
	int i;
	for(i=0; i<pxPrioQueue->uxElement;i++)
	{
		xReturn &= xQueueReceive( pxPrioQueue->xQueue, &xElement, xTicksToWait );
		if(xReturn==pdFALSE)
			break;
		if(xFound==pdFALSE && xElement.ucPrio == pxPrioQueue->ucLPrio)
		{
			xElementOut=xElement;
			xFound=pdTRUE;
		}
		else
		{
			if(ucNextLow>xElement.ucPrio)
				ucNextLow=xElement.ucPrio;

			xReturn &= xQueueSend(pxPrioQueue->xQueue,&xElement,xTicksToWait);
			if(xReturn==pdFALSE)
				break;
		}
	}
	
	if(xReturn == pdPASS)
	{
		pxPrioQueue->ucLPrio=ucNextLow;
		memcpy( ( void * ) pvBuffer, ( void * ) xElementOut.vData, ( unsigned ) pxPrioQueue->uxItemSize );
		pxPrioQueue->uxElement--;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueSend(xPrioQueueHandle pxPrioQueue, void * pvItemToQueue, unsigned portCHAR prio, portTickType xTicksToWait)
{
	if(pxPrioQueue->uxElement==pxPrioQueue->uxMaxElement)
		return pdFALSE;

	portBASE_TYPE xReturn;
	PrioElement xElement;
	
	//Element bauen
	xElement.ucPrio=prio;
	xElement.vData=pvItemToQueue;
	xReturn = xQueueSend(pxPrioQueue->xQueue,&xElement,xTicksToWait);
	if(xReturn==pdPASS)
	{
		if(pxPrioQueue->uxElement==0)
		{
			pxPrioQueue->ucHPrio=prio;
			pxPrioQueue->ucLPrio=prio;
		}
		else
		{
			if(prio>pxPrioQueue->ucHPrio)
				pxPrioQueue->ucHPrio=prio;
			if(prio<pxPrioQueue->ucLPrio)
				pxPrioQueue->ucLPrio=prio;
		}
		pxPrioQueue->uxElement++;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueIsEmpty(xPrioQueueHandle pxPrioQueue)
{
	return xPrioQueueIsEmptyFromISR(pxPrioQueue);
}

//================================ FROM ISR ================================//

portBASE_TYPE xPrioQueueReceiveHighFromISR(xPrioQueueHandle pxPrioQueue,void *pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken)
{
	if(pxPrioQueue->uxElement==0)
		return pdFALSE;

	signed portBASE_TYPE pxHPTWtest=pdFALSE;
	portBASE_TYPE xReturn=pdPASS;//==pdTRUE
	PrioElement xElement;
	PrioElement xElementOut;
	portBASE_TYPE xFound=pdFALSE;
	unsigned portCHAR ucNextHigh=pxPrioQueue->ucLPrio;
	int i;
	for(i=0; i<pxPrioQueue->uxElement;i++)
	{	
		xReturn &= xQueueReceiveFromISR( pxPrioQueue->xQueue, &xElement, &pxHPTWtest );
		if(xReturn==pdFALSE)
			break;
		if(xFound==pdFALSE && xElement.ucPrio == pxPrioQueue->ucHPrio)
		{
			xElementOut=xElement;
			xFound=pdTRUE;
		}
		else
		{
			if(ucNextHigh<xElement.ucPrio)
				ucNextHigh=xElement.ucPrio;

			xReturn &= xQueueSendFromISR(pxPrioQueue->xQueue,&xElement,NULL);
			if(xReturn==pdFALSE)
				break;
		}
	}
	if(pxHPTWtest && pxHigherPriorityTaskWoken!=NULL)
		*pxHigherPriorityTaskWoken=pxHPTWtest;

	if(xReturn == pdPASS)
	{
		pxPrioQueue->ucHPrio=ucNextHigh;
		memcpy( ( void * ) pvBuffer, ( void * ) xElementOut.vData, ( unsigned ) pxPrioQueue->uxItemSize );
		pxPrioQueue->uxElement--;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueReceiveLowFromISR(xPrioQueueHandle pxPrioQueue,void *pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken)
{
	if(pxPrioQueue->uxElement==0)
		return pdFALSE;

	signed portBASE_TYPE pxHPTWtest=pdFALSE;
	portBASE_TYPE xReturn=pdPASS;
	PrioElement xElement;
	PrioElement xElementOut;
	portBASE_TYPE xFound=pdFALSE;
	unsigned portCHAR ucNextLow=pxPrioQueue->ucHPrio;
	int i;
	for(i=0; i<pxPrioQueue->uxElement;i++)
	{
		xReturn &= xQueueReceiveFromISR( pxPrioQueue->xQueue, &xElement, &pxHPTWtest );

		if(xReturn==pdFALSE)
			break;
		if(xFound==pdFALSE && xElement.ucPrio == pxPrioQueue->ucLPrio)
		{
			xElementOut=xElement;
			xFound=pdTRUE;
		}
		else
		{
			if(ucNextLow>xElement.ucPrio)
				ucNextLow=xElement.ucPrio;

			xReturn &= xQueueSendFromISR(pxPrioQueue->xQueue,&xElement,NULL);
			if(xReturn==pdFALSE)
				break;
		}
	}

	if(pxHPTWtest && pxHigherPriorityTaskWoken!=NULL)
		*pxHigherPriorityTaskWoken=pxHPTWtest;

	if(xReturn == pdPASS)
	{
		pxPrioQueue->ucLPrio=ucNextLow;
		memcpy( ( void * ) pvBuffer, ( void * ) xElementOut.vData, ( unsigned ) pxPrioQueue->uxItemSize );
		pxPrioQueue->uxElement--;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueSendFromISR(xPrioQueueHandle pxPrioQueue, void * pvItemToQueue, unsigned portCHAR prio, signed portBASE_TYPE *pxHigherPriorityTaskWoken)
{
	if(pxPrioQueue->uxElement==pxPrioQueue->uxMaxElement)
		return pdFALSE;

	portBASE_TYPE xReturn;
	PrioElement xElement;
	
	//Element bauen
	xElement.ucPrio=prio;
	xElement.vData=pvItemToQueue;
	xReturn = xQueueSendFromISR(pxPrioQueue->xQueue,&xElement,pxHigherPriorityTaskWoken);
	if(xReturn==pdPASS)
	{
		if(pxPrioQueue->uxElement==0)
		{
			pxPrioQueue->ucHPrio=prio;
			pxPrioQueue->ucLPrio=prio;
		}
		else
		{
			if(prio>pxPrioQueue->ucHPrio)
				pxPrioQueue->ucHPrio=prio;
			if(prio<pxPrioQueue->ucLPrio)
				pxPrioQueue->ucLPrio=prio;
		}
		pxPrioQueue->uxElement++;
	}
	return xReturn;
}

portBASE_TYPE xPrioQueueIsEmptyFromISR(xPrioQueueHandle pxPrioQueue)
{
	return pxPrioQueue->uxElement==0 ? pdTRUE : pdFALSE;
}
