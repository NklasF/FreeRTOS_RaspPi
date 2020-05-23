#include "FreeRTOS.h"
#include<list.h>
#include<task.h>
#include<stdlib.h>
#include<string.h>

#define priorityQueueUNLOCKED					( ( signed portBASE_TYPE ) -1 )
#define priorityQueueLOCKED_UNMODIFIED			( ( signed portBASE_TYPE ) 0 )

#define errPRIOQUEUE_EMPTY							( 0 )
#define errPRIOQUEUE_FULL							( 0 )
#define pdPASS									( 1 )
#define pdFAIL									( 0 )

#define prvLockPriorityQueue( pxQueue )								\
	taskENTER_CRITICAL();									\
	{														\
		if( ( pxQueue )->xRxLock == priorityQueueUNLOCKED )			\
		{													\
			( pxQueue )->xRxLock = priorityQueueLOCKED_UNMODIFIED;	\
		}													\
		if( ( pxQueue )->xTxLock == priorityQueueUNLOCKED )			\
		{													\
			( pxQueue )->xTxLock = priorityQueueLOCKED_UNMODIFIED;	\
		}													\
	}														\
	taskEXIT_CRITICAL()

typedef struct PriorityQueueDefinition {
	signed char *pcHead[configMAX_PRIORITIES]; /*< Points to the beginning of the queue storage area. */
	signed char *pcTail[configMAX_PRIORITIES]; /*< Points to the byte at the end of the queue storage area.  Once more byte is allocated than necessary to store the queue items, this is used as a marker. */

	signed char *pcWriteTo[configMAX_PRIORITIES]; /*< Points to the free next place in the storage area. */
	signed char *pcReadFrom[configMAX_PRIORITIES]; /*< Points to the last place that a queued item was read from. */

	xList xTasksWaitingToSend; /*< List of tasks that are blocked waiting to post onto this queue.  Stored in priority order. */
	xList xTasksWaitingToReceive; /*< List of tasks that are blocked waiting to read from this queue.  Stored in priority order. */

	volatile unsigned portBASE_TYPE uxMessagesWaiting[configMAX_PRIORITIES];/*< The number of items currently in the queue. */
	unsigned portBASE_TYPE uxLength; /*< The length of the queue defined as the number of items it will hold, not the number of bytes. */
	unsigned portBASE_TYPE uxItemSize; /*< The size of each items that the queue will hold. */

	volatile signed portBASE_TYPE xRxLock; /*< Stores the number of items received from the queue (removed from the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */
	volatile signed portBASE_TYPE xTxLock; /*< Stores the number of items transmitted to the queue (added to the queue) while the queue was locked.  Set to queueUNLOCKED when the queue is not locked. */

} xPRIOQUEUE;

typedef xPRIOQUEUE * xPriorityQueueHandle;

enum PriorityQueueReceiveMode {
	PQ_RECEIVE_HIGH, PQ_RECEIVE_LOW, PQ_PEEK_HIGH
};

xPriorityQueueHandle xPriorityQueueCreate(unsigned portBASE_TYPE uxQueueLength,
		unsigned portBASE_TYPE uxItemSize);

portBASE_TYPE xPriorityQueueReset(xPriorityQueueHandle pxQueue,
portBASE_TYPE xNewQueue);

signed portBASE_TYPE xPriorityQueueGenericSend(xPriorityQueueHandle pxQueue,
		const void * const pvItemToQueue, portTickType xTicksToWait,
		unsigned portBASE_TYPE uxPriority);

signed portBASE_TYPE xPriorityQueueGenericSendFromISR(xPriorityQueueHandle pxQueue,
		const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken,
		unsigned portBASE_TYPE uxPriority);

void prvCopyDataToPriorityQueue(xPRIOQUEUE *pxQueue, const void *pvItemToQueue,
		unsigned portBASE_TYPE uxPriority);

signed portBASE_TYPE prvIsPriorityQueueFull(const xPriorityQueueHandle pxQueue,
		unsigned portBASE_TYPE uxPriority);

void prvUnlockPriorityQueue(xPriorityQueueHandle pxQueue);

signed portBASE_TYPE xPriorityQueueGenericReceive(xPriorityQueueHandle pxQueue,
		void * const pvBuffer, portTickType xTicksToWait,
		unsigned portBASE_TYPE uxMode);

signed portBASE_TYPE xPriorityQueueGenericReceiveFromISR(xPriorityQueueHandle pxQueue,
		void * const pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken,unsigned portBASE_TYPE uxMode);

void prvCopyDataFromPriorityQueue(xPRIOQUEUE * const pxQueue,
		const void *pvBuffer, unsigned portBASE_TYPE uxPriority);

signed portBASE_TYPE prvIsPriorityQueueEmpty(
		const xPriorityQueueHandle pxQueue);

#define xPriorityQueueReceiveHigh(pxQueue,pvBuffer,xTicksToWait) xPriorityQueueGenericReceive(pxQueue,pvBuffer,xTicksToWait,PQ_RECEIVE_HIGH)
#define xPriorityQueueReceiveLow(pxQueue,pvBuffer,xTicksToWait) xPriorityQueueGenericReceive(pxQueue,pvBuffer,xTicksToWait,PQ_RECEIVE_LOW)
#define xPriorityQueueReceiveHighFromISR(pxQueue,pvBuffer,pxHigherPriorityTaskWoken) xPriorityQueueGenericReceiveFromISR(pxQueue,pvBuffer,pxHigherPriorityTaskWoken,PQ_RECEIVE_HIGH)
#define xPriorityQueueReceiveLowFromISR(pxQueue,pvBuffer,pxHigherPriorityTaskWoken) xPriorityQueueGenericReceiveFromISR(pxQueue,pvBuffer,pxHigherPriorityTaskWoken,PQ_RECEIVE_LOW)
#define xPriorityQueuePeekHigh(pxQueue,pvBuffer,xTicksToWait) xPriorityQueueGenericReceive(pxQueue,pvBuffer,xTicksToWait,PQ_PEEK_HIGH)

void vPriorityQueueDelete(xPriorityQueueHandle pxQueue);

unsigned portBASE_TYPE uxPriorityQueueTotalMessagesWaiting(
		xPriorityQueueHandle pxQueue);

unsigned portBASE_TYPE uxPriorityQueueMessagesWaiting(
		xPriorityQueueHandle pxQueue, unsigned portBASE_TYPE uxPriority);

