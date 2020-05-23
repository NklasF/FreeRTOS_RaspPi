#include <priority_queue.h>
#define queueUNLOCKED					( ( signed portBASE_TYPE ) -1 )

xPriorityQueueHandle xPriorityQueueCreate(unsigned portBASE_TYPE uxQueueLength,
		unsigned portBASE_TYPE uxItemSize) {
	xPRIOQUEUE *pxNewQueue;
	size_t xQueueSizeInBytes;
	xPriorityQueueHandle xReturn = NULL;

	/* Allocate the new queue structure. */
	if (uxQueueLength > (unsigned portBASE_TYPE ) 0) {
		pxNewQueue = (xPRIOQUEUE *) pvPortMalloc(sizeof(xPRIOQUEUE));
		if (pxNewQueue != NULL) {
			/* Create the list of pointers to queue items.  The queue is one byte
			 longer than asked for to make wrap checking easier/faster. */
			xQueueSizeInBytes = (size_t)(uxQueueLength * uxItemSize)
					+ (size_t) 1;

			unsigned portBASE_TYPE uxI;
			for ( uxI = 0; uxI < configMAX_PRIORITIES;
					uxI++) {
				pxNewQueue->pcHead[uxI] = (signed char *) pvPortMalloc(
						xQueueSizeInBytes);
				if (pxNewQueue->pcHead != NULL) {
					/* Initialise the queue members as described above where the
					 queue type is defined. */
					pxNewQueue->uxLength = uxQueueLength;
					pxNewQueue->uxItemSize = uxItemSize;

					traceQUEUE_CREATE( pxNewQueue );
					xReturn = pxNewQueue;
				} else {
					traceQUEUE_CREATE_FAILED( ucQueueType );
					unsigned portBASE_TYPE uxJ;
					for (uxJ = 0; uxJ < uxI; uxJ++) {
						vPortFree(pxNewQueue->pcHead[uxJ]);
					}
					vPortFree(pxNewQueue);
				}
			}
			xPriorityQueueReset(pxNewQueue, pdTRUE);
		}
	}

	configASSERT( xReturn );

	return xReturn;
}

portBASE_TYPE xPriorityQueueReset(xPriorityQueueHandle pxQueue,
portBASE_TYPE xNewQueue) {
	configASSERT( pxQueue );

	taskENTER_CRITICAL()
	;
	{
		unsigned portBASE_TYPE uxI;
		for (uxI = 0; uxI < configMAX_PRIORITIES;
				uxI++) {
			pxQueue->pcTail[uxI] = pxQueue->pcHead[uxI]
					+ (pxQueue->uxLength * pxQueue->uxItemSize);
			pxQueue->pcWriteTo[uxI] = pxQueue->pcHead[uxI];
			pxQueue->pcReadFrom[uxI] = pxQueue->pcHead[uxI]
					+ ((pxQueue->uxLength - (unsigned portBASE_TYPE ) 1U)
							* pxQueue->uxItemSize);
			pxQueue->uxMessagesWaiting[uxI] = (unsigned portBASE_TYPE ) 0U;
		}
		pxQueue->xRxLock = priorityQueueUNLOCKED;
		pxQueue->xTxLock = priorityQueueUNLOCKED;

		if (xNewQueue == pdFALSE) {
			/* If there are tasks blocked waiting to read from the queue, then
			 the tasks will remain blocked as after this function exits the queue
			 will still be empty.  If there are tasks blocked waiting to	write to
			 the queue, then one should be unblocked as after this function exits
			 it will be possible to write to it. */
			if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToSend)) == pdFALSE) {
				if (xTaskRemoveFromEventList(
						&(pxQueue->xTasksWaitingToSend)) == pdTRUE) {
					portYIELD_WITHIN_API();
				}
			}
		} else {
			/* Ensure the event queues start in the correct state. */
			vListInitialise(&(pxQueue->xTasksWaitingToSend));
			vListInitialise(&(pxQueue->xTasksWaitingToReceive));
		}
	}
	taskEXIT_CRITICAL()
	;

	/* A value is returned for calling semantic consistency with previous
	 versions. */
	return pdPASS;
}

