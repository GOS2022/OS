//*************************************************************************************************
//
//                            #####             #####             #####
//                          #########         #########         #########
//                         ##                ##       ##       ##
//                        ##                ##         ##        #####
//                        ##     #####      ##         ##           #####
//                         ##       ##       ##       ##                ##
//                          #########         #########         #########
//                            #####             #####             #####
//
//                                      (c) Ahmed Gazar, 2022
//
//*************************************************************************************************
//! @file       gos_queue.c
//! @author     Ahmed Gazar
//! @date       2024-04-02
//! @version    1.8
//!
//! @brief      GOS queue service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_queue.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-23    Ahmed Gazar     Initial version created
// 1.1        2022-11-14    Ahmed Gazar     +    Dump separator changed
//                                          +    Signal sender enumerators added
//                                          +    Formatted log used in dump
// 1.2        2022-11-15    Ahmed Gazar     +    Queue peek function added
//                                          +    License added
// 1.3        2022-12-13    Ahmed Gazar     +    Initialization error logging added
// 1.4        2023-05-04    Ahmed Gazar     *    Lock calls replaced with mutex calls
// 1.5        2023-06-17    Ahmed Gazar     *    Queue dump moved to function
// 1.6        2023-06-30    Ahmed Gazar     *    Mutex unlock calls added before hook calls
//                                          *    Queue dump remapped to shell driver
//                                          +    Timeout parameter added to queue peek, put, get
// 1.7        2023-09-14    Ahmed Gazar     +    Mutex initialization result processing added
// 1.8        2024-04-02    Ahmed Gazar     *    Inline macros removed from functions
//                                          +    gos_queueReset added
//*************************************************************************************************
//
// Copyright (c) 2022 Ahmed Gazar
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*************************************************************************************************
/*
 * Includes
 */
#include <gos_queue.h>
#include <gos_mutex.h>
#include <gos_shell_driver.h>
#include <stdio.h>
#include <string.h>

/*
 * Macros
 */
#if CFG_QUEUE_USE_NAME == 1
/**
 * Dump separator line.
 */
#define DUMP_SEPARATOR    "+--------+------------------------------+---------------+\r\n"
#else
/**
 * Dump separator line.
 */
#define DUMP_SEPARATOR    "+--------+------------------------------+\r\n"
#endif

/*
 * Type definitions
 */
/**
 * Queue element type.
 */
typedef struct
{
    gos_queueByte_t   queueElementBytes [CFG_QUEUE_MAX_LENGTH]; //!< Queue element bytes.
    gos_queueLength_t elementLength;                            //!< Queue element length.
}gos_queueElement_t;

/**
 * Queue type.
 */
typedef struct
{
    gos_queueId_t      queueId;                                //!< Queue ID.
#if CFG_QUEUE_USE_NAME == 1
    gos_queueName_t    queueName;                              //!< Queue name.
#endif
    gos_queueElement_t queueElements [CFG_QUEUE_MAX_ELEMENTS]; //!< Queue element array.
    gos_queueIndex_t   actualElementNumber;                    //!< Actual number of queue elements.
}gos_queue_t;

/*
 * Static variables
 */
/**
 * Internal queue array.
 */
GOS_STATIC gos_queue_t        queues         [CFG_QUEUE_MAX_NUMBER];

/**
 * Read counter array (next queue element to read).
 */
GOS_STATIC gos_queueLength_t  readCounters   [CFG_QUEUE_MAX_NUMBER];

/**
 * Write counter array (next queue element to write).
 */
GOS_STATIC gos_queueLength_t  writeCounters  [CFG_QUEUE_MAX_NUMBER];

/**
 * Queue mutex.
 */
GOS_STATIC gos_mutex_t        queueMutex;

/**
 * Queue full hook.
 */
GOS_STATIC gos_queueFullHook  queueFullHook  = NULL;

/**
 * Queue empty hook.
 */
GOS_STATIC gos_queueEmptyHook queueEmptyHook = NULL;

/*
 * Function: gos_queueInit
 */
