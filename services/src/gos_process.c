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
//! @file       gos_process.c
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.6
//!
//! @brief      GOS process service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_process.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-22    Ahmed Gazar     Initial version created
// 1.1        2022-10-24    Ahmed Gazar     +    Dump mechanism changed: it is now handled by a
//                                               dedicated task (gos_procDumpTask)
// 1.2        2022-11-14    Ahmed Gazar     +    If service is off, dump signal handler invokes the
//                                               signal with its own sender ID
// 1.3        2022-11-15    Ahmed Gazar     +    License added
// 1.4        2023-06-17    Ahmed Gazar     *    Process dump moved to function
//                                          +    System process added for statistics calculation
// 1.5        2023-06-30    Ahmed Gazar     *    PROC_SYS_POLL_TIME -> PROC_SYS_POLL_TIME_MS
//                                          *    PROC_DAEMON_POLL_TIME -> PROC_DAEMON_POLL_TIME_MS
// 1.6        2023-07-12    Ahmed Gazar     +    System process forced scheduling added for the
//                                               case of an overloaded system
//                                          *    Process privilege handling rework
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
#include <gos_process.h>
#include <gos_signal.h>
#include <gos_shell_driver.h>
#include <gos_time.h>
#include <gos_timer_driver.h>
#include <gos_trace.h>
#include <stdio.h>
#include <string.h>
#if CFG_PROC_USE_SERVICE == 1
/*
 * Macros
 */
/**
 * Dump separator line.
 */
#define DUMP_SEPARATOR           "+--------+------------------------------+---------------+---------+-----------+\r\n"

/**
 * Process daemon poll time [ms].
 */
#define PROC_DAEMON_POLL_TIME_MS ( 20u )

/**
 * System process poll time [ms].
 */
#define PROC_SYS_POLL_TIME_MS    ( 500u )

/*
 * Type definitions
 */
#if CFG_PROC_MAX_NUMBER < 255
typedef u8_t  gos_procIndex_t;    //!< Process index type.
#else
typedef u16_t gos_procIndex_t;    //!< Process index type.
#endif

/*
 * Static variables
 */
/**
 * Current process.
 */
GOS_STATIC u32_t                 currentProcIndex        = 0u;

/**
 * Process idle hook function.
 */
GOS_STATIC gos_procIdleHook_t    procIdleHookFunction    = NULL;

/**
 * Process swap hook function.
 */
GOS_STATIC gos_procSwapHook_t    procSwapHookFunction    = NULL;

/**
 * Process resume hook function.
 */
GOS_STATIC gos_procResumeHook_t  procResumeHookFunction  = NULL;

/**
 * Process sleep hook function.
 */
GOS_STATIC gos_procSleepHook_t   procSleepHookFunction   = NULL;

/**
 * Process suspend hook function.
 */
GOS_STATIC gos_procSuspendHook_t procSuspendHookFunction = NULL;

/**
 * Process wake-up hook function.
 */
GOS_STATIC gos_procWakeupHook_t  procWakeupHookFunction  = NULL;

/**
 * Process daemon task ID.
 */
GOS_STATIC gos_tid_t             procDaemonTaskId        = GOS_INVALID_TASK_ID;

/**
 * Monitoring process run-time for statistics.
 */
GOS_STATIC gos_runtime_t         monitoringTime          = {0};

/**
 * System process last run tick value.
 */
GOS_STATIC u32_t                 systemProcLastRunTick   = 0u;

/*
 * Function prototypes
 */
GOS_STATIC gos_result_t gos_procCheckProcDescriptor (gos_procDescriptor_t* procDescriptor);
GOS_STATIC void_t       gos_idleProc                (void_t);
GOS_STATIC void_t       gos_systemProc              (void_t);
GOS_STATIC void_t       gos_procDaemonTask          (void_t);
GOS_STATIC char_t*      gos_procGetProcStateString  (gos_procState_t procState);

/**
 * Internal process array.
 */
