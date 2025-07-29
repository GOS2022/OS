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
//! @file       gos_queue.h
//! @author     Ahmed Gazar
//! @date       2024-04-02
//! @version    1.6
//!
//! @brief      GOS queue service header.
//! @details    Queue service is one of the inter-task communication solutions offered by the OS.
//!             A queue can hold a given number of elements with a given element size. The queue
//!             is implemented as a FIFO. Queues are unsafe in a way that any task with the
//!             queue ID in their knowledge can put and get elements to/from the queue. So queues
//!             are most suitable for serializing data that are coming from multiple resources but
//!             processes by a single task. It is also possible to peek the queue, meaning that
//!             the next element of the queue can be requested without deleting it form the queue.
//!             This way multiple tasks can process data coming from a queue in a cooperative way,
//!             but they rely on each others data processing.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-23    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    Function descriptions updated
// 1.2        2022-11-15    Ahmed Gazar     +    Queue peek function added
//                                          +    Service description added
//                                          +    License added
// 1.3        2022-12-11    Ahmed Gazar     +    Function descriptions completed
// 1.4        2023-06-17    Ahmed Gazar     *    Queue dump moved to function
// 1.5        2023-06-30    Ahmed Gazar     +    Timeout parameter added to queue peek, put, get
// 1.6        2024-04-02    Ahmed Gazar     +    gos_queueReset added
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
#ifndef GOS_QUEUE_H
#define GOS_QUEUE_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Macros
 */
/**
 * Default queue ID.
 */
#define GOS_DEFAULT_QUEUE_ID    ( (gos_queueId_t) 0x3000 )

/**
 * Invalid queue ID.
 */
#define GOS_INVALID_QUEUE_ID    ( (gos_queueId_t) 0x0300 )

/*
 * Type definitions
 */
/**
 * One byte in a queue element.
 */
typedef u8_t   gos_queueByte_t;

#if CFG_QUEUE_MAX_LENGTH < 255
typedef u8_t   gos_queueLength_t;                              //!< Queue length type.
#else
typedef u16_t  gos_queueLength_t;                              //!< Queue length type.
#endif
#if CFG_QUEUE_MAX_NUMBER < 255 && CFG_QUEUE_MAX_LENGTH < 255
typedef u8_t   gos_queueIndex_t;                               //!< Queue index type.
#else
typedef u16_t  gos_queueIndex_t;                               //!< Queue index type.
#endif
typedef u16_t  gos_queueId_t;                                  //!< Queue ID type.

#if CFG_QUEUE_USE_NAME == 1
typedef char_t gos_queueName_t [CFG_QUEUE_MAX_NAME_LENGTH];    //!< Queue name type.
#endif

/**
 * Queue full hook type.
 */
typedef void_t (*gos_queueFullHook)(gos_queueId_t);

/**
 * Queue full hook type.
 */
typedef void_t (*gos_queueEmptyHook)(gos_queueId_t);

/**
 * Public queue descriptor type.
 */
typedef struct
{
    gos_queueId_t   queueId;   //!< Queue ID.
#if CFG_QUEUE_USE_NAME == 1
    gos_queueName_t queueName; //!< Queue name.
#endif
}gos_queueDescriptor_t;

/*
 * Function prototypes
 */
/**
 * @brief   This function initializes the queue service.
 * @details Initializes the internal queue array, creates the queue lock, and
 *          registers the queue dump task in the kernel.
 *
 * @return  Result of initialization.
 *
 * @retval  #GOS_SUCCESS Initialization successful.
 * @retval  #GOS_ERROR   Lock creation, task registration or task suspension error.
 */
gos_result_t gos_queueInit (
        void_t
        );

/**
 * @brief   This function creates a new queue.
 * @details This function loops through the internal queue array and registers the
 *          new queue in the next free slot.
 *
 * @param[in, out] pQueueDescriptor Pointer to queue descriptor variable with queue data.
 *
 * @return  Result of queue creation.
 *
 * @retval  #GOS_SUCCESS Queue creation successful.
 * @retval  #GOS_ERROR   Queue descriptor is NULL pointer or queue array is full.
 */
gos_result_t gos_queueCreate (
        gos_queueDescriptor_t* pQueueDescriptor
        );

/**
 * @brief   This function puts an element in the given queue.
 * @details This function checks the queue state and places the given element in the
 *          next queue element.
 *
 * @param[in] queueId     Queue ID.
 * @param[in] element     Pointer to element.
 * @param[in] elementSize Size of element.
 * @param[in] timeout     Timeout for locking queue mutex.
 *
 * @return  Result of element putting.
 *
 * @retval  #GOS_SUCCESS Element successfully put in the queue.
 * @retval  #GOS_ERROR   Invalid queue ID, invalid element size or queue is full.
 */