signed portBASE_TYPE xPriorityQueueGenericSend(xPriorityQueueHandle pxQueue,
		const void * const pvItemToQueue, portTickType xTicksToWait,
		unsigned portBASE_TYPE uxPriority) {
	signed portBASE_TYPE xEntryTimeSet = pdFALSE;
	xTimeOutType xTimeOut;

	configASSERT( pxQueue );configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );

	/* This function relaxes the coding standard somewhat to allow return
	 statements within the function itself.  This is done in the interest
	 of execution time efficiency. */
	for (;;) {
		taskENTER_CRITICAL()
		;
		{
			/* Is there room on the queue now?  To be running we must be
			 the highest priority task wanting to access the queue. */
			if (pxQueue->uxMessagesWaiting[uxPriority] < pxQueue->uxLength) {
				traceQUEUE_SEND( pxQueue );
				prvCopyDataToPriorityQueue(pxQueue, pvItemToQueue, uxPriority);

				/* If there was a task waiting for data to arrive on the
				 queue then unblock it now. */
				if ( listLIST_IS_EMPTY(
						&(pxQueue->xTasksWaitingToReceive)) == pdFALSE) {
					if (xTaskRemoveFromEventList(
							&(pxQueue->xTasksWaitingToReceive)) == pdTRUE) {
						/* The unblocked task has a priority higher than
						 our own so yield immediately.  Yes it is ok to do
						 this from within the critical section - the kernel
						 takes care of that. */
						portYIELD_WITHIN_API();
					}
				}

				taskEXIT_CRITICAL()
				;

				/* Return to the original privilege level before exiting the
				 function. */
				return pdPASS;
			} else {
				if (xTicksToWait == (portTickType) 0) {
					/* The queue was full and no block time is specified (or
					 the block time has expired) so leave now. */
					taskEXIT_CRITICAL()
					;

					/* Return to the original privilege level before exiting
					 the function. */
					traceQUEUE_SEND_FAILED( pxQueue );
					return errPRIOQUEUE_FULL;
				} else if (xEntryTimeSet == pdFALSE) {
					/* The queue was full and a block time was specified so
					 configure the timeout structure. */
					vTaskSetTimeOutState(&xTimeOut);
					xEntryTimeSet = pdTRUE;
				}
			}
		}
		taskEXIT_CRITICAL()
		;

		/* Interrupts and other tasks can send to and receive from the queue
		 now the critical section has been exited. */

		vTaskSuspendAll();
		prvLockPriorityQueue(pxQueue);

		/* Update the timeout state to see if it has expired yet. */
		if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE) {
			if (prvIsPriorityQueueFull(pxQueue, uxPriority) != pdFALSE) {
				traceBLOCKING_ON_QUEUE_SEND( pxQueue );
				vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToSend),
						xTicksToWait);

				/* Unlocking the queue means queue events can effect the
				 event list.  It is possible	that interrupts occurring now
				 remove this task from the event	list again - but as the
				 scheduler is suspended the task will go onto the pending
				 ready last instead of the actual ready list. */
				prvUnlockPriorityQueue(pxQueue);

				/* Resuming the scheduler will move tasks from the pending
				 ready list into the ready list - so it is feasible that this
				 task is already in a ready list before it yields - in which
				 case the yield will not cause a context switch unless there
				 is also a higher priority task in the pending ready list. */
				if (xTaskResumeAll() == pdFALSE) {
					portYIELD_WITHIN_API();
				}
			} else {
				/* Try again. */
				prvUnlockPriorityQueue(pxQueue);
				(void) xTaskResumeAll();
			}
		} else {
			/* The timeout has expired. */
			prvUnlockPriorityQueue(pxQueue);
			(void) xTaskResumeAll();

			/* Return to the original privilege level before exiting the
			 function. */
			traceQUEUE_SEND_FAILED( pxQueue );
			return errPRIOQUEUE_FULL;
		}
	}
	return pdFAIL;
}

void prvCopyDataToPriorityQueue(xPRIOQUEUE *pxQueue, const void *pvItemToQueue,
		unsigned portBASE_TYPE uxPriority) {

	memcpy((void *) pxQueue->pcWriteTo[uxPriority], pvItemToQueue,
			(unsigned) pxQueue->uxItemSize);
	pxQueue->pcWriteTo[uxPriority] += pxQueue->uxItemSize;
	if (pxQueue->pcWriteTo[uxPriority] >= pxQueue->pcTail[uxPriority]) {
		pxQueue->pcWriteTo[uxPriority] = pxQueue->pcHead[uxPriority];
	}

	++(pxQueue->uxMessagesWaiting[uxPriority]);
}

signed portBASE_TYPE prvIsPriorityQueueFull(const xPriorityQueueHandle pxQueue,
		unsigned portBASE_TYPE uxPriority) {
	signed portBASE_TYPE xReturn;

	taskENTER_CRITICAL()
	;
	xReturn = (pxQueue->uxMessagesWaiting[uxPriority] == pxQueue->uxLength);
	taskEXIT_CRITICAL()
	;

	return xReturn;
}