GOS_STATIC gos_procDescriptor_t procDescriptors [CFG_PROC_MAX_NUMBER] = {
    [0] =
        {
            .procFunction   = gos_idleProc,
            .procState      = GOS_PROC_READY,
            .procId         = GOS_DEFAULT_PROC_ID,
            .procPriority   = CFG_PROC_IDLE_PRIO,
            .procName       = "gos_idle_proc"
        },
    [1] =
        {
            .procFunction   = gos_systemProc,
            .procState      = GOS_PROC_READY,
            .procId         = GOS_DEFAULT_PROC_ID + 1,
            .procPriority   = CFG_PROC_IDLE_PRIO - 1,
            .procName       = "gos_system_proc"
        }
};

/**
 * Process daemon task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t processDaemonTaskDesc =
{
    .taskFunction        = gos_procDaemonTask,
    .taskName            = "gos_process_daemon",
    .taskStackSize       = CFG_TASK_PROC_DAEMON_STACK,
    .taskPriority        = CFG_TASK_PROC_DAEMON_PRIO,
    .taskPrivilegeLevel  = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Function: gos_procInit
 */
gos_result_t gos_procInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t    initResult     = GOS_ERROR;
    gos_procIndex_t procIndex    = 1u;

    /*
     * Function code.
     */
    // Initialize process descriptors.
    for (procIndex = 2u; procIndex < CFG_PROC_MAX_NUMBER; procIndex++)
    {
        procDescriptors[procIndex].procFunction = NULL;
        procDescriptors[procIndex].procPriority = CFG_PROC_MAX_PRIO_LEVELS;
        procDescriptors[procIndex].procState    = GOS_PROC_SUSPENDED;
        procDescriptors[procIndex].procId       = GOS_INVALID_PROC_ID;
    }

    if (gos_kernelTaskRegister(&processDaemonTaskDesc, &procDaemonTaskId) == GOS_SUCCESS)
    {
        initResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return initResult;
}

/*
 * Function: gos_procRegister
 */
