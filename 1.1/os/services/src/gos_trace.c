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
//! @file       gos_trace.c
//! @author     Ahmed Gazar
//! @date       2025-04-06
//! @version    1.13
//!
//! @brief      GOS trace service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_trace.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-24    Ahmed Gazar     Initial version created
// 1.1        2022-11-14    Ahmed Gazar     +    Formatted log function implemented
// 1.2        2022-11-15    Ahmed Gazar     +    License added
// 1.3        2022-12-04    Ahmed Gazar     +    Log daemon task added
// 1.4        2022-12-11    Ahmed Gazar     -    Log lock removed
// 1.5        2022-12-13    Ahmed Gazar     +    Privilege handling added
//                                          +    Initialization error logging added
// 1.6        2023-01-13    Ahmed Gazar     *    Service renamed to trace
// 1.7        2023-06-17    Ahmed Gazar     +    Optional trace timestamp added
// 1.8        2023-06-30    Ahmed Gazar     *    gos_traceTrace and gos_traceTraceFormatted
//                                               privilege check modified based on new kernel APIs
// 1.9        2023-07-12    Ahmed Gazar     +    GOS_TRACE_MUTEX_TMO_MS added
//                                          *    Privilege handling rework
// 1.10       2023-09-14    Ahmed Gazar     +    Mutex initialization result processing added
// 1.11       2023-10-04    Ahmed Gazar     *    Trace timestamp: milliseconds added
// 1.12       2024-03-08    Ahmed Gazar     -    GOS_TRACE_DAEMON_POLL_TIME_MS removed
// 1.13       2025-04-06    Ahmed Gazar     *    Initializer result logic inverted
//                                          *    GOS_CONCAT_RESULT usage added
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
#include <gos_trace.h>
#include <gos_mutex.h>
#include <gos_queue.h>
#include <gos_time.h>
#include <gos_trace_driver.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
/*
 * Macros
 */
/**
 * Trace timestamp formatter.
 */
#define GOS_TRACE_TIMESTAMP_FORMAT       "["TRACE_FG_YELLOW_START"%04d-%02d-%02d %02d:%02d:%02d.%03d"TRACE_FORMAT_RESET"]\t"

/**
 * Trace timestamp length.
 */
#define GOS_TRACE_TIMESTAMP_LENGTH       ( 46u )

/**
 * Timeout value in [ms] for queue operations.
 */
#define GOS_TRACE_QUEUE_TMO_MS           ( 2000u )

/**
 * Timeout value in [ms] for mutex operations.
 */
#define GOS_TRACE_MUTEX_TMO_MS           ( 2000u )

/*
 * Static variables
 */
/**
 * Trace queue.
 */
GOS_STATIC gos_queueDescriptor_t traceQueue =
{
#if CFG_QUEUE_USE_NAME == 1
    .queueName = "gos_trace_queue"
#endif
};

/**
 * Trace line buffer.
 */
GOS_STATIC char_t traceLine       [CFG_TRACE_MAX_LENGTH];

/**
 * Trace formatted buffer for message formatting.
 */
GOS_STATIC char_t formattedBuffer [CFG_TRACE_MAX_LENGTH];

/**
 * Buffer for timestamp printing.
 */
GOS_STATIC char_t timeStampBuffer [GOS_TRACE_TIMESTAMP_LENGTH];

/**
 * Trace mutex.
 */
GOS_STATIC gos_mutex_t traceMutex;

/*
 * Function prototypes
 */
GOS_STATIC void_t gos_traceDaemonTask (void_t);

/**
 * Trace daemon task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t traceDaemonTaskDesc =
{
    .taskFunction        = gos_traceDaemonTask,
    .taskName            = "gos_trace_daemon",
    .taskPriority        = CFG_TASK_TRACE_DAEMON_PRIO,
    .taskStackSize       = CFG_TASK_TRACE_DAEMON_STACK,
    .taskPrivilegeLevel  = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Function: gos_traceInit
 */
gos_result_t gos_traceInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t traceInitResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Create trace queue and register trace daemon task.
    if (gos_queueCreate(&traceQueue)                 == GOS_SUCCESS &&
        gos_taskRegister(&traceDaemonTaskDesc, NULL) == GOS_SUCCESS &&
        gos_mutexInit(&traceMutex) == GOS_SUCCESS
        )
    {
        traceInitResult = GOS_SUCCESS;
    }

    return traceInitResult;
}