void prvUnlockPriorityQueue(xPriorityQueueHandle pxQueue) {
	/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED. */

	/* The lock counts contains the number of extra data items placed or
	 removed from the queue while the queue was locked.  When a queue is
	 locked items can be added or removed, but the event lists cannot be
	 updated. */
	taskENTER_CRITICAL()
	;
	{
		/* See if data was added to the queue while it was locked. */
		while (pxQueue->xTxLock > priorityQueueLOCKED_UNMODIFIED) {
			/* Data was posted while the queue was locked.  Are any tasks
			 blocked waiting for data to become available? */
			if ( listLIST_IS_EMPTY(
					&(pxQueue->xTasksWaitingToReceive)) == pdFALSE) {
				/* Tasks that are removed from the event list will get added to
				 the pending ready list as the scheduler is still suspended. */
				if (xTaskRemoveFromEventList(
						&(pxQueue->xTasksWaitingToReceive)) != pdFALSE) {
					/* The task waiting has a higher priority so record that a
					 context	switch is required. */
					vTaskMissedYield();
				}

				--(pxQueue->xTxLock);
			} else {
				break;
			}
		}

		pxQueue->xTxLock = priorityQueueUNLOCKED;
	}
	taskEXIT_CRITICAL()
	;

	/* Do the same for the Rx lock. */
	taskENTER_CRITICAL()
	;
	{
		while (pxQueue->xRxLock > priorityQueueLOCKED_UNMODIFIED) {
			if ( listLIST_IS_EMPTY(
					&(pxQueue->xTasksWaitingToSend)) == pdFALSE) {
				if (xTaskRemoveFromEventList(
						&(pxQueue->xTasksWaitingToSend)) != pdFALSE) {
					vTaskMissedYield();
				}

				--(pxQueue->xRxLock);
			} else {
				break;
			}
		}

		pxQueue->xRxLock = priorityQueueUNLOCKED;
	}
	taskEXIT_CRITICAL()
	;
}

