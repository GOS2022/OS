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
//! @file       gos_signal.c
//! @author     Ahmed Gazar
//! @date       2023-10-04
//! @version    1.8
//!
//! @brief      GOS signal service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_signal.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-23    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    License added
// 1.2        2022-12-04    Ahmed Gazar     +    Kernel dump ready signal handler added
// 1.3        2022-12-15    Ahmed Gazar     +    Initialization error logging added
//                                          +    Privilege handling added
// 1.4        2023-05-04    Ahmed Gazar     -    Lock dump signal removed
// 1.5        2023-06-17    Ahmed Gazar     *    Service rework, optimizations, queue use removed,
//                                               unnecessary system signals removed
// 1.6        2023-06-30    Ahmed Gazar     +    Dump ready signal added back
//                                          -    signalDaemonTaskId removed
// 1.7        2023-07-12    Ahmed Gazar     +    Signal handler privilege-handling added
// 1.8        2023-10-04    Ahmed Gazar     *    Signal daemon polling replaced by async unblocking
//                                          -    GOS_SIGNAL_DAEMON_POLL_TIME_MS removed
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
#include <gos_signal.h>
#include <gos_error.h>
#include <gos_queue.h>
#include <gos_trigger.h>
#include <string.h>

/*
 * Type definitions
 */
/**
 * Signal descriptor type.
 */
typedef struct
{
    bool_t                   inUse;                                         //!< Flag to indicate whether the signal is in use.
    gos_signalHandler_t      handlers         [CFG_SIGNAL_MAX_SUBSCRIBERS]; //!< Signal handler array.
    gos_taskPrivilegeLevel_t handlerPrvileges [CFG_SIGNAL_MAX_SUBSCRIBERS]; //!< Signal handler privileges array.
    bool_t                   invokeRequired;                                //!< Invoke required flag.
    gos_signalSenderId_t     senderId;                                      //!< Sender ID.
}gos_signalDescriptor_t;

/**
 * Signal invoke descriptor type.
 */
typedef struct
{
    gos_signalId_t           signalId;                                      //!< Signal ID.
    gos_signalSenderId_t     senderId;                                      //!< Sender ID.
}gos_signalInvokeDescriptor;

/*
 * Static variables
 */
/**
 * Internal signal descriptor array.
 */
GOS_STATIC gos_signalDescriptor_t signalArray [CFG_SIGNAL_MAX_NUMBER];

/**
 * Invoke trigger (to count the number of signal invokings).
 */
GOS_STATIC gos_trigger_t          signalInvokeTrigger;

/*
 * External variables
 */
GOS_EXTERN gos_signalId_t kernelTaskDeleteSignal;
GOS_EXTERN gos_signalId_t kernelDumpReadySignal;

/*
 * Function prototypes
 */
GOS_STATIC void_t gos_signalDaemonTask (void_t);

/**
 * Signal daemon task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t signalDaemonTaskDescriptor =
{
    .taskFunction        = gos_signalDaemonTask,
    .taskName            = "gos_signal_daemon",
    .taskStackSize       = CFG_TASK_SIGNAL_DAEMON_STACK,
    .taskPriority        = CFG_TASK_SIGNAL_DAEMON_PRIO,
    .taskPrivilegeLevel  = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Function: gos_signalInit
 */
gos_result_t gos_signalInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t      signalInitResult = GOS_SUCCESS;
    gos_signalIndex_t signalIndex      = 0u;

    /*
     * Function code.
     */
    // Reset the in-use flag for every signal instance.
    for (signalIndex = 0u; signalIndex < CFG_SIGNAL_MAX_NUMBER; signalIndex++)
    {
        signalArray[signalIndex].inUse = GOS_FALSE;
    }

    // Register signal daemon and create kernel task delete signal.
    if (gos_taskRegister(&signalDaemonTaskDescriptor, NULL) != GOS_SUCCESS ||
        gos_signalCreate(&kernelTaskDeleteSignal)           != GOS_SUCCESS ||
        gos_signalCreate(&kernelDumpReadySignal)            != GOS_SUCCESS ||
        gos_triggerInit(&signalInvokeTrigger)               != GOS_SUCCESS
    )
    {
        signalInitResult = GOS_ERROR;
    }
    else
    {
        // Nothing to do.
    }

    return signalInitResult;
}

/*
 * Function: gos_signalCreate
 */