/*
 * Function: gos_traceTrace
 */
GOS_INLINE gos_result_t gos_traceTrace (bool_t addTimeStamp, char_t* traceMessage)
{
    /*
     * Local variables.
     */
    gos_result_t             traceResult    = GOS_ERROR;
    gos_tid_t                callerTaskId   = GOS_INVALID_TASK_ID;
    gos_taskPrivilegeLevel_t privileges     = 0u;
    gos_taskDescriptor_t     callerTaskDesc = {0};
    gos_time_t               sysTime        = {0};

    /*
     * Function code.
     */
    if ((traceMessage                                          != NULL           ) &&
        (gos_taskGetCurrentId(&callerTaskId)                   == GOS_SUCCESS    ) &&
        (gos_taskGetData(callerTaskId, &callerTaskDesc)        == GOS_SUCCESS    ) &&
        (((callerTaskDesc.taskPrivilegeLevel & GOS_PRIV_TRACE) == GOS_PRIV_TRACE ) ||
        (gos_kernelIsCallerIsr()                               == GOS_TRUE       )) &&
        (gos_mutexLock(&traceMutex, GOS_TRACE_MUTEX_TMO_MS)    == GOS_SUCCESS    )
        )
    {
        // Pre-set result.
        traceResult = GOS_SUCCESS;

        if (addTimeStamp == GOS_TRUE)
        {
            GOS_CONCAT_RESULT(traceResult, gos_timeGet(&sysTime));
            (void_t) sprintf(timeStampBuffer, GOS_TRACE_TIMESTAMP_FORMAT,
                    sysTime.years,
                    sysTime.months,
                    sysTime.days,
                    sysTime.hours,
                    sysTime.minutes,
                    sysTime.seconds,
                    sysTime.milliseconds
                    );
            // Add timestamp to queue.
            GOS_CONCAT_RESULT(traceResult, gos_queuePut(
                    traceQueue.queueId,
                    (void_t*)timeStampBuffer,
                    strlen(timeStampBuffer) + 1,
                    GOS_TRACE_QUEUE_TMO_MS
                    ));
        }
        else
        {
            // Timestamp not needed.
        }

        // Add trace message to queue.
        GOS_CONCAT_RESULT(traceResult, gos_queuePut(
                traceQueue.queueId,
                (void_t*)traceMessage,
                strlen(traceMessage) + 1,
                GOS_TRACE_QUEUE_TMO_MS
                ));

        GOS_ATOMIC_ENTER

        // Get task privileges.
        (void_t) gos_taskGetPrivileges(callerTaskId, &privileges);

        if ((privileges & GOS_PRIV_TASK_MANIPULATE) != GOS_PRIV_TASK_MANIPULATE)
        {
            // If task has no privilege to manipulate other tasks, add it temporarily.
            (void_t) gos_taskAddPrivilege(callerTaskId, GOS_PRIV_TASK_MANIPULATE);
            (void_t) gos_taskResume(traceDaemonTaskDesc.taskId);
            (void_t) gos_taskRemovePrivilege(callerTaskId, GOS_PRIV_TASK_MANIPULATE);
        }
        else
        {
            (void_t) gos_taskResume(traceDaemonTaskDesc.taskId);
        }
        GOS_ATOMIC_EXIT

        (void_t) gos_mutexUnlock(&traceMutex);
    }
    else
    {
        // Nothing to do.
    }

    return traceResult;
}

/*
 * Function: gos_traceTraceFormatted
 */