gos_result_t gos_procRegister (gos_procDescriptor_t* procDescriptor, gos_pid_t* procId)
{
    /*
     * Local variables.
     */
    gos_result_t    procRegisterResult = GOS_SUCCESS;
    gos_procIndex_t procIndex          = 0u;

    /*
     * Function code.
     */
    // Check descriptor structure.
    if (gos_procCheckProcDescriptor(procDescriptor) == GOS_ERROR)
    {
        procRegisterResult = GOS_ERROR;
    }
    else
    {
        // Find empty slot.
        for (procIndex = 1u; procIndex < CFG_PROC_MAX_NUMBER; procIndex++)
        {
            if (procDescriptors[procIndex].procFunction == NULL)
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        // Check if empty slot was found.
        if (procIndex >= CFG_PROC_MAX_NUMBER)
        {
            procRegisterResult = GOS_ERROR;
        }
        else
        {
            // Initial state.
            procDescriptors[procIndex].procState    = GOS_PROC_READY;
            procDescriptors[procIndex].procFunction = procDescriptor->procFunction;
            procDescriptors[procIndex].procPriority = procDescriptor->procPriority;
            procDescriptors[procIndex].procId       = (GOS_DEFAULT_PROC_ID + procIndex);

            // Copy process name.
            if (procDescriptor->procName != NULL &&
                strlen(procDescriptor->procName) <= CFG_PROC_MAX_NAME_LENGTH)
            {
                (void_t) strcpy(procDescriptors[procIndex].procName, procDescriptor->procName);
            }
            else
            {
                // Nothing to do.
            }

            // Set process ID.
            procDescriptor->procId = procDescriptors[procIndex].procId;
            if (procId != NULL)
            {
                *procId = procDescriptors[procIndex].procId;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return procRegisterResult;
}

/*
 * Function: gos_procSleep
 */
gos_result_t gos_procSleep (gos_procSleepTick_t sleepTicks)
{
    /*
     * Local variables.
     */
    gos_result_t procSleepResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (currentProcIndex > 0u)
    {
        GOS_DISABLE_SCHED
        {
            if (procDescriptors[currentProcIndex].procState == GOS_PROC_READY)
            {
                procDescriptors[currentProcIndex].procState      = GOS_PROC_SLEEPING;
                procDescriptors[currentProcIndex].procSleepTicks = gos_kernelGetSysTicks() + sleepTicks;
                procSleepResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        GOS_ENABLE_SCHED

        // Call hook.
        if (procSleepResult == GOS_SUCCESS && procSleepHookFunction != NULL)
        {
            procSleepHookFunction((GOS_DEFAULT_PROC_ID + currentProcIndex));
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

    return procSleepResult;
}

/*
 * Function: gos_procWakeup
 */
gos_result_t gos_procWakeup (gos_pid_t procId)
{
    /*
     * Local variables.
     */
    gos_result_t    procWakeupResult = GOS_ERROR;
    gos_procIndex_t procIndex        = 0u;

    /*
     * Function code.
     */
    if (procId > GOS_DEFAULT_PROC_ID && (procId - GOS_DEFAULT_PROC_ID) < CFG_PROC_MAX_NUMBER)
    {
        procIndex = (u32_t)(procId - GOS_DEFAULT_PROC_ID);

        GOS_DISABLE_SCHED
        {
            if (procDescriptors[procIndex].procState == GOS_PROC_SLEEPING)
            {
                procDescriptors[procIndex].procState = GOS_PROC_READY;
                procWakeupResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        GOS_ENABLE_SCHED

        // Call hook.
        if (procWakeupResult == GOS_SUCCESS && procWakeupHookFunction != NULL)
        {
            procWakeupHookFunction(procId);
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

    return procWakeupResult;
}

/*
 * Function: gos_procSuspend
 */
gos_result_t gos_procSuspend (gos_pid_t procId)
{
    /*
     * Local variables.
     */
    gos_result_t    procSuspendResult = GOS_ERROR;
    gos_procIndex_t procIndex         = 0u;

    /*
     * Function code.
     */
    if (procId > GOS_DEFAULT_PROC_ID && (procId - GOS_DEFAULT_PROC_ID) < CFG_PROC_MAX_NUMBER)
    {
        procIndex = (u32_t)(procId - GOS_DEFAULT_PROC_ID);

        GOS_DISABLE_SCHED
        {
            if (procDescriptors[procIndex].procState == GOS_PROC_READY ||
                procDescriptors[procIndex].procState == GOS_PROC_SLEEPING)
            {
                procDescriptors[procIndex].procState = GOS_PROC_SUSPENDED;
                procSuspendResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        GOS_ENABLE_SCHED

        // Call hook.
        if (procSuspendResult == GOS_SUCCESS && procSuspendHookFunction != NULL)
        {
            procSuspendHookFunction(procId);
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

    return procSuspendResult;
}

/*
 * Function: gos_procResume
 */
gos_result_t gos_procResume (gos_pid_t procId)
{
    /*
     * Local variables.
     */
    gos_result_t    procResumeResult = GOS_ERROR;
    gos_procIndex_t procIndex        = 0u;

    /*
     * Function code.
     */
    if (procId > GOS_DEFAULT_PROC_ID && (procId - GOS_DEFAULT_PROC_ID) < CFG_PROC_MAX_NUMBER)
    {
        procIndex = (u32_t)(procId - GOS_DEFAULT_PROC_ID);

        GOS_DISABLE_SCHED
        {
            if (procDescriptors[procIndex].procState == GOS_PROC_SUSPENDED)
            {
                procDescriptors[procIndex].procState = GOS_PROC_READY;
                procResumeResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        GOS_ENABLE_SCHED

        // Call hook.
        if (procResumeResult == GOS_SUCCESS && procResumeHookFunction != NULL)
        {
            procResumeHookFunction(procId);
        }
        else
        {
            // Nothing to do.
        }
    }

    return procResumeResult;
}

/*
 * Function: gos_procGetName
 */
gos_result_t gos_procGetName (gos_pid_t procId, gos_procName_t procName)
{
    /*
     * Local variables.
     */
    gos_result_t    procGetNameResult = GOS_ERROR;
    gos_procIndex_t procIndex         = 0u;

    /*
     * Function code.
     */
    if (procId > GOS_DEFAULT_PROC_ID && (procId - GOS_DEFAULT_PROC_ID) < CFG_PROC_MAX_NUMBER &&
        procName != NULL)
    {
        procIndex = (u32_t)(procId - GOS_DEFAULT_PROC_ID);

        (void_t) strcpy(procName, procDescriptors[procIndex].procName);

        procGetNameResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return procGetNameResult;
}

/*
 * Function: gos_procGetId
 */
gos_result_t gos_procGetId (gos_procName_t procName, gos_pid_t* procId)
{
    /*
     * Local variables.
     */
    gos_result_t    procGetIdResult = GOS_ERROR;
    gos_procIndex_t procIndex       = 0u;

    /*
     * Function code.
     */
    for (procIndex = 0u; procIndex < CFG_PROC_MAX_NUMBER && procId != NULL; procIndex++)
    {
        if (strcmp(procName, procDescriptors[procIndex].procName) == 0u)
        {
            *procId = procDescriptors[procIndex].procId;
            procGetIdResult = GOS_SUCCESS;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return procGetIdResult;
}

/*
 * Function: gos_procGetData
 */
gos_result_t gos_procGetData (gos_pid_t procId, gos_procDescriptor_t* procData)
{
    /*
     * Local variables.
     */
    gos_result_t    procGetDataResult = GOS_ERROR;
    gos_procIndex_t procIndex         = 0u;

    /*
     * Function code.
     */
    if (procId >= GOS_DEFAULT_PROC_ID && (procId - GOS_DEFAULT_PROC_ID) < CFG_PROC_MAX_NUMBER &&
        procData != NULL)
    {
        procIndex = (u32_t)(procId - GOS_DEFAULT_PROC_ID);

        (void_t) memcpy((void*)procData, (void*)&procDescriptors[procIndex], sizeof(*procData));

        procGetDataResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return procGetDataResult;
}

/*
 * Function: gos_procRegisterSwapHook
 */
gos_result_t gos_procRegisterSwapHook (gos_procSwapHook_t swapHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (swapHookFunction != NULL && procSwapHookFunction == NULL)
    {
        procSwapHookFunction = swapHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procRegisterIdleHook
 */
gos_result_t gos_procRegisterIdleHook (gos_procIdleHook_t idleHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (idleHookFunction != NULL && procIdleHookFunction == NULL)
    {
        procIdleHookFunction = idleHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procRegisterSleepHook
 */
gos_result_t gos_procRegisterSleepHook (gos_procSleepHook_t sleepHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (sleepHookFunction != NULL && procSleepHookFunction == NULL)
    {
        procSleepHookFunction = sleepHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procRegisterWakeupHook
 */
gos_result_t gos_procRegisterWakeupHook (gos_procWakeupHook_t wakeupHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (wakeupHookFunction != NULL && procWakeupHookFunction == NULL)
    {
        procWakeupHookFunction = wakeupHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procRegisterSuspendHook
 */
gos_result_t gos_procRegisterSuspendHook (gos_procSuspendHook_t suspendHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (suspendHookFunction != NULL && procSuspendHookFunction == NULL)
    {
        procSuspendHookFunction = suspendHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procRegisterResumeHook
 */
gos_result_t gos_procRegisterResumeHook (gos_procResumeHook_t resumeHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (resumeHookFunction != NULL && procResumeHookFunction == NULL)
    {
        procResumeHookFunction = resumeHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_procDump
 */
void_t gos_procDump (void_t)
{
    /*
     * Local variables.
     */
    u16_t procIndex = 0u;

    /*
     * Function code.
     */
    (void_t) gos_shellDriverTransmitString("Process dump:\r\n");
    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR);
    (void_t) gos_shellDriverTransmitString(
            "| %6s | %28s | %13s | %7s | %9s |\r\n",
            "pid",
            "name",
            "prio",
            "[%]",
            "state"
        );

    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR);

    for (procIndex = 0u; procIndex < CFG_PROC_MAX_NUMBER; procIndex++)
    {
        if (procDescriptors[procIndex].procFunction == NULL)
        {
            break;
        }
        else
        {
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28s | %13d | %4u.%02u | %9s "TRACE_FORMAT_RESET"|\r\n",
                    procDescriptors[procIndex].procId,
                    procDescriptors[procIndex].procName,
                    procDescriptors[procIndex].procPriority,
                    procDescriptors[procIndex].procCpuUsage / 100,
                    procDescriptors[procIndex].procCpuUsage % 100,
                    gos_procGetProcStateString(procDescriptors[procIndex].procState)
                    );
        }
    }
    (void_t) gos_shellDriverTransmitString(DUMP_SEPARATOR"\n");
}

/**
 * @brief   Checks the process descriptor.
 * @details Returns with error if the process function is NULL, the priority is invalid,
 *          or the process function is the idle process.
 *
 * @param   procDescriptor : Pointer to the process descriptor structure.
 *
 * @return  Check result.
 *
 * @retval  GOS_SUCCESS    : Process descriptor valid.
 * @retval  GOS_ERROR      : Process descriptor invalid.
 */
GOS_STATIC gos_result_t gos_procCheckProcDescriptor    (gos_procDescriptor_t* procDescriptor)
{
    /*
     * Local variables.
     */
    gos_result_t procDescCheckResult = GOS_SUCCESS;

    /*
     * Function code.
     */
    if (procDescriptor->procFunction == NULL                    ||
        procDescriptor->procPriority > CFG_PROC_MAX_PRIO_LEVELS ||
        procDescriptor->procFunction == gos_idleProc
        )
    {
        procDescCheckResult = GOS_ERROR;
    }
    else
    {
        // Nothing to do.
    }

    return procDescCheckResult;
}

/**
 * @brief   Idle process.
 * @details Increases the idle process run counter and calls the idle process hook function
 *          (if registered).
 *
 * @return  -
 */
GOS_STATIC void_t gos_idleProc (void_t)
{
    /*
     * Function code.
     */
    procDescriptors[0].procRunCounter++;
    if (procIdleHookFunction != NULL)
    {
        procIdleHookFunction();
    }
    else
    {
        // Nothing to do.
    }
}

/**
 * @brief   System process.
 * @details Updates the process usage statistics.
 *
 * @return  -
 */
GOS_STATIC void_t gos_systemProc (void_t)
{
    /*
     * Local variables.
     */
    gos_procIndex_t procIndex = 0u;
    u32_t systemConvertedTime = 0u;
    u32_t procConvertedTime   = 0u;

    /*
     * Function code.
     */
    // Calculate in microseconds.
    systemConvertedTime = monitoringTime.minutes * 60 * 1000 * 1000 +
                          monitoringTime.seconds * 1000 * 1000 +
                          monitoringTime.milliseconds * 1000 +
                          monitoringTime.microseconds;

    for (procIndex = 0u; procIndex < CFG_PROC_MAX_NUMBER; procIndex++)
    {
        procConvertedTime = procDescriptors[procIndex].procRunTime.minutes * 60 * 1000 * 1000 +
                            procDescriptors[procIndex].procRunTime.seconds * 1000 * 1000 +
                            procDescriptors[procIndex].procRunTime.milliseconds * 1000 +
                            procDescriptors[procIndex].procRunTime.microseconds;

        if (systemConvertedTime > 0)
        {
            // Calculate CPU usage and then reset runtime counter.
            procDescriptors[procIndex].procCpuUsage = (u16_t)(((u32_t)10000 * procConvertedTime) / systemConvertedTime);
            procDescriptors[procIndex].procRunTime.days         = 0u;
            procDescriptors[procIndex].procRunTime.hours        = 0u;
            procDescriptors[procIndex].procRunTime.minutes      = 0u;
            procDescriptors[procIndex].procRunTime.seconds      = 0u;
            procDescriptors[procIndex].procRunTime.milliseconds = 0u;
            procDescriptors[procIndex].procRunTime.microseconds = 0u;
        }
        else
        {
            // Nothing to do.
        }

        if (procDescriptors[procIndex].procFunction == NULL)
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    // Reset monitoring time.
    monitoringTime.days         = 0u;
    monitoringTime.hours        = 0u;
    monitoringTime.minutes      = 0u;
    monitoringTime.seconds      = 0u;
    monitoringTime.milliseconds = 0u;
    monitoringTime.microseconds = 0u;

    systemProcLastRunTick = gos_kernelGetSysTicks();
    gos_procSleep(PROC_SYS_POLL_TIME_MS);
}

/**
 * @brief   Process daemon task.
 * @details Polls the process states and selects the next process to run based on
 *          process states and priorities. Calls the next process function and calculates
 *          the related run statistics.
 * @return  -
 */
GOS_STATIC void_t gos_procDaemonTask (void_t)
{
    /*
     * Local variables.
     */
    gos_procIndex_t procIndex       = 0u;
    u16_t           currentProc     = currentProcIndex;
    gos_taskPrio_t  lowestPrio      = CFG_PROC_IDLE_PRIO;
    u16_t           nextProc        = 0u;
    u16_t           sysTimerInitial = 0u;
    u16_t           sysTimerCurrVal = 0u;
    u16_t           currentRunTime  = 0u;

    /*
     * Function code.
     */
    for (;;)
    {
        currentProc = currentProcIndex;
        lowestPrio  = CFG_PROC_IDLE_PRIO;
        nextProc    = 0u;

        for (procIndex = 0U; procIndex < CFG_PROC_MAX_NUMBER; procIndex++)
        {
            // Wake-up sleeping processes if their sleep time has elapsed.
            if (procDescriptors[procIndex].procState == GOS_PROC_SLEEPING &&
                gos_kernelGetSysTicks() >= procDescriptors[procIndex].procSleepTicks)
            {
                procDescriptors[procIndex].procState = GOS_PROC_READY;
            }
            else
            {
                // Nothing to do.
            }

            // Choose the highest priority process - that is not the current one, and is ready - to run.
            if (procIndex != currentProc &&
                procDescriptors[procIndex].procFunction != NULL &&
                procDescriptors[procIndex].procState == GOS_PROC_READY &&
                procDescriptors[procIndex].procPriority < lowestPrio)
            {
                nextProc = procIndex;
                lowestPrio = procDescriptors[procIndex].procPriority;
            }
            else
            {
                // Nothing to do.
            }

            if (procDescriptors[procIndex].procFunction == NULL)
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        // If there was a process-swap, call the hook function.
        if (currentProcIndex != nextProc && procSwapHookFunction != NULL)
        {
            procSwapHookFunction(procDescriptors[currentProc].procId, procDescriptors[nextProc].procId);
        }
        else
        {
            // Nothing to do.
        }

        // Check if system process has not been scheduled for 4 poll periods.
        // If so, force schedule it to update process statistics.
        if ((gos_kernelGetSysTicks() - systemProcLastRunTick) > (4 * PROC_SYS_POLL_TIME_MS))
        {
            nextProc = 1u;
        }
        else
        {
            // Nothing to do.
        }

        currentProcIndex = nextProc;

        // Prepare for run-time measuring.
        (void_t) gos_timerDriverSysTimerGet(&sysTimerInitial);

        // Set privileges.
        (void_t) gos_taskSetPrivileges(
                procDaemonTaskId,
                procDescriptors[currentProcIndex].procPrivileges
                );

        // Call selected process.
        procDescriptors[currentProcIndex].procFunction();

        // Restore privileges.
        (void_t) gos_taskSetPrivileges(
                procDaemonTaskId,
                GOS_TASK_PRIVILEGE_KERNEL
                );

        procDescriptors[currentProcIndex].procRunCounter++;
        (void_t) gos_timerDriverSysTimerGet(&sysTimerCurrVal);
        currentRunTime = sysTimerInitial - sysTimerCurrVal;
        // Increase monitoring system time and current task runtime.
        (void_t) gos_runTimeAddMicroseconds(
                &monitoringTime,
                &procDescriptors[currentProcIndex].procRunTime,
                currentRunTime);

        (void_t) gos_kernelTaskSleep(PROC_DAEMON_POLL_TIME_MS);
    }
}

/**
 * @brief   Translates the process state variable to a string.
 * @details Returns the corresponding string for the process state enumerator.
 *
 * @param   procState : The process state to be translated.
 *
 * @return  String based on the process state.
 */
GOS_STATIC char_t* gos_procGetProcStateString (gos_procState_t procState)
{
    /*
     * Function code.
     */
    switch (procState)
    {
        case GOS_PROC_READY:
        {
            return TRACE_FG_GREEN_START"ready    ";
        }
        case GOS_PROC_SLEEPING:
        {
            return TRACE_FG_YELLOW_START"sleeping " ;
        }
        case GOS_PROC_SUSPENDED:
        {
            return TRACE_FG_MAGENTA_START"suspended";
        }
        default:
        {
            return "";
        }
    }
}
#endif