gos_result_t gos_signalCreate (gos_signalId_t* pSignal)
{
    /*
     * Local variables.
     */
    gos_result_t      signalCreateResult = GOS_ERROR;
    gos_signalIndex_t signalIndex        = 0u;

    /*
     * Function code.
     */
    // Find the next unused signal and reserve it.
    for (signalIndex = 0u; signalIndex < CFG_SIGNAL_MAX_NUMBER; signalIndex++)
    {
        if (signalArray[signalIndex].inUse == GOS_FALSE && pSignal != NULL)
        {
            *pSignal                                = signalIndex;
            signalArray[signalIndex].inUse          = GOS_TRUE;
            signalArray[signalIndex].invokeRequired = GOS_FALSE;
            signalCreateResult                      = GOS_SUCCESS;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return signalCreateResult;
}

/*
 * Function: gos_signalSubscribe
 */
gos_result_t gos_signalSubscribe (
        gos_signalId_t           signalId,
        gos_signalHandler_t      signalHandler,
        gos_taskPrivilegeLevel_t signalHandlerPrivileges
        )
{
    /*
     * Local variables.
     */
    gos_result_t             signalSubscribeResult = GOS_ERROR;
    gos_signalHandlerIndex_t signalHandlerIndex    = 0u;

    /*
     * Function code.
     */
    if (signalId < CFG_SIGNAL_MAX_NUMBER && signalArray[signalId].inUse == GOS_TRUE && signalHandler != NULL)
    {
        for (signalHandlerIndex = 0u; signalHandlerIndex < CFG_SIGNAL_MAX_SUBSCRIBERS; signalHandlerIndex++)
        {
            if (signalArray[signalId].handlers[signalHandlerIndex] == NULL)
            {
                signalArray[signalId].handlers[signalHandlerIndex]         = signalHandler;
                signalArray[signalId].handlerPrvileges[signalHandlerIndex] = signalHandlerPrivileges;
                signalSubscribeResult                                      = GOS_SUCCESS;
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

    return signalSubscribeResult;
}

/*
 * Function: gos_signalInvoke
 */
GOS_INLINE gos_result_t gos_signalInvoke (gos_signalId_t signalId, gos_signalSenderId_t senderId)
{
    /*
     * Local variables.
     */
    gos_result_t         signalInvokeResult = GOS_ERROR;
    gos_tid_t            callerTaskId       = GOS_INVALID_TASK_ID;
    gos_taskDescriptor_t callerTaskDesc     = {0};

    /*
     * Function code.
     */
    if (signalId < CFG_SIGNAL_MAX_NUMBER && signalArray[signalId].inUse == GOS_TRUE)
    {
        if ((gos_kernelIsCallerIsr()                                 == GOS_TRUE    ||
            (gos_taskGetCurrentId(&callerTaskId)                     == GOS_SUCCESS &&
            gos_taskGetData(callerTaskId, &callerTaskDesc)           == GOS_SUCCESS &&
            (callerTaskDesc.taskPrivilegeLevel & GOS_PRIV_SIGNALING) == GOS_PRIV_SIGNALING))
            )
        {
            signalArray[signalId].senderId       = senderId;
            signalArray[signalId].invokeRequired = GOS_TRUE;
            signalInvokeResult                   = GOS_SUCCESS;

            // Unblock signal daemon to handle signal invoking by
            // incrementing the invoke trigger.
            (void_t) gos_triggerIncrement(&signalInvokeTrigger);
        }
        else
        {
            gos_errorHandler(GOS_ERROR_LEVEL_OS_WARNING, __func__, __LINE__, "<%s> has no privilege to invoke signals!",
                    callerTaskDesc.taskName
            );
        }
    }
    else
    {
        // Nothing to do.
    }

    return signalInvokeResult;
}

/**
 * @brief   Signal daemon task.
 * @details Polls the signal invoke queue, and completes the necessary signal invokings.
 *
 * @return -
 */
GOS_STATIC void_t gos_signalDaemonTask (void_t)
{
    /*
     * Local variables.
     */
    gos_signalHandlerIndex_t signalHandlerIndex = 0u;
    gos_signalIndex_t        signalIndex        = 0u;

    /*
     * Function code.
     */
    for (;;)
    {
        // Wait for trigger.
        if (gos_triggerWait(&signalInvokeTrigger, 1u, GOS_TRIGGER_ENDLESS_TMO) == GOS_SUCCESS)
        {
            (void_t) gos_triggerReset(&signalInvokeTrigger);
            for (signalIndex = 0u; signalIndex < CFG_SIGNAL_MAX_NUMBER; signalIndex++)
            {
                if (signalArray[signalIndex].invokeRequired == GOS_TRUE)
                {
                    for (signalHandlerIndex = 0u; signalHandlerIndex < CFG_SIGNAL_MAX_SUBSCRIBERS; signalHandlerIndex++)
                    {
                        if (signalArray[signalIndex].handlers[signalHandlerIndex] == NULL)
                        {
                            // Last handler called, stop calling.
                            break;
                        }
                        else
                        {
                            // Switch to signal handler privilege.
                            (void_t) gos_taskSetPrivileges(
                                    signalDaemonTaskDescriptor.taskId,
                                    signalArray[signalIndex].handlerPrvileges[signalHandlerIndex]
                                    );
                            // Call signal handler.
                            signalArray[signalIndex].handlers[signalHandlerIndex](signalArray[signalIndex].senderId);

                            // Switch back to kernel privilege.
                            (void_t) gos_taskSetPrivileges(signalDaemonTaskDescriptor.taskId, GOS_TASK_PRIVILEGE_KERNEL);
                        }
                    }
                    signalArray[signalIndex].invokeRequired = GOS_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else
        {
            // Unexpected error.
        }
    }
}