signed portBASE_TYPE xPriorityQueueGenericReceive(xPriorityQueueHandle pxQueue,
		void * const pvBuffer, portTickType xTicksToWait,
		unsigned portBASE_TYPE uxMode) {
	signed portBASE_TYPE xEntryTimeSet = pdFALSE;
	xTimeOutType xTimeOut;
	signed char *pcOriginalReadPosition;

	configASSERT( pxQueue );configASSERT( !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );

	/* This function relaxes the coding standard somewhat to allow return
	 statements within the function itself.  This is done in the interest
	 of execution time efficiency. */

	for (;;) {
		taskENTER_CRITICAL()
		;
		{
			/* Is there data in the queue now?  To be running we must be
			 the highest priority task wanting to access the queue. */
			unsigned portBASE_TYPE uxPriority;
			unsigned portBASE_TYPE empty = pdTRUE;
			unsigned portBASE_TYPE lowerBound =
					uxMode == PQ_RECEIVE_LOW ? (configMAX_PRIORITIES - 1) : 0;
			unsigned portBASE_TYPE upperBound =
					uxMode == PQ_RECEIVE_LOW ? 0 : (configMAX_PRIORITIES - 1);
			unsigned portBASE_TYPE incrementStep =
					uxMode == PQ_RECEIVE_LOW ? -1 : 1;
			unsigned portBASE_TYPE uxI;
			for (uxI = lowerBound; uxI <= upperBound;
					uxI += incrementStep) {
				if (pxQueue->uxMessagesWaiting[uxI]
						> (unsigned portBASE_TYPE) 0) {
					uxPriority = uxI;
					empty = pdFALSE;
				}
			}
			if (!empty) {
				/* Remember our read position in case we are just peeking. */
				pcOriginalReadPosition = pxQueue->pcReadFrom[uxPriority];

				prvCopyDataFromPriorityQueue(pxQueue, pvBuffer, uxPriority);

				if (uxMode != PQ_PEEK_HIGH) {
					traceQUEUE_RECEIVE( pxQueue );

					/* We are actually removing data. */
					--(pxQueue->uxMessagesWaiting[uxPriority]);

					if ( listLIST_IS_EMPTY(
							&(pxQueue->xTasksWaitingToSend)) == pdFALSE) {
						if (xTaskRemoveFromEventList(
								&(pxQueue->xTasksWaitingToSend)) == pdTRUE) {
							portYIELD_WITHIN_API();
						}
					}
				} else {
					traceQUEUE_PEEK( pxQueue );

					/* We are not removing the data, so reset our read
					 pointer. */
					pxQueue->pcReadFrom[uxPriority] = pcOriginalReadPosition;

					/* The data is being left in the queue, so see if there are
					 any other tasks waiting for the data. */
					if ( listLIST_IS_EMPTY(
							&(pxQueue->xTasksWaitingToReceive)) == pdFALSE) {
						/* Tasks that are removed from the event list will get added to
						 the pending ready list as the scheduler is still suspended. */
						if (xTaskRemoveFromEventList(
								&(pxQueue->xTasksWaitingToReceive)) != pdFALSE) {
							/* The task waiting has a higher priority than this task. */
							portYIELD_WITHIN_API();
						}
					}
				}

				taskEXIT_CRITICAL()
				;
				return pdPASS;
			} else {
				if (xTicksToWait == (portTickType) 0) {
					/* The queue was empty and no block time is specified (or
					 the block time has expired) so leave now. */
					taskEXIT_CRITICAL()
					;traceQUEUE_RECEIVE_FAILED( pxQueue );
					return errQUEUE_EMPTY;
				} else if (xEntryTimeSet == pdFALSE) {
					/* The queue was empty and a block time was specified so
					 configure the timeout structure. */
					vTaskSetTimeOutState(&xTimeOut);
					xEntryTimeSet = pdTRUE;
				}
			}
		}
		taskEXIT_CRITICAL()
		;

		/* Interrupts and other tasks can send to and receive from the queue
		 now the critical section has been exited. */

		vTaskSuspendAll();
		prvLockPriorityQueue(pxQueue);

		/* Update the timeout state to see if it has expired yet. */
		if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE) {
			if (prvIsPriorityQueueEmpty(pxQueue) != pdFALSE) {
				traceBLOCKING_ON_QUEUE_RECEIVE( pxQueue );

				vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToReceive),
						xTicksToWait);
				prvUnlockPriorityQueue(pxQueue);
				if (xTaskResumeAll() == pdFALSE) {
					portYIELD_WITHIN_API();
				}
			} else {
				/* Try again. */
				prvUnlockPriorityQueue(pxQueue);
				(void) xTaskResumeAll();
			}
		} else {
			prvUnlockPriorityQueue(pxQueue);
			(void) xTaskResumeAll();
			traceQUEUE_RECEIVE_FAILED( pxQueue );
			return errQUEUE_EMPTY;
		}
	}
	return pdFALSE;
}

void prvCopyDataFromPriorityQueue(xPRIOQUEUE * const pxQueue,
		const void *pvBuffer, unsigned portBASE_TYPE uxPriority) {

	pxQueue->pcReadFrom[uxPriority] += pxQueue->uxItemSize;
	if (pxQueue->pcReadFrom[uxPriority] >= pxQueue->pcTail[uxPriority]) {
		pxQueue->pcReadFrom[uxPriority] = pxQueue->pcHead[uxPriority];
	}
	memcpy((void *) pvBuffer, (void *) pxQueue->pcReadFrom[uxPriority],
			(unsigned) pxQueue->uxItemSize);

}

signed portBASE_TYPE prvIsPriorityQueueEmpty(const xPriorityQueueHandle pxQueue) {
	signed portBASE_TYPE xReturn;

	taskENTER_CRITICAL()
	;
	unsigned portBASE_TYPE uxI;
	for (uxI = 0; uxI < configMAX_PRIORITIES; uxI++) {
		xReturn =
				(pxQueue->uxMessagesWaiting[uxI] > (unsigned portBASE_TYPE ) 0);
	}
	taskEXIT_CRITICAL()
	;

	return !xReturn;
}

void vPriorityQueueDelete(xPriorityQueueHandle pxQueue) {
	unsigned portBASE_TYPE uxJ;
	for (uxJ = 0; uxJ < configMAX_PRIORITIES; uxJ++) {
		vPortFree(pxQueue->pcHead[uxJ]);
	}
	vPortFree(pxQueue);
}

unsigned portBASE_TYPE uxPriorityQueueTotalMessagesWaiting(
		xPriorityQueueHandle pxQueue) {
	unsigned portBASE_TYPE ret = 0;
	unsigned portBASE_TYPE uxI;
	for (uxI = 0; uxI < configMAX_PRIORITIES; uxI++) {
		ret += pxQueue->uxMessagesWaiting[uxI];
	}
	return ret;
}

