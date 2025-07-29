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
//! @file       gos_signal.h
//! @author     Ahmed Gazar
//! @date       2022-11-15
//! @version    1.1
//!
//! @brief      GOS signal service header.
//! @details    Signal service is a way of inter-task or inter-process communication provided by
//!             the operating system. Signals can be created, subscribed to and invoked. When a
//!             signal is invoked, it is placed into an invoke queue and the signal daemon task
//!             will handle the invoke requests in the background. Thus signals are not instantly
//!             invoked. When a signal is invoked, all the subscribed functions get called. A
//!             signal can be invoked without any subscribers (in this case no function will be
//!             called).
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-23    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    License added
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
#ifndef GOS_SIGNAL_H
#define GOS_SIGNAL_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Type definitions
 */
#if CFG_SIGNAL_MAX_NUMBER < 255
//! Signal ID.
typedef u8_t  gos_signalId_t;
//! Signal sender ID.
typedef u16_t gos_signalSenderId_t;
//! Signal index for loops.
typedef u8_t  gos_signalIndex_t;
#else
//! Signal ID.
typedef u16_t gos_signalId_t;
//! Signal sender ID.
typedef u16_t gos_signalSenderId_t;
//! Signal index for loops.
typedef u16_t gos_signalIndex_t;
#endif
#if CFG_SIGNAL_MAX_SUBSCRIBERS < 255
//! Signal handler index type.
typedef u8_t  gos_signalHandlerIndex_t;
#else
//! Signal handler index type.
typedef u16_t    gos_signalHandlerIndex_t;
#endif
//! Signal handler function type.
typedef void_t (*gos_signalHandler_t)(gos_signalSenderId_t);

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the signal service.
 * @details Initializes the internal signal array, creates a signal queue,
 *          registers the signal daemon task, creates the kernel dump signal,
 *          and subscribes the necessary components for the dump signal.
 *
 * @return  -
 */
gos_result_t gos_signalInit (
        void_t
        );

/**
 * @brief   Creates a new signal.
 * @details Finds the next free slot in the signal array and
 *          registers the new signal there.
 *
 * @param[out] pSignal Pointer to a signal identifier.
 *
 * @return  Success of signal creation.
 *
 * @retval  #GOS_SUCCESS Creation successful.
 * @retval  #GOS_ERROR   Signal array full.
 */
gos_result_t gos_signalCreate (
        gos_signalId_t* pSignal
        );

/**
 * @brief   Subscribes to the given signal.
 * @details Finds the next free slot in the signal handler array and registers the
 *          signal handler there.
 *
 * @param[in] signalId                Signal identifier.
 * @param[in] signalHandler           Signal handler function pointer.
 * @param[in] signalHandlerPrivileges Signal handler privilege level.
 *
 * @return  Success of signal subscription.
 *
 * @retval  #GOS_SUCCESS Subscription successful.
 * @retval  #GOS_ERROR   Invalid signal ID, signal handler NULL pointer, or
 *                       handler array full.
 */
gos_result_t gos_signalSubscribe (
        gos_signalId_t           signalId,
        gos_signalHandler_t      signalHandler,
        gos_taskPrivilegeLevel_t signalHandlerPrivileges
        );

/**
 * @brief   Invokes the given signal.
 * @details Places the given signal in the invoke queue (for the signal daemon to
 *          actually invoke the signal in the background).
 *
 * @param[in] signalId Signal identifier.
 * @param[in] senderId Sender identifier (or data to pass).
 *
 * @return  Success of signal invoking.
 *
 * @retval  #GOS_SUCCESS Invoking successful.
 * @retval  #GOS_ERROR   Invalid signal ID or signal unused.
 */
gos_result_t gos_signalInvoke (
        gos_signalId_t       signalId,
        gos_signalSenderId_t senderId
        );

#endif