gos_result_t gos_queuePut (
        gos_queueId_t     queueId,     void_t* element,
        gos_queueLength_t elementSize, u32_t   timeout
        );

/**
 * @brief   This function gets the next element from the given queue.
 * @details This function checks the queue state and gets the next element from the queue.
 *
 * @param[in]  queueId    Queue ID.
 * @param[out] target     Pointer to target variable.
 * @param[in]  targetSize Size of target.
 * @param[in]  timeout    Timeout for locking queue mutex.
 *
 * @return  Result of element getting.
 *
 * @retval  #GOS_SUCCESS Element successfully moved from queue to target.
 * @retval  #GOS_ERROR   Invalid queue ID, invalid target size or queue is empty.
 */
gos_result_t gos_queueGet (
        gos_queueId_t     queueId,    void_t* target,
        gos_queueLength_t targetSize, u32_t   timeout
        );

/**
 * @brief   This function gets the next element from the given queue without removing it.
 * @details This function checks the queue state and returns the next element from the queue
 *          without modifying the queue counters.
 *
 * @param[in]  queueId    Queue ID.
 * @param[out] target     Pointer to target variable.
 * @param[in]  targetSize Size of target.
 * @param[in]  timeout    Timeout for locking queue mutex.
 *
 * @return  Result of element getting.
 *
 * @retval  #GOS_SUCCESS Element successfully copied from queue to target.
 * @retval  #GOS_ERROR   Invalid queue ID, invalid target size or queue is empty.
 */
gos_result_t gos_queuePeek (
        gos_queueId_t     queueId,    void_t* target,
        gos_queueLength_t targetSize, u32_t   timeout
        );

/**
 * @brief   Resets the given queue.
 * @details Sets the read and write counter to zero, making the queue empty.
 *
 * @param[in] queueId Queue ID.
 *
 * @return  Result of queue resetting.
 *
 * @retval  #GOS_SUCCESS Reset successful.
 * @retval  #GOS_ERROR   Invalid queue ID.
 */
gos_result_t gos_queueReset (gos_queueId_t queueId);

/**
 * @brief   This function registers a queue full hook function.
 * @details This function checks whether a hook has been already registered, and if not,
 *          it saves the given hook function.
 *
 * @param[in] fullHook Hook function.
 *
 * @return  Result of hook registration.
 *
 * @retval  #GOS_SUCCESS Hook registration successful.
 * @retval  #GOS_ERROR   Hook already exists or parameter is NULL pointer.
 */
gos_result_t gos_queueRegisterFullHook (
        gos_queueFullHook fullHook
        );

/**
 * @brief   This function registers a queue empty hook function.
 * @details This function checks whether a hook has been already registered, and if not,
 *          it saves the given hook function.
 *
 * @param[in] emptyHook Hook function.
 *
 * @return  Result of hook registration.
 *
 * @retval  #GOS_SUCCESS Hook registration successful.
 * @retval  #GOS_ERROR   Hook already exists or parameter is NULL pointer.
 */
gos_result_t gos_queueRegisterEmptyHook (
        gos_queueEmptyHook emptyHook
        );

/**
 * @brief   This function gets the name of the given queue.
 * @details This function copies the name of the queue belonging to the given ID to
 *          the given variable.
 *
 * @param[in]  queueId   Queue ID.
 * @param[out] queueName Queue name.
 *
 * @return  Result of queue name getting.
 *
 * @retval  #GOS_SUCCESS Name getting successful.
 * @retval  #GOS_ERROR   Invalid queue ID or queue name variable is NULL.
 */
gos_result_t gos_queueGetName (
        gos_queueId_t   queueId,
        gos_queueName_t queueName
        );

/**
 * @brief   This function gets the number of elements in the given queue.
 * @details This function copies the number of elements in the queue belonging to the given ID
 *          to the given variable.
 *
 * @param[in]  queueId       Queue ID.
 * @param[out] elementNumber Pointer to variable to store the element number in.
 *
 * @return  Result of queue element number getting.
 *
 * @retval  #GOS_SUCCESS Element number getting successful.
 * @retval  #GOS_ERROR   Invalid queue ID or element number variable is NULL.
 */
gos_result_t gos_queueGetElementNumber (
        gos_queueId_t     queueId,
        gos_queueIndex_t* elementNumber
        );

/**
 * @brief    Queue dump.
 * @details  Prints the queue data of all queues to the trace output.
 *
 * @return    -
 */
void_t gos_queueDump (
        void_t
        );

#endif