gos_result_t gos_queueInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t     queueInitResult   = GOS_SUCCESS;
    gos_queueIndex_t queueIndex        = 0u;
    gos_queueIndex_t queueElementIndex = 0u;

    /*
     * Function code.
     */
    for (queueIndex = 0u; queueIndex < CFG_QUEUE_MAX_NUMBER; queueIndex++)
    {
        readCounters[queueIndex]               = 0u;
        writeCounters[queueIndex]              = 0u;
        queues[queueIndex].queueId             = GOS_INVALID_QUEUE_ID;
        queues[queueIndex].actualElementNumber = 0u;

        for (queueElementIndex = 0u; queueElementIndex < CFG_QUEUE_MAX_ELEMENTS; queueElementIndex++)
        {
            queues[queueIndex].queueElements[queueElementIndex].elementLength = 0u;
        }
    }

    // Initialize mutex.
    queueInitResult = gos_mutexInit(&queueMutex);

    return queueInitResult;
}

/*
 * Function: gos_queueCreate
 */
gos_result_t gos_queueCreate (gos_queueDescriptor_t* pQueueDescriptor)
{
    /*
     * Local variables.
     */
    gos_result_t     queueCreateResult = GOS_ERROR;
    gos_queueIndex_t queueIndex        = 0u;

    /*
     * Function code.
     */
    if (pQueueDescriptor != NULL)
    {
        for (queueIndex = 0u; queueIndex < CFG_QUEUE_MAX_NUMBER; queueIndex++)
        {
            if (queues[queueIndex].queueId == GOS_INVALID_QUEUE_ID)
            {
                queues[queueIndex].queueId = (GOS_DEFAULT_QUEUE_ID + queueIndex);
                pQueueDescriptor->queueId = queues[queueIndex].queueId;
#if CFG_QUEUE_USE_NAME == 1
                if (pQueueDescriptor->queueName != NULL &&
                    strlen(pQueueDescriptor->queueName) <= CFG_QUEUE_MAX_NAME_LENGTH)
                {
                    (void_t) strcpy(queues[queueIndex].queueName, pQueueDescriptor->queueName);
                }
                else
                {
                    // Nothing to do.
                }
#endif
                queueCreateResult = GOS_SUCCESS;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return queueCreateResult;
}

/*
 * Function: gos_queuePut
 */
gos_result_t gos_queuePut (
        gos_queueId_t     queueId,     void_t* element,
        gos_queueLength_t elementSize, u32_t   timeout
        )
{
    /*
     * Local variables.
     */
    gos_result_t     queuePutResult = GOS_ERROR;
    gos_queueIndex_t queueIndex     = 0u;

    /*
     * Function code.
     */
    if (gos_mutexLock(&queueMutex, timeout)              == GOS_SUCCESS          &&
        queueId                                          >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                 <  CFG_QUEUE_MAX_NUMBER &&
        element                                          != NULL                 &&
        elementSize                                      <= CFG_QUEUE_MAX_LENGTH &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId != GOS_INVALID_QUEUE_ID
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        // Check if queue is full.
        if ((readCounters[queueIndex] > 0 && writeCounters[queueIndex] != (readCounters[queueIndex] - 1)) ||
            ((readCounters[queueIndex] == 0) && writeCounters[queueIndex] != CFG_QUEUE_MAX_ELEMENTS - 1))
        {
            (void_t) memcpy(
                    queues[queueIndex].queueElements[writeCounters[queueIndex]].queueElementBytes,
                    element,
                    elementSize
                    );
            queues[queueIndex].queueElements[writeCounters[queueIndex]].elementLength = elementSize;
            writeCounters[queueIndex]++;
            queues[queueIndex].actualElementNumber++;

            if (writeCounters[queueIndex] >= CFG_QUEUE_MAX_ELEMENTS)
            {
                writeCounters[queueIndex] = 0u;
            }
            else
            {
                // Nothing to do.
            }

            // If queue gets full and there is a registered hook function, call it.
            if (queues[queueIndex].actualElementNumber == CFG_QUEUE_MAX_ELEMENTS - 1 &&
                queueFullHook != NULL)
            {
                gos_mutexUnlock(&queueMutex);
                queueFullHook(queueId);
            }
            else
            {
                // Nothing to do.
            }

            queuePutResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // Unlock mutex.
    (void_t) gos_mutexUnlock(&queueMutex);

    return queuePutResult;
}

/*
 * Function: gos_queueGet
 */
gos_result_t gos_queueGet (
        gos_queueId_t     queueId,    void_t* target,
        gos_queueLength_t targetSize, u32_t   timeout
        )
{
    /*
     * Local variables.
     */
    gos_result_t     queueGetResult = GOS_ERROR;
    gos_queueIndex_t queueIndex     = 0u;

    /*
     * Function code.
     */
    if (gos_mutexLock(&queueMutex, timeout)              == GOS_SUCCESS          &&
        target                                           != NULL                 &&
        queueId                                          >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                 <  CFG_QUEUE_MAX_NUMBER &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId != GOS_INVALID_QUEUE_ID &&
        targetSize                                       >=
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueElements[readCounters[(queueId - GOS_DEFAULT_QUEUE_ID)]].elementLength
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        // Check if queue is empty.
        if (readCounters[queueIndex] != writeCounters[queueIndex])
        {
            (void_t) memcpy(target, queues[queueIndex].queueElements[readCounters[queueIndex]].queueElementBytes, queues[queueIndex].queueElements[readCounters[queueIndex]].elementLength);
            queues[queueIndex].queueElements[readCounters[queueIndex]].elementLength = 0u;
            readCounters[queueIndex]++;
            queues[queueIndex].actualElementNumber--;

            if (readCounters[queueIndex] >= CFG_QUEUE_MAX_ELEMENTS)
            {
                readCounters[queueIndex] = 0u;
            }
            else
            {
                // Nothing to do.
            }

            // If queue is empty now and empty hook function is registered, call it.
            if (queues[queueIndex].actualElementNumber == 0u &&
                queueEmptyHook != NULL)
            {
                gos_mutexUnlock(&queueMutex);
                queueEmptyHook(queueId);
            }
            else
            {
                // Nothing to do.
            }

            queueGetResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // Unlock mutex.
    (void_t) gos_mutexUnlock(&queueMutex);

    return queueGetResult;
}

/*
 * Function: gos_queuePeek
 */
gos_result_t gos_queuePeek (
        gos_queueId_t     queueId,    void_t* target,
        gos_queueLength_t targetSize, u32_t   timeout
        )
{
    /*
     * Local variables.
     */
    gos_result_t     queuePeekResult = GOS_ERROR;
    gos_queueIndex_t queueIndex      = 0u;

    /*
     * Function code.
     */
    if (gos_mutexLock(&queueMutex, timeout)              == GOS_SUCCESS          &&
        target                                           != NULL                 &&
        queueId                                          >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                 <  CFG_QUEUE_MAX_NUMBER &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId != GOS_INVALID_QUEUE_ID &&
        targetSize                                       >=
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueElements[readCounters[(queueId - GOS_DEFAULT_QUEUE_ID)]].elementLength
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        // Check if queue is empty.
        if (readCounters[queueIndex] != writeCounters[queueIndex])
        {
            (void_t) memcpy(target, queues[queueIndex].queueElements[readCounters[queueIndex]].queueElementBytes, queues[queueIndex].queueElements[readCounters[queueIndex]].elementLength);
            queuePeekResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // Unlock mutex.
    (void_t) gos_mutexUnlock(&queueMutex);

    return queuePeekResult;
}

/*
 * Function: gos_queueReset
 */
gos_result_t gos_queueReset (gos_queueId_t queueId)
{
    /*
     * Local variables.
     */
    gos_result_t     queueResetResult = GOS_ERROR;
    gos_queueIndex_t queueIndex       = 0u;

    /*
     * Function code.
     */
    if (gos_mutexLock(&queueMutex, GOS_MUTEX_ENDLESS_TMO) == GOS_SUCCESS          &&
        queueId                                           >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                  <  CFG_QUEUE_MAX_NUMBER &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId  != GOS_INVALID_QUEUE_ID
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        readCounters[queueIndex]  = 0u;
        writeCounters[queueIndex] = 0u;

        queueResetResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    // Unlock mutex.
    (void_t) gos_mutexUnlock(&queueMutex);

    return queueResetResult;
}

/*
 * Function: gos_queueRegisterFullHook
 */
gos_result_t gos_queueRegisterFullHook (gos_queueFullHook fullHook)
{
    /*
     * Local variables.
     */
    gos_result_t queueRegisterFullHookResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (fullHook != NULL && queueFullHook == NULL)
    {
        queueFullHook               = fullHook;
        queueRegisterFullHookResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return queueRegisterFullHookResult;
}

/*
 * Function: gos_queueRegisterEmptyHook
 */
gos_result_t gos_queueRegisterEmptyHook (gos_queueEmptyHook emptyHook)
{
    /*
     * Local variables.
     */
    gos_result_t queueRegisterEmptyHookResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (emptyHook != NULL && queueEmptyHook == NULL)
    {
        queueEmptyHook               = emptyHook;
        queueRegisterEmptyHookResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return queueRegisterEmptyHookResult;
}

/*
 * Function: gos_queueGetName
 */
gos_result_t gos_queueGetName (gos_queueId_t queueId, gos_queueName_t queueName)
{
    /*
     * Local variables.
     */
    gos_result_t     queueGetNameResult = GOS_ERROR;
    gos_queueIndex_t queueIndex         = 0u;

    /*
     * Function code.
     */
    if (queueId                                          >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                 <  CFG_QUEUE_MAX_NUMBER &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId != GOS_INVALID_QUEUE_ID
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        if (queueName != NULL)
        {
            (void_t) strcpy(queueName, queues[queueIndex].queueName);
            queueGetNameResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return queueGetNameResult;
}

/*
 * Function: gos_queueGetElementNumber
 */
gos_result_t gos_queueGetElementNumber (gos_queueId_t queueId, gos_queueIndex_t* elementNumber)
{

    /*
     * Local variables.
     */
    gos_result_t     queueGetElementNumberResult = GOS_ERROR;
    gos_queueIndex_t queueIndex                  = 0u;

    /*
     * Function code.
     */
    if (queueId                                          >= GOS_DEFAULT_QUEUE_ID &&
        (queueId - GOS_DEFAULT_QUEUE_ID)                 <  CFG_QUEUE_MAX_NUMBER &&
        elementNumber                                    != NULL                 &&
        queues[(queueId - GOS_DEFAULT_QUEUE_ID)].queueId != GOS_INVALID_QUEUE_ID
        )
    {
        queueIndex = (gos_queueIndex_t)(queueId - GOS_DEFAULT_QUEUE_ID);

        *elementNumber = queues[queueIndex].actualElementNumber;
        queueGetElementNumberResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return queueGetElementNumberResult;
}

/*
 * Function: gos_queueDump
 */
void_t gos_queueDump (void_t)
{
    /*
     * Local variables.
     */
    gos_queueIndex_t queueIndex = 0u;

    /*
     * Function code.
     */
    (void_t) gos_shellDriverTransmitString("Queue dump:\r\n");
    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR);

#if CFG_QUEUE_USE_NAME == 1
    (void_t) gos_shellDriverTransmitString(
            "| %6s | %28s | %13s |\r\n",
            "qid",
            "name",
            "elements"
            );
#else
    (void_t) gos_shellDriverTransmitString(
            "| %6s | %28s |\r\n",
            "qid",
            "elements"
            );
#endif
    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR);

    for (queueIndex = 0u; queueIndex < CFG_QUEUE_MAX_NUMBER; queueIndex++)
    {
        if (queues[queueIndex].queueId == GOS_INVALID_QUEUE_ID)
        {
            break;
        }
        else
        {
#if CFG_QUEUE_USE_NAME == 1
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28s | %13d |\r\n",
                    queues[queueIndex].queueId,
                    queues[queueIndex].queueName,
                    queues[queueIndex].actualElementNumber
                    );
#else
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28d |\r\n",
                    queues[queueIndex].queueId,
                    queues[queueIndex].actualElementNumber
                    );
#endif
        }
    }
    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR"\n");
}