gos_result_t gos_traceTraceFormatted (bool_t addTimeStamp, GOS_CONST char_t* traceFormat, ...)
{
    /*
     * Local variables.
     */
    gos_result_t             traceResult    = GOS_SUCCESS;
    gos_tid_t                callerTaskId   = GOS_INVALID_TASK_ID;
    gos_taskDescriptor_t     callerTaskDesc = {0};
    gos_time_t               sysTime        = {0};
    va_list                  args;
    gos_taskPrivilegeLevel_t privileges     = 0u;

    /*
     * Function code.
     */
    if ((traceFormat                                           != NULL           ) &&
        (gos_taskGetCurrentId(&callerTaskId)                   == GOS_SUCCESS    ) &&
        (gos_taskGetData(callerTaskId, &callerTaskDesc)        == GOS_SUCCESS    ) &&
        (((callerTaskDesc.taskPrivilegeLevel & GOS_PRIV_TRACE) == GOS_PRIV_TRACE ) ||
        (gos_kernelIsCallerIsr()                               == GOS_TRUE       )) &&
        (gos_mutexLock(&traceMutex, GOS_TRACE_MUTEX_TMO_MS)    == GOS_SUCCESS    )
        )
    {
        if (addTimeStamp == GOS_TRUE)
        {
            GOS_CONCAT_RESULT(traceResult, gos_timeGet(&sysTime));
            (void_t) sprintf(timeStampBuffer, GOS_TRACE_TIMESTAMP_FORMAT,
                    sysTime.years,
                    sysTime.months,
                    sysTime.days,
                    sysTime.hours,
                    sysTime.minutes,
                    sysTime.seconds,
                    sysTime.milliseconds
                    );
            // Add timestamp to queue.
            GOS_CONCAT_RESULT(traceResult, gos_queuePut(
                    traceQueue.queueId,
                    (void_t*)timeStampBuffer,
                    strlen(timeStampBuffer) + 1,
                    GOS_TRACE_QUEUE_TMO_MS
                    ));
        }
        else
        {
            // Timestamp not needed.
        }

        // Add trace message to queue.
        va_start(args, traceFormat);
        (void_t) vsprintf(formattedBuffer, traceFormat, args);
        va_end(args);
        GOS_CONCAT_RESULT(traceResult, gos_queuePut(
                traceQueue.queueId,
                (void_t*)formattedBuffer,
                strlen(formattedBuffer) + 1,
                GOS_TRACE_QUEUE_TMO_MS
                ));

        GOS_ATOMIC_ENTER

        // Get task privileges.
        (void_t) gos_taskGetPrivileges(callerTaskId, &privileges);

        if ((privileges & GOS_PRIV_TASK_MANIPULATE) != GOS_PRIV_TASK_MANIPULATE)
        {
            // If task has no privilege to manipulate other tasks, add it temporarily.
            (void_t) gos_taskAddPrivilege(callerTaskId, GOS_PRIV_TASK_MANIPULATE);
            (void_t) gos_taskResume(traceDaemonTaskDesc.taskId);
            (void_t) gos_taskRemovePrivilege(callerTaskId, GOS_PRIV_TASK_MANIPULATE);
        }
        else
        {
            (void_t) gos_taskResume(traceDaemonTaskDesc.taskId);
        }
        GOS_ATOMIC_EXIT

        (void_t) gos_mutexUnlock(&traceMutex);
    }
    else
    {
        traceResult = GOS_ERROR;
    }

    return traceResult;
}

/*
 * Function: gos_traceTraceFormattedUnsafe
 */
gos_result_t gos_traceTraceFormattedUnsafe (GOS_CONST char_t* traceFormat, ...)
{
    /*
     * Local variables.
     */
    gos_result_t traceResult = GOS_ERROR;
    va_list      args;

    /*
     * Function code.
     */
    if (traceFormat != NULL)
    {
        GOS_DISABLE_SCHED
        {
            va_start(args, traceFormat);
            (void_t) vsprintf(formattedBuffer, traceFormat, args);
            va_end(args);

            traceResult = gos_traceDriverTransmitString_Unsafe(formattedBuffer);
        }
        GOS_ENABLE_SCHED
    }
    else
    {
        // Nothing to do.
    }

    return traceResult;
}

/**
 * @brief   Trace daemon task.
 * @details Polls the trace queue and transmits the elements in the trace queue
 *          via the registered trace driver.
 *
 * @return  -
 */
GOS_STATIC void_t gos_traceDaemonTask (void_t)
{
    /*
     * Function code.
     */
    for (;;)
    {
        if (gos_mutexLock(&traceMutex, GOS_TRACE_MUTEX_TMO_MS) == GOS_SUCCESS)
        {
            while (gos_queueGet(
                    traceQueue.queueId,
                    (void_t*)traceLine,
                    sizeof(traceLine),
                    GOS_TRACE_QUEUE_TMO_MS) == GOS_SUCCESS)
            {
                (void_t) gos_traceDriverTransmitString(traceLine);
            }
        }
        else
        {
            // Nothing to do.
        }
        (void_t) gos_mutexUnlock(&traceMutex);
        (void_t) gos_taskSuspend(traceDaemonTaskDesc.taskId);
    }
}
