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
//! @file       gos_message.h
//! @author     Ahmed Gazar
//! @date       2022-11-15
//! @version    1.2
//!
//! @brief      GOS message service header.
//! @details    Message service is a way of inter-task communication provided by the operating
//!             system. With the use of messages, data can be passed between different tasks. The
//!             data sent through the message service is only limited in size which is a
//!             configuration parameter. Message passing is handled by the background daemon task
//!             thus ensuring that big data transfers do not block the system. Messages are first
//!             copied into an internal message array and whenever a waiter and a message is
//!             matched, it gets copied into the target buffer. If a message is not received, it
//!             will be stuck in the internal buffer and it causes the message service to halt if
//!             the internal circular buffer gets back to the position of the stuck message.
//!             Reception of messages can happen with a given timeout or with an endless timeout.
//!             Either way the caller task will go to blocked state until the message is received
//!             or the timeout elapses, so it will not be scheduled in the meantime. Each message
//!             contains a message ID. When receiving messages, the task can define a list of IDs
//!             for reception as a filter. This way one task can receive more than one message but
//!             a message can only be received by one task.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-27    Ahmed Gazar     Initial version created
// 1.1        2022-11-05    Ahmed Gazar     +    Service description modified to match with the
//                                               FIFO implementation
// 1.2        2022-11-15    Ahmed Gazar     +    License added
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
#ifndef GOS_MESSAGE_H
#define GOS_MESSAGE_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Macros
 */
/**
 * Endless timeout.
 */
#define GOS_MESSAGE_ENDLESS_TMO     ( UINT16_MAX )

/**
 * Invalid message ID.
 */
#define GOS_MESSAGE_INVALID_ID      ( UINT16_MAX )

/*
 * Type definitions
 */
/**
 * Message identifier type.
 */
typedef u16_t gos_messageId_t;

/**
 * Message timeout type.
 */
typedef u16_t gos_messageTimeout_t;

#if CFG_MESSAGE_MAX_LENGTH < 255
typedef u8_t  gos_messageSize_t;           //!< Message size type.
#else
typedef u16_t gos_messageSize_t;           //!< Message size type.
#endif

#if CFG_MESSAGE_MAX_NUMBER < 255
typedef u8_t  gos_messageIndex_t;          //!< Message index type.
#else
typedef u16_t gos_messageIndex_t;          //!< Message index type.
#endif

#if CFG_MESSAGE_MAX_WAITERS < 255
typedef u8_t  gos_messageWaiterIndex_t;    //!< Message waiter index type.
#else
typedef u16_t gos_messageWaiterIndex_t;    //!< Message waiter index type.
#endif

#if CFG_MESSAGE_MAX_WAITER_IDS < 255
typedef u8_t  gos_messageIdIndex_t;        //!< Message ID index type.
#else
typedef u16_t gos_messageIdIndex_t;        //!< Message ID index type.
#endif

/**
 * Message type.
 */
typedef struct
{
    gos_messageId_t   messageId;                                 //!< Message ID.
    gos_messageSize_t messageSize;                               //!< Message size.
    u8_t              messageBytes [CFG_MESSAGE_MAX_LENGTH];     //!< Message bytes.
}gos_message_t;

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the message service.
 * @details Initializes the internal message and waiter arrays and registers the
 *          message daemon task.
 *
 * @return  Result of initialization.
 *
 * @retval  GOS_SUCCESS : Initialization successful.
 * @retval  GOS_ERROR   : Message daemon task registration failed.
 */
gos_result_t gos_messageInit (
        void_t
        );

/**
 * @brief   Receives the selected messages.
 * @details Based on the selected message IDs, this function receives the first available
 *          message. Until the message is received, the caller task is put to blocked state.
 *          The caller will be unblocked if the message is received or the timeout elapsed.
 *
 * @param   messageIdArray : Array of messages IDs the function should receive. Must be
 *                           by a 0 element!
 * @param   target         : Pointer to the target message structure. Received data
 *                           will be placed here.
 * @param   tmo            : Timeout value in [ms]. The resolution of timeout is 10ms!
 *                           Do not use endless timeout if it is not guaranteed that
 *                           the message will be received!
 *
 * @return    Result of message reception.
 *
 * @retval  GOS_SUCCESS    : Reception successful, data placed in the target structure.
 * @retval  GOS_ERROR      : Reception failed because of invalid parameters or timeout.
 */
gos_result_t gos_messageRx (
        gos_messageId_t*     messageIdArray,
        gos_message_t*       target,
        gos_messageTimeout_t tmo
        );

/**
 * @brief   Transmits a message.
 * @details Copies the message in the internal message array for the message daemon to
 *          transfer it to the recipient task.
 *
 * @param   message     : Pointer to the message structure to be transmitted.
 *
 * @return  Result of message transmission.
 *
 * @retval  GOS_SUCCESS : Message transmission initiated successfully.
 * @retval  GOS_ERROR   : Invalid message pointer or data or message array is full.
 */
gos_result_t gos_messageTx (
        gos_message_t* message
        );

#endif
