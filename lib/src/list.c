/******************************************************************************************************
 *                                                                                                    *
 *  L I S T . C                                                                                       *
 *  ===========                                                                                       *
 *                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU        *
 *  General Public License version 2 as published by the Free Software Foundation.  Note that I am    *
 *  not granting permission to redistribute or modify this under the terms of any later version of    *
 *  the General Public License.                                                                       *
 *                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even    *
 *  the impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
 *  General Public License for more details.                                                          *
 *                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in     *
 *  the file "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place -       *
 *  Suite 330, Boston, MA 02111, USA.                                                                 *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @file
 *  @brief Functions to a generic linked list.
 *  @version $Id: list.c 1076 2010-10-02 20:38:45Z chris $
 */
#include <stdlib.h>
#include <string.h>
#include "dircmd.h"

#ifdef MULTI_THREAD
	#ifdef WIN32
		#include <windows.h>
	#endif
#endif

/******************************************************************************************************
 *                                                                                                    *
 * Structure to hold an item on the queue                                                             *
 *                                                                                                    *
 ******************************************************************************************************/
typedef struct _queueItem
{
	void *myNextPtr;
	void *myPrevPtr;
	void *myData;
}
QUEUE_ITEM;

/******************************************************************************************************
 *                                                                                                    *
 * Structure to hold the queue header                                                                 *
 *                                                                                                    *
 ******************************************************************************************************/
typedef struct _queueHeader
{
	QUEUE_ITEM *firstInQueue;
	QUEUE_ITEM *lastInQueue;
	QUEUE_ITEM *currentItem;
	unsigned long itemCount;
	unsigned long freeData;

#ifdef MULTI_THREAD
#ifdef WIN32

	HANDLE queueMutex;

#else

	pthread_mutex_t queueMutex;

#endif
#endif
}
QUEUE_HEADER;

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  L O C K                                                                                *
 *  ==================                                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Internal function to lock the queue.
 *  @param myQueue Queue to lock.
 *  @result None.
 */