unsigned portBASE_TYPE uxPriorityQueueMessagesWaiting(
		xPriorityQueueHandle pxQueue, unsigned portBASE_TYPE uxPriority) {
	return pxQueue->uxMessagesWaiting[uxPriority];
}


//--------------------------------------------------From ISR

signed portBASE_TYPE xPriorityQueueGenericSendFromISR(xPriorityQueueHandle pxQueue,
		const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken,
		unsigned portBASE_TYPE uxPriority) {

signed portBASE_TYPE xReturn;
unsigned portBASE_TYPE uxSavedInterruptStatus;

	configASSERT( pxQueue );configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );

	/* Similar to xQueueGenericSend, except we don't block if there is no room
	in the queue.  Also we don't directly wake a task that was blocked on a
	queue read, instead we return a flag to say whether a context switch is
	required or not (i.e. has a task with a higher priority than us been woken
	by this	post). */
	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( pxQueue->uxMessagesWaiting[uxPriority] < pxQueue->uxLength )
		{
			traceQUEUE_SEND_FROM_ISR( pxQueue );

			prvCopyDataToPriorityQueue( pxQueue, pvItemToQueue, uxPriority );

			/* If the queue is locked we do not alter the event list.  This will
			be done when the queue is unlocked later. */
			if( pxQueue->xTxLock == queueUNLOCKED )
			{
				if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
				{
					if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
					{
						/* The task waiting has a higher priority so record that a
						context	switch is required. */
						if( pxHigherPriorityTaskWoken != NULL )
						{
							*pxHigherPriorityTaskWoken = pdTRUE;
						}
					}
				}
			}
			else
			{
				/* Increment the lock count so the task that unlocks the queue
				knows that data was posted while it was locked. */
				++( pxQueue->xTxLock );
			}

			xReturn = pdPASS;
		}
		else
		{
			traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue );
			xReturn = errQUEUE_FULL;
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

signed portBASE_TYPE xPriorityQueueGenericReceiveFromISR(xPriorityQueueHandle pxQueue,
		void * const pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken,unsigned portBASE_TYPE uxMode) {
signed portBASE_TYPE xReturn;
unsigned portBASE_TYPE uxSavedInterruptStatus;
	signed char *pcOriginalReadPosition;

	configASSERT( pxQueue );configASSERT( !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );


	/* Is there data in the queue now?  To be running we must be
	 the highest priority task wanting to access the queue. */
	unsigned portBASE_TYPE uxPriority;
	unsigned portBASE_TYPE empty = pdTRUE;
	unsigned portBASE_TYPE lowerBound =
			uxMode == PQ_RECEIVE_LOW ? (configMAX_PRIORITIES - 1) : 0;
	unsigned portBASE_TYPE upperBound =
			uxMode == PQ_RECEIVE_LOW ? 0 : (configMAX_PRIORITIES - 1);
	unsigned portBASE_TYPE incrementStep =
			uxMode == PQ_RECEIVE_LOW ? -1 : 1;
	unsigned portBASE_TYPE uxI;
	for (uxI = lowerBound; uxI <= upperBound;
			uxI += incrementStep) {
		if (pxQueue->uxMessagesWaiting[uxI]
				> (unsigned portBASE_TYPE) 0) {
			uxPriority = uxI;
			empty = pdFALSE;
		}
	}

	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		if (!empty) {
			/* Remember our read position in case we are just peeking. */
			pcOriginalReadPosition = pxQueue->pcReadFrom[uxPriority];

			prvCopyDataFromPriorityQueue(pxQueue, pvBuffer, uxPriority);

			traceQUEUE_RECEIVE_FROM_ISR( pxQueue );

			/* We are actually removing data. */
			--(pxQueue->uxMessagesWaiting[uxPriority]);

			/* If the queue is locked we will not modify the event list.  Instead
			we update the lock count so the task that unlocks the queue will know
			that an ISR has removed data while the queue was locked. */
			if( pxQueue->xRxLock == queueUNLOCKED )
			{
				if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
				{
					if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
					{
						/* The task waiting has a higher priority than us so
						force a context switch. */
						if( pxHigherPriorityTaskWoken != NULL )
						{
							*pxHigherPriorityTaskWoken = pdTRUE;
						}
					}
				}
			}
			else
			{
				/* Increment the lock count so the task that unlocks the queue
				knows that data was removed while it was locked. */
				++( pxQueue->xRxLock );
			}

			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
			traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue );
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}