static void queueLock (QUEUE_HEADER *myQueue)
{
#ifdef MULTI_THREAD
#ifdef WIN32

	WaitForSingleObject (myQueue -> queueMutex, INFINITE);

#else

	pthread_mutex_lock(&myQueue -> queueMutex);

#endif
#endif
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  U N  L O C K                                                                           *
 *  =======================                                                                           *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Internal function to unloch the queue.
 *  @param myQueue Queue to unlock.
 *  @result None.
 */
static void queueUnLock (QUEUE_HEADER *myQueue)
{
#ifdef MULTI_THREAD
#ifdef WIN32

	ReleaseMutex (myQueue -> queueMutex);

#else

	pthread_mutex_unlock(&myQueue -> queueMutex);

#endif
#endif
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  C R E A T E                                                                            *
 *  ======================                                                                            *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Create a new queue.
 *  @result Reutrns a pointer to a queue header to be used in future calls.
 */
void *queueCreate ()
{
	QUEUE_HEADER *newQueue;

	if ((newQueue = malloc (sizeof (QUEUE_HEADER))) == NULL)
		return NULL;

	newQueue -> firstInQueue = NULL;
	newQueue -> lastInQueue = NULL;
	newQueue -> currentItem = NULL;
	newQueue -> itemCount = 0;
	newQueue -> freeData = 0;

#ifdef MULTI_THREAD
#ifdef WIN32

	newQueue -> queueMutex = CreateMutex (NULL, FALSE, "Queue");

#else

	pthread_mutex_init(&newQueue -> queueMutex, NULL);

#endif
#endif
	return newQueue;
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  D E L E T E                                                                            *
 *  ======================                                                                            *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Delete a queue.
 *  @param queueHandle Handle of the queue to delete, returned from create.
 *  @result None.
 */
void queueDelete (void *queueHandle)
{
	if (queueHandle)
	{
#ifdef MULTI_THREAD

		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;

#ifdef WIN32

		CloseHandle (myQueue -> queueMutex);

#else

		pthread_mutex_destroy(&myQueue -> queueMutex);

#endif
#endif

		free (queueHandle);
	}
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  G E T                                                                                  *
 *  ================                                                                                  *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Read the first item from the queue and remove it.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @result A pointer to the item or NULL if the queue was empty.
 */
void *queueGet (void *queueHandle)
{
	void *retn = NULL;

	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *oldQueueItem;

		queueLock (myQueue);
		if (myQueue -> firstInQueue)
		{
			oldQueueItem = myQueue -> firstInQueue;
			retn = oldQueueItem -> myData;
			myQueue -> firstInQueue = oldQueueItem -> myNextPtr;
			free (oldQueueItem);

			if (myQueue -> firstInQueue)
				myQueue -> firstInQueue -> myPrevPtr = NULL;
			else
				myQueue -> lastInQueue = NULL;
			
			myQueue -> itemCount --;
		}
		queueUnLock (myQueue);
	}
	return retn;
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  P U T                                                                                  *
 *  ================                                                                                  *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Put a item on the end of the queue.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param putData Data to put on the queue, normally a pointer.
 *  @result None.
 */
void queuePut (void *queueHandle, void *putData)
{
	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *newQueueItem;

		if ((newQueueItem = malloc (sizeof (QUEUE_ITEM))) == NULL)
			return;

		newQueueItem -> myNextPtr = newQueueItem -> myPrevPtr = NULL;
		newQueueItem -> myData = putData;

		queueLock (myQueue);
		if (myQueue -> lastInQueue)
		{
			myQueue -> lastInQueue -> myNextPtr = newQueueItem;
			newQueueItem -> myPrevPtr = myQueue -> lastInQueue;
			myQueue -> lastInQueue = newQueueItem;
		}
		else
			myQueue -> firstInQueue = myQueue -> lastInQueue = newQueueItem;

		myQueue -> itemCount ++;
		queueUnLock (myQueue);
	}
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  P U S H                                                                                *
 *  ==================                                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Put a item at the front of the queue.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param putData Data to push on to queue, normally a pointer.
 *  @result None.
 */
void queuePush (void *queueHandle, void *putData)
{
	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *newQueueItem;

		if ((newQueueItem = malloc (sizeof (QUEUE_ITEM))) == NULL)
			return;

		newQueueItem -> myNextPtr = newQueueItem -> myPrevPtr = NULL;
		newQueueItem -> myData = putData;

		queueLock (myQueue);
		if (myQueue -> firstInQueue)
		{
			myQueue -> firstInQueue -> myPrevPtr = newQueueItem;
			newQueueItem -> myNextPtr = myQueue -> firstInQueue;
			myQueue -> firstInQueue = newQueueItem;
		}
		else
			myQueue -> firstInQueue = myQueue -> lastInQueue = newQueueItem;

		myQueue -> itemCount ++;
		queueUnLock (myQueue);
	}
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  P U T  S O R T                                                                         *
 *  =========================                                                                         *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Put an item on the queue in a sorted order.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param putData Data to put on the queue, normally a pointer.
 *  @param Compare Function to compare two items.
 *  @result None.
 */
void queuePutSort (void *queueHandle, void *putData, 
		int(*Compare)(const void *item1, const void *item2))
{
	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *newQueueItem;

		if ((newQueueItem = malloc (sizeof (QUEUE_ITEM))) == NULL)
			return;

		newQueueItem -> myNextPtr = newQueueItem -> myPrevPtr = NULL;
		newQueueItem -> myData = putData;

		queueLock (myQueue);
		if (myQueue -> firstInQueue)
		{
			QUEUE_ITEM *currentItem = myQueue -> firstInQueue;
	
			while (currentItem)
			{
				int comp = Compare (putData, currentItem -> myData);
			
				if (comp < 0)
				{
					newQueueItem -> myPrevPtr = currentItem -> myPrevPtr;
					newQueueItem -> myNextPtr = currentItem;
				
					if (newQueueItem -> myPrevPtr)
					{
						QUEUE_ITEM *tempItem = newQueueItem -> myPrevPtr;
						tempItem -> myNextPtr = newQueueItem;
					}
					else
						myQueue -> firstInQueue = newQueueItem;

					currentItem -> myPrevPtr = newQueueItem;
					break;
				}
				currentItem = currentItem -> myNextPtr;
			}
			if (!currentItem)
			{
				myQueue -> lastInQueue -> myNextPtr = newQueueItem;
				newQueueItem -> myPrevPtr = myQueue -> lastInQueue;
				myQueue -> lastInQueue = newQueueItem;
			}
		}
		else
			myQueue -> firstInQueue = myQueue -> lastInQueue = newQueueItem;

		myQueue -> itemCount ++;
		queueUnLock (myQueue);
	}
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  R E A D                                                                                *
 *  ==================                                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Read, do not remove, the nth item on the queue.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param item The number of the item to read, 0 is the first.
 *  @result A pointer to the item or NULL if none found.
 */
void *queueRead (void *queueHandle, int item)
{
	void *retn = NULL;

	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *currentItem = myQueue -> firstInQueue;

		queueLock (myQueue);
		while (currentItem && item > 0)
		{
			currentItem = currentItem -> myNextPtr;
			item --;
		}
		if (currentItem)
			retn = currentItem -> myData;

		queueUnLock (myQueue);
	}
	return retn;
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  S E T  F R E E  D A T A                                                                *
 *  ==================================                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Sets a user defined long value on the queue .
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param setData Data to save on the queue.
 *  @result None.
 */
void queueSetFreeData (void *queueHandle, unsigned long setData)
{
	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		queueLock (myQueue);
		myQueue -> freeData = setData;
		queueUnLock (myQueue);
	}
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  G E T  F R E E  D A T A                                                                *
 *  ==================================                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Gets a user defined long value from the queue.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @result Data to saved on the queue, default 0.
 */
unsigned long queueGetFreeData (void *queueHandle)
{
	unsigned long retn = 0L;

	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
	
		queueLock (myQueue);
		retn = myQueue -> freeData;
		queueUnLock (myQueue);
	}
	return retn;
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  G E T  I T E M  C O U N T                                                              *
 *  ====================================                                                              *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Get the number of items currently in the queue.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @result The number of items in the queue.
 */
unsigned long queueGetItemCount (void *queueHandle)
{
	unsigned long retn = 0L;

	if (queueHandle)
	{
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
	
		queueLock (myQueue);
		retn = myQueue -> itemCount;
		queueUnLock (myQueue);
	}
	return retn;
}

/******************************************************************************************************
 *                                                                                                    *
 *  Q U E U E  S O R T                                                                                *
 *  ==================                                                                                *
 *                                                                                                    *
 ******************************************************************************************************/
/**
 *  @brief Sort the list.
 *  @param queueHandle Handle of the queue, returned from create.
 *  @param Compare Function to compare two items.
 *  @result None.
 */
void queueSort (void *queueHandle, int(*Compare)(const void *item1, const void *item2))
{
	if (queueHandle)
	{
		char *tempArray, **tempPtr;
		QUEUE_HEADER *myQueue = (QUEUE_HEADER *)queueHandle;
		QUEUE_ITEM *queueItem, *queueItemNext;
		int itemCount, i = 0;

		queueLock (myQueue);
		itemCount = myQueue -> itemCount;
		if (itemCount == 0)
		{
			queueUnLock (myQueue);
			return;
		}
		if ((tempArray = malloc (sizeof (char *) * itemCount)) != NULL)
		{
			i = 0;
			tempPtr = (char **)tempArray;
			queueItem = myQueue -> firstInQueue;
			while (queueItem && i < itemCount)
			{
				tempPtr[i++] = queueItem -> myData;
				queueItemNext = queueItem -> myNextPtr;
				free (queueItem);
				queueItem = queueItemNext;
			}
			myQueue -> firstInQueue = myQueue -> lastInQueue = NULL;
			qsort (&tempPtr[0], itemCount, sizeof (char *), Compare);
			for (i = 0; i < itemCount; i++)
			{
				if ((queueItem = malloc (sizeof (QUEUE_ITEM))) == NULL)
				{
					myQueue -> itemCount = i;
					break;
				}
				queueItem -> myNextPtr = queueItem -> myPrevPtr = NULL;
				queueItem -> myData = tempPtr[i];
				if (myQueue -> lastInQueue)
				{
					myQueue -> lastInQueue -> myNextPtr = queueItem;
					queueItem -> myPrevPtr = myQueue -> lastInQueue;
					myQueue -> lastInQueue = queueItem;
				}
				else
				{
					myQueue -> firstInQueue = myQueue -> lastInQueue = queueItem;
				}
			}
			free (tempArray);
		}
		queueUnLock (myQueue);
	}
}

