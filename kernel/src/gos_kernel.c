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
//! @file       gos_kernel.c
//! @author     Ahmed Gazar
//! @date       2024-04-24
//! @version    1.20
//!
//! @brief      GOS kernel source.
//! @details    For a more detailed description of this module, please refer to @ref gos_kernel.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-21    Ahmed Gazar     Initial version created.
// 1.1        2022-11-15    Ahmed Gazar     +    License added
// 1.2        2022-12-03    Ahmed Gazar     +    gos_kernelTaskRegisterTasks added
//                                          +    Tasks external task ID handling added
//                                          +    Kernel configuration structure array added
//                                          +    Cooperative scheduling introduced
// 1.3        2022-12-04    Ahmed Gazar     *    Sleep, wake-up, suspend, resume, block, unblock
//                                               hook functions removed
//                                          +    Kernel dump ready signal added
// 1.4        2022-12-08    Ahmed Gazar     +    Task priority setter and getter functions added
// 1.5        2022-12-15    Ahmed Gazar     +    Privilege handling introduced, privilege check
//                                               added to kernel functions
// 1.6        2023-01-13    Ahmed Gazar     *    Scheduling disabled flag replaced with counter
//                                          -    Configuration dump removed
// 1.7        2023-05-03    Ahmed Gazar     +    Stack overflow monitoring added
//                                          *    Dump separated to task and stack statistics
// 1.8        2023-05-19    Ahmed Gazar     +    gos_kernelGetCpuUsage added
// 1.9        2023-06-17    Ahmed Gazar     *    Kernel dump moved from task to function
//                                          *    CPU statistics fix
// 1.10       2023-06-30    Ahmed Gazar     +    cpuUseLimit added, CPU usage limit implemented
//                                          *    CPU statistics calculation moved to idle task
//                                          *    gos_kernelCalculateTaskCpuUsages reset required
//                                               flag added
//                                          +    gos_kernelSetMaxCpuLoad added
//                                          +    gos_kernelGetMaxCpuLoad added
//                                          +    gos_kernelIsCallerIsr added
//                                          +    gos_kernelIsCallerPrivileged added
//                                          +    gos_kernelTaskSetPrivileges  added
// 1.11       2023-07-11    Ahmed Gazar     -    privilegedAccess removed
//                                          +    kernelSysTickHookFunction added
//                                          -    Privileged access concept removed
//                                          +    Task block timeout introduced
// 1.12       2023-07-25    Ahmed Gazar     +    gos_kernelTaskGetDataByIndex added
//                                          *    Ported function calls added
// 1.13       2023-09-08    Ahmed Gazar     *    Sleep and block tick handling modified
// 1.14       2023-09-25    Ahmed Gazar     +    Missing else branches added
// 1.15       2023-10-20    Ahmed Gazar     +    isKernelRunning flag added
//                                          +    kernelPrivilegedHookFunction added
//                                          +    privilegedModeSetRequired flag added
//                                          +    gos_kernelPrivilegedModeSetRequired function added
//                                          +    Millisecond system time increment added to systick
//                                               handler
// 1.16       2023-11-01    Ahmed Gazar     *    Task-related definitions, variables, and functions
//                                               removed
//                                          +    inIsr and atomic counters, primask added
// 1.17       2023-11-10    Ahmed Gazar     +    Missing else branches and void casts added
// 1.18       2024-04-02    Ahmed Gazar     *    gos_kernelCalculateTaskCpuUsages: break from loop
//                                               moved to the beginning of loop
// 1.19       2024-04-17    Ahmed Gazar     *    Task block timeout check fixed
// 1.20       2024-04-24    Ahmed Gazar     -    Process service include removed
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
#include <gos_error.h>
#include <gos_kernel.h>
#include <gos_port.h>
#include <gos_signal.h>
#include <gos_shell_driver.h>
#include <gos_timer_driver.h>
#include <gos_trace.h>
#include <stdio.h>
#include <string.h>

/*
 * Macros
 */
/**
 * System Handler control and state register.
 */
#define SHCSR                   *( (volatile u32_t*) 0xE000ED24u )

/**
 * Interrupt Control and State Register.
 */
#define ICSR                    *( (volatile u32_t*) 0xE000ED04u )

/**
 * Task dump separator line.
 */
#define TASK_DUMP_SEPARATOR     "+--------+------------------------------+------+------------------+---------+-----------+\r\n"

/**
 * Max CPU loads dump separator line.
 */
#define MAX_CPU_DUMP_SEPARATOR  "+--------+------------------------------+-------------+\r\n"

/**
 * Stack statistics separator line.
 */
#define STACK_STATS_SEPARATOR   "+--------+------------------------------+--------+----------------+-----------+\r\n"

/**
 * Config dump separator line.
 */
#define CONFIG_DUMP_SEPARATOR   "+----------------------------------+--------+\r\n"

/**
 * Pattern for binary format printing.
 */
#define BINARY_PATTERN          "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"TRACE_FORMAT_RESET

/**
 * u16_t to binary converter macro.
 */
#define TO_BINARY(word)  \
    (word & 0x8000 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x4000 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x2000 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x1000 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0800 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0400 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0200 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0100 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0080 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0040 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0020 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0010 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0008 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0004 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0002 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0"), \
    (word & 0x0001 ? TRACE_FG_GREEN_START"1" : TRACE_FG_RED_START"0")

/*
 * Global variables
 */
/**
 * Kernel dump signal.
 */
gos_signalId_t                      kernelDumpSignal;

/**
 * Kernel dump ready signal.
 */
gos_signalId_t                      kernelDumpReadySignal;

/**
 * Scheduling disabled counter.
 */
u8_t                                schedDisableCntr             = 0u;

/**
 * In ISR counter.
 */
u8_t                                inIsr                        = 0u;

/**
 * Atomic counter (incremented when GOS_ATOMIC_ENTER is called).
 */
u8_t                                atomicCntr                   = 0u;

/**
 * IRQ state for restoring after GOS_ATOMIC_EXIT.
 */
u32_t                               primask                      = 0u;

/**
 * Current task index - for priority scheduling.
 */
u32_t                               currentTaskIndex             = 0u;

/**
 * CPU use limit.
 */
u16_t                               cpuUseLimit                  = 10000;

/*
 * Static variables
 */
/**
 * System tick value.
 */
GOS_STATIC u32_t                    sysTicks                     = 0u;

/**
 * System timer value for run-time calculations.
 */
GOS_STATIC u16_t                    sysTimerValue                = 0u;

/**
 * Monitoring system time since last statistics calculation.
 */
GOS_STATIC gos_runtime_t            monitoringTime               = {0};

/**
 * Kernel swap hook function.
 */
GOS_STATIC gos_taskSwapHook_t       kernelSwapHookFunction       = NULL;

/**
 * Kernel system tick hook function.
 */
GOS_STATIC gos_sysTickHook_t        kernelSysTickHookFunction    = NULL;

/**
 * Kernel privileged execution mode set hook function.
 */
GOS_STATIC gos_privilegedHook_t     kernelPrivilegedHookFunction = NULL;

/**
 * Reset required flag.
 */
GOS_STATIC bool_t                   resetRequired                = GOS_FALSE;

/**
 * Flag to indicate whether privileged mode set is required.
 */
GOS_STATIC bool_t                   privilegedModeSetRequired    = GOS_FALSE;

/**
 * Previous tick value for sleep and block tick calculation.
 */
GOS_STATIC u32_t                    previousTick                 = 0u;

/**
 * Flag to indicate whether kernel is running.
 */
GOS_STATIC bool_t                   isKernelRunning              = GOS_FALSE;

/*
 * External variables
 */
GOS_EXTERN gos_taskDescriptor_t     taskDescriptors [CFG_TASK_MAX_NUMBER];

/*
 * Function prototypes
 */
GOS_STATIC void_t  gos_kernelCheckTaskStack     (void_t);
GOS_STATIC u32_t   gos_kernelGetCurrentPsp      (void_t);
GOS_STATIC void_t  gos_kernelSaveCurrentPsp     (u32_t psp);
GOS_STATIC void_t  gos_kernelSelectNextTask     (void_t);
GOS_STATIC char_t* gos_kernelGetTaskStateString (gos_taskState_t taskState);
GOS_STATIC void_t  gos_kernelProcessorReset     (void_t);

/*
 * External functions
 */
GOS_EXTERN void_t  gos_idleTask                 (void_t);

/*
 * Function: gos_kernelInit
 */
gos_result_t gos_kernelInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t  initResult = GOS_ERROR;
    u16_t         taskIndex  = 1u;

    /*
     * Function code.
     */
    // Initialize task descriptors.
    for (taskIndex = 1u; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
    {
        taskDescriptors[taskIndex].taskFunction         = NULL;
        taskDescriptors[taskIndex].taskPriority         = GOS_TASK_MAX_PRIO_LEVELS;
        taskDescriptors[taskIndex].taskOriginalPriority = GOS_TASK_MAX_PRIO_LEVELS;
        taskDescriptors[taskIndex].taskState            = GOS_TASK_SUSPENDED;
        taskDescriptors[taskIndex].taskId               = GOS_INVALID_TASK_ID;
    }

    // Register idle task PSP.
    u32_t* psp = (u32_t*)(MAIN_STACK - GLOBAL_STACK);

    // Fill dummy stack frame.
    *(--psp) = 0x01000000u; // Dummy xPSR, just enable Thumb State bit;
    *(--psp) = (u32_t) gos_idleTask; // PC
    *(--psp) = 0xFFFFFFFDu; // LR with EXC_RETURN to return to Thread using PSP
    *(--psp) = 0x12121212u; // Dummy R12
    *(--psp) = 0x03030303u; // Dummy R3
    *(--psp) = 0x02020202u; // Dummy R2
    *(--psp) = 0x01010101u; // Dummy R1
    *(--psp) = 0x00000000u; // Dummy R0
    *(--psp) = 0x11111111u; // Dummy R11
    *(--psp) = 0x10101010u; // Dummy R10
    *(--psp) = 0x09090909u; // Dummy R9
    *(--psp) = 0x08080808u; // Dummy R8
    *(--psp) = 0x07070707u; // Dummy R7
    *(--psp) = 0x06060606u; // Dummy R6
    *(--psp) = 0x05050505u; // Dummy R5
    *(--psp) = 0x04040404u; // Dummy R4

    // Save PSP.
    taskDescriptors[0].taskPsp = (u32_t)psp;

    // Calculate stack overflow threshold.
    taskDescriptors[0].taskStackOverflowThreshold = taskDescriptors[0].taskPsp - taskDescriptors[0].taskStackSize + 64;

    // Enable Fault Handlers
    gos_ported_enableFaultHandlers();

    schedDisableCntr = 0u;

    initResult = GOS_SUCCESS;

    return initResult;
}

/*
 * Function: gos_kernelStart
 */
gos_result_t gos_kernelStart (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t kernelStartResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Start with the first task.
    currentTaskIndex = 0;

    // Do low-level initialization.
    gos_ported_kernelStartInit();

    // Get the handler of the first task by tracing back from PSP which is at R4 slot.
    gos_task_t firstTask = taskDescriptors[currentTaskIndex].taskFunction;

    // Initialize system timer value.
    (void_t) gos_timerDriverSysTimerGet(&sysTimerValue);

    // Enable scheduling.
    GOS_ENABLE_SCHED

    // Set kernel running flag.
    isKernelRunning = GOS_TRUE;

    // Execute the handler.
    firstTask();

    // This point should not be reached.
    return kernelStartResult;
}

/*
 * Function: gos_kernelRegisterSwapHook
 */
gos_result_t gos_kernelRegisterSwapHook (gos_taskSwapHook_t swapHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (swapHookFunction != NULL && kernelSwapHookFunction == NULL)
    {
        kernelSwapHookFunction = swapHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_kernelRegisterSysTickHook
 */
gos_result_t gos_kernelRegisterSysTickHook (gos_sysTickHook_t sysTickHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (sysTickHookFunction != NULL && kernelSysTickHookFunction == NULL)
    {
        kernelSysTickHookFunction = sysTickHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_kernelRegisterPrivilegedHook
 */
gos_result_t gos_kernelRegisterPrivilegedHook (gos_privilegedHook_t privilegedHookFunction)
{
    /*
     * Local variables.
     */
    gos_result_t hookRegisterResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (privilegedHookFunction != NULL && kernelPrivilegedHookFunction == NULL)
    {
        kernelPrivilegedHookFunction = privilegedHookFunction;
        hookRegisterResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return hookRegisterResult;
}

/*
 * Function: gos_kernelSubscribeDumpReadySignal
 */
gos_result_t gos_kernelSubscribeDumpReadySignal (gos_signalHandler_t dumpReadySignalHandler)
{
    /*
     * Local variables.
     */
    gos_result_t subscriptionResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (dumpReadySignalHandler != NULL)
    {
        subscriptionResult = gos_signalSubscribe(kernelDumpReadySignal, dumpReadySignalHandler, GOS_TASK_PRIVILEGE_USER);
    }
    else
    {
        // Signal handler is NULL pointer.
    }

    return subscriptionResult;
}

/*
 * Function: gos_ported_sysTickInterrupt
 */
#include <gos_time.h>
void_t gos_ported_sysTickInterrupt (void_t)
{
    /*
     * Function code.
     */
    sysTicks++;
    (void_t) gos_timeIncreaseSystemTime(1);

    // Periodic stack overflow check on currently running task.
    if (isKernelRunning == GOS_TRUE)
    {
        gos_kernelCheckTaskStack();

#if CFG_SCHED_COOPERATIVE == 0
        if (schedDisableCntr == 0u)
        {
            // Privileged.
            gos_kernelReschedule(GOS_PRIVILEGED);
        }
        else
        {
            // Scheduling disabled.
        }
#endif
    }
    else
    {
        // Kernel is not running.
    }

    if (kernelSysTickHookFunction != NULL)
    {
        kernelSysTickHookFunction();
    }
    else
    {
        // Nothing to do.
    }
}

/*
 * Function: gos_kernelGetSysTicks
 */
u32_t gos_kernelGetSysTicks (void_t)
{
    /*
     * Function code.
     */
    return sysTicks;
}

/*
 * Function: gos_kernelGetCpuUsage
 */
u16_t gos_kernelGetCpuUsage (void_t)
{
    /*
     * Function code.
     */
    return (10000 - taskDescriptors[0].taskCpuMonitoringUsage);
}

/*
 * Function: gos_kernelReset
 */
void_t gos_kernelReset (void_t)
{
    /*
     * Function code.
     */
    resetRequired = GOS_TRUE;
    gos_kernelReschedule(GOS_UNPRIVILEGED);
}

/*
 * Function: gos_kernelPrivilegedModeSetRequired
 */
void_t gos_kernelPrivilegedModeSetRequired (void_t)
{
    /*
     * Function code.
     */
    privilegedModeSetRequired = GOS_TRUE;
    gos_kernelReschedule(GOS_UNPRIVILEGED);
    kernelPrivilegedHookFunction();
}

/*
 * Function: gos_kernelDelayUs
 */
GOS_INLINE void_t gos_kernelDelayUs (u16_t microseconds)
{
    /*
     * Local variables.
     */
    u16_t tmrInitialValue = 0u;
    u16_t tmrActualValue  = 0u;

    /*
     * Function code.
     */
    (void_t) gos_timerDriverSysTimerGet(&tmrInitialValue);
    do
    {
        (void_t) gos_timerDriverSysTimerGet(&tmrActualValue);
    }
    while ((u16_t)(tmrActualValue - tmrInitialValue) < microseconds);
}

/*
 * Function: gos_kernelDelayMs
 */
GOS_INLINE void_t gos_kernelDelayMs (u16_t milliseconds)
{
    /*
     * Local variables.
     */
    u32_t sysTickVal = sysTicks;

    /*
     * Function code.
     */
    while ((u16_t)(sysTicks - sysTickVal) < milliseconds);
}

/*
 * Function: gos_kernelCalculateTaskCpuUsages
 */
GOS_INLINE void_t gos_kernelCalculateTaskCpuUsages (bool_t isResetRequired)
{
    /*
     * Local variables.
     */
    u16_t taskIndex           = 0u;
    u32_t systemConvertedTime = 0u;
    u32_t taskConvertedTime   = 0u;

    /*
     * Function code.
     */
    // Calculate in microseconds.
    systemConvertedTime = monitoringTime.minutes * 60 * 1000 * 1000 +
                          monitoringTime.seconds * 1000 * 1000 +
                          monitoringTime.milliseconds * 1000 +
                          monitoringTime.microseconds;

    for (taskIndex = 0u; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
    {
        if (taskDescriptors[taskIndex].taskFunction == NULL)
        {
            break;
        }
        else
        {
            // Continue.
        }

        taskConvertedTime   = taskDescriptors[taskIndex].taskMonitoringRunTime.minutes * 60 * 1000 * 1000 +
                              taskDescriptors[taskIndex].taskMonitoringRunTime.seconds * 1000 * 1000 +
                              taskDescriptors[taskIndex].taskMonitoringRunTime.milliseconds * 1000 +
                              taskDescriptors[taskIndex].taskMonitoringRunTime.microseconds;

        if (systemConvertedTime > 0)
        {
            // Calculate CPU usage and then reset runtime counter.
            taskDescriptors[taskIndex].taskCpuMonitoringUsage = (u16_t)(((u32_t)10000 * taskConvertedTime) / systemConvertedTime);

            if (isResetRequired == GOS_TRUE || monitoringTime.seconds > 0)
            {
                taskDescriptors[taskIndex].taskCpuUsage = (u16_t)((u32_t)(10000 * taskConvertedTime) / systemConvertedTime);

                // Increase runtime microseconds.
                (void_t) gos_runTimeAddMicroseconds(
                        &taskDescriptors[taskIndex].taskRunTime,
                        NULL,
                        (u16_t)taskDescriptors[taskIndex].taskMonitoringRunTime.microseconds);

                // Increase runtime milliseconds.
                (void_t) gos_runTimeAddMilliseconds(
                        &taskDescriptors[taskIndex].taskRunTime,
                        (u32_t)(taskDescriptors[taskIndex].taskMonitoringRunTime.milliseconds +
                        taskDescriptors[taskIndex].taskMonitoringRunTime.seconds * 1000));

                taskDescriptors[taskIndex].taskMonitoringRunTime.days         = 0u;
                taskDescriptors[taskIndex].taskMonitoringRunTime.hours        = 0u;
                taskDescriptors[taskIndex].taskMonitoringRunTime.minutes      = 0u;
                taskDescriptors[taskIndex].taskMonitoringRunTime.seconds      = 0u;
                taskDescriptors[taskIndex].taskMonitoringRunTime.milliseconds = 0u;
                taskDescriptors[taskIndex].taskMonitoringRunTime.microseconds = 0u;

                // Store the highest CPU usage value.
                if (taskDescriptors[taskIndex].taskCpuUsage > taskDescriptors[taskIndex].taskCpuUsageMax)
                {
                    taskDescriptors[taskIndex].taskCpuUsageMax = taskDescriptors[taskIndex].taskCpuUsage;
                }
                else
                {
                    // Max. value has not been reached.
                }
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
    }

    // Reset monitoring time.
    if (isResetRequired == GOS_TRUE || monitoringTime.seconds > 0)
    {
        monitoringTime.days         = 0u;
        monitoringTime.hours        = 0u;
        monitoringTime.minutes      = 0u;
        monitoringTime.seconds      = 0u;
        monitoringTime.milliseconds = 0u;
        monitoringTime.microseconds = 0u;
    }
    else
    {
        // Nothing to do.
    }
}

/*
 * Function: gos_kernelDump
 */
void_t gos_kernelDump (void_t)
{
    /*
     * Local variables.
     */
    u16_t taskIndex = 0u;

    /*
     * Function code.
     */
    (void_t) gos_shellDriverTransmitString("Task dump:\r\n");
    (void_t) gos_shellDriverTransmitString(TASK_DUMP_SEPARATOR);
    (void_t) gos_shellDriverTransmitString(
        "| %6s | %28s | %4s | %16s | %6s | %9s |\r\n",
        "tid",
        "name",
        "prio",
        "privileges",
        "cpu [%]",
        "state"
        );
    (void_t) gos_shellDriverTransmitString(TASK_DUMP_SEPARATOR);

    for (taskIndex = 0u; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
    {
        if (taskDescriptors[taskIndex].taskFunction == NULL)
        {
            break;
        }
        else
        {
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28s | %4d | " BINARY_PATTERN " | %4u.%02u | %18s |\r\n",
                    taskDescriptors[taskIndex].taskId,
                    taskDescriptors[taskIndex].taskName,
                    taskDescriptors[taskIndex].taskPriority,
                    TO_BINARY((u16_t)taskDescriptors[taskIndex].taskPrivilegeLevel),
                    taskDescriptors[taskIndex].taskCpuUsage / 100,
                    taskDescriptors[taskIndex].taskCpuUsage % 100,
                    gos_kernelGetTaskStateString(taskDescriptors[taskIndex].taskState)
                    );
        }
    }
    (void_t) gos_shellDriverTransmitString(TASK_DUMP_SEPARATOR"\n");

    // Max CPU load statistics.
    (void_t) gos_shellDriverTransmitString("CPU max loads:\r\n");
    (void_t) gos_shellDriverTransmitString(MAX_CPU_DUMP_SEPARATOR);
    (void_t) gos_shellDriverTransmitString(
        "| %6s | %28s | %11s |\r\n",
        "tid",
        "name",
        "max cpu [%]"
        );
    (void_t) gos_shellDriverTransmitString(MAX_CPU_DUMP_SEPARATOR);

    for (taskIndex = 0u; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
    {
        if (taskDescriptors[taskIndex].taskFunction == NULL)
        {
            break;
        }
        else
        {
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28s | %8u.%02u |\r\n",
                    taskDescriptors[taskIndex].taskId,
                    taskDescriptors[taskIndex].taskName,
                    taskDescriptors[taskIndex].taskCpuUsageMax / 100,
                    taskDescriptors[taskIndex].taskCpuUsageMax % 100
                    );
        }
    }
    (void_t) gos_shellDriverTransmitString(MAX_CPU_DUMP_SEPARATOR"\n");

    // Stack stats.
    (void_t) gos_shellDriverTransmitString("Stack statistics:\r\n");
    (void_t) gos_shellDriverTransmitString(STACK_STATS_SEPARATOR);
    (void_t) gos_shellDriverTransmitString(
        "| %6s | %28s | %6s | %14s | %9s |\r\n",
        "tid",
        "name",
        "stack",
        "max stack use",
        "stack [%]"
        );
    (void_t) gos_shellDriverTransmitString(STACK_STATS_SEPARATOR);
    for (taskIndex = 0u; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
    {
        if (taskDescriptors[taskIndex].taskFunction == NULL)
        {
            break;
        }
        else
        {
            (void_t) gos_shellDriverTransmitString(
                    "| 0x%04X | %28s | 0x%04X | 0x%-12X | %6u.%02u |%\r\n",
                    taskDescriptors[taskIndex].taskId,
                    taskDescriptors[taskIndex].taskName,
                    taskDescriptors[taskIndex].taskStackSize,
                    taskDescriptors[taskIndex].taskStackSizeMaxUsage,
                    ((10000 * taskDescriptors[taskIndex].taskStackSizeMaxUsage) / taskDescriptors[taskIndex].taskStackSize) / 100,
                    ((10000 * taskDescriptors[taskIndex].taskStackSizeMaxUsage) / taskDescriptors[taskIndex].taskStackSize) % 100
                    );
        }
    }
    (void_t) gos_shellDriverTransmitString(STACK_STATS_SEPARATOR"\n");
}

/*
 * Function: gos_kernelSetMaxCpuLoad
 */
gos_result_t gos_kernelSetMaxCpuLoad (u16_t maxCpuLoad)
{
    /*
     * Local variables.
     */
    gos_result_t setMaxCpuLoadResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (maxCpuLoad > 0 && maxCpuLoad <= 10000)
    {
        cpuUseLimit = maxCpuLoad;
        setMaxCpuLoadResult = GOS_SUCCESS;
    }
    else
    {
        // Max. CPU load is invalid.
    }

    return setMaxCpuLoadResult;
}

/*
 * Function: gos_kernelGetMaxCpuLoad
 */
gos_result_t gos_kernelGetMaxCpuLoad (u16_t* maxCpuLoad)
{
    /*
     * Local variables.
     */
    gos_result_t getMaxCpuLoadResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (maxCpuLoad != NULL)
    {
        *maxCpuLoad = cpuUseLimit;
        getMaxCpuLoadResult = GOS_SUCCESS;
    }
    else
    {
        // Target is NULL pointer.
    }

    return getMaxCpuLoadResult;
}

/*
 * Function: gos_kernelIsCallerPrivileged
 */
bool_t gos_kernelIsCallerIsr (void_t)
{
    /*
     * Function code.
     */
    return inIsr > 0u ? GOS_TRUE : GOS_FALSE;
}

/*
 * Function: gos_ported_svcHandler
 */
void_t gos_ported_svcHandler (void_t)
{
    /*
     * Function code.
     */
    gos_ported_handleSVC();
}

/*
 * Function: gos_ported_svcHandlerMain
 */
void_t gos_ported_svcHandlerMain (u32_t* sp)
{
    /*
     * Function code.
     */
    gos_ported_handleSVCMain(sp);
}

/*
 * Function: gos_ported_pendSVHandler
 */
void_t gos_ported_pendSVHandler (void_t)
{
    /*
     * Function code.
     */
    if (privilegedModeSetRequired == GOS_TRUE)
    {
        // Set mode to privileged.
        GOS_ASM("MRS R0, CONTROL");
        // Set bit[0] nPRIV to 0.
        GOS_ASM("BIC R0, R0, #1");
        GOS_ASM("MSR CONTROL, R0");

        // Reset flag.
        privilegedModeSetRequired = GOS_FALSE;
    }
    else
    {
        // Otherwise perform context switch.
        gos_ported_doContextSwitch();
    }
}

/*
 * Function: gos_kernelReschedule
 */
GOS_INLINE void_t gos_kernelReschedule (gos_kernel_privilege_t privilege)
{
    /*
     * Function code.
     */
    gos_ported_reschedule(privilege);
}

/**
 * @brief   Checks the stack use of the current task.
 * @details Gets the current stack pointer value and checks whether it is
 *          lower than the allowed threshold value defined for the current stack.
 *          After the overflow check, it calculates the stack usage and updates the
 *          maximum stack usage for the current task. In case of stack overflow, it
 *          goes to system error.
 *
 * @return  -
 */
GOS_STATIC void_t gos_kernelCheckTaskStack (void_t)
{
    /*
     * Local variables.
     */
    u32_t sp = 0u;

    /*
     * Function code.
     */
    __asm volatile ("MRS %0, psp\n\t" : "=r" (sp));
    if (sp != 0 &&
        sp < taskDescriptors[currentTaskIndex].taskStackOverflowThreshold)
    {
        gos_errorHandler(
                GOS_ERROR_LEVEL_OS_FATAL,
                NULL,
                0,
                "Stack overflow detected in <%s>. \r\nPSP: 0x%x overflown by %d bytes.",
                taskDescriptors[currentTaskIndex].taskName,
                sp,
                (taskDescriptors[currentTaskIndex].taskStackOverflowThreshold - sp));
    }
    else
    {
        // No stack overflow was detected.
    }

    if (sp != 0 &&
        (taskDescriptors[currentTaskIndex].taskStackOverflowThreshold - 64 + taskDescriptors[currentTaskIndex].taskStackSize - sp) >
        taskDescriptors[currentTaskIndex].taskStackSizeMaxUsage)
    {
        taskDescriptors[currentTaskIndex].taskStackSizeMaxUsage = (taskDescriptors[currentTaskIndex].taskStackOverflowThreshold - 64 + 32 + taskDescriptors[currentTaskIndex].taskStackSize - sp);
    }
    else
    {
        // Max. value has not been exceeded.
    }
}

/**
 * @brief    Returns the current PSP.
 * @details  Returns the current PSP.
 *
 * @return   Current PSP value.
 */
GOS_UNUSED GOS_STATIC u32_t gos_kernelGetCurrentPsp (void_t)
{
    /*
     * Function code.
     */
    return taskDescriptors[currentTaskIndex].taskPsp;
}

/**
 * @brief   Saves the current PSP.
 * @details Saves the current PSP.
 *
 * @param   psp : Current PSP value.
 *
 * @return    -
 */
GOS_UNUSED GOS_STATIC void_t gos_kernelSaveCurrentPsp (u32_t psp)
{
    /*
     * Function code.
     */
    taskDescriptors[currentTaskIndex].taskPsp = psp;
}

/**
 * @brief   Selects the next task for execution.
 * @details Loops through the internal task descriptor array and first checks
 *          the sleeping tasks and wakes up the ones that passed their sleeping time.
 *          Then based on the priority of the ready tasks, it selects the one with
 *          the highest priority (lowest number in priority). If there is a swap-hook
 *          function registered, it calls it, and then it refreshes the task run-time
 *          statistics.
 *
 * @return    -
 */
GOS_UNUSED GOS_STATIC void_t gos_kernelSelectNextTask (void_t)
{
    /*
     * Local variables.
     */
    u16_t          taskIndex      = 0u;
    gos_taskPrio_t lowestPrio     = GOS_TASK_IDLE_PRIO;
    u16_t          nextTask       = 0u;
    u16_t          sysTimerActVal = 0u;
    u16_t          currentRunTime = 0u;
    u32_t          elapsedTicks   = sysTicks - previousTick;

    /*
     * Function code.
     */
    // Run stack check.
    gos_kernelCheckTaskStack();

    if (schedDisableCntr == 0u)
    {
        for (taskIndex = 0U; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
        {
            // Wake-up sleeping tasks if their sleep time has elapsed.
            if (taskDescriptors[taskIndex].taskState == GOS_TASK_SLEEPING &&
                (taskDescriptors[taskIndex].taskSleepTickCounter += elapsedTicks) >= taskDescriptors[taskIndex].taskSleepTicks)
            {
                taskDescriptors[taskIndex].taskState = GOS_TASK_READY;
            }
            // Unblock tasks if their timeout time has elapsed.
            else if ((taskDescriptors[taskIndex].taskState == GOS_TASK_BLOCKED) &&
                    (taskDescriptors[taskIndex].taskBlockTicks != GOS_TASK_MAX_BLOCK_TIME_MS) &&
                    ((taskDescriptors[taskIndex].taskBlockTickCounter += elapsedTicks) >= taskDescriptors[taskIndex].taskBlockTicks))
            {
                taskDescriptors[taskIndex].taskState = GOS_TASK_READY;
            }
            else
            {
                // Nothing to do.
            }

            // Choose the highest priority task - that is not the current one, and is ready - to run.
            // Also, check if task has reached the CPU usage limit.
            if (taskIndex != currentTaskIndex &&
                taskDescriptors[taskIndex].taskState == GOS_TASK_READY &&
                taskDescriptors[taskIndex].taskPriority < lowestPrio &&
                taskDescriptors[taskIndex].taskCpuMonitoringUsage < taskDescriptors[taskIndex].taskCpuUsageLimit &&
                taskDescriptors[taskIndex].taskCpuUsage < taskDescriptors[taskIndex].taskCpuUsageLimit)
            {
                nextTask = taskIndex;
                lowestPrio = taskDescriptors[taskIndex].taskPriority;
            }
            else
            {
                // Nothing to do.
            }

            if (taskDescriptors[taskIndex].taskFunction == NULL)
            {
                break;
            }
            else
            {
                // Continue.
            }
        }

        // If CPU limit exceeded, override scheduling, and select the idle task to run.
        if (gos_kernelGetCpuUsage() > cpuUseLimit)
        {
            nextTask = 0u;
        }
        else
        {
            // Nothing to do.
        }

        // If there was a task-swap, call the hook function.
        if (currentTaskIndex != nextTask)
        {
            if (kernelSwapHookFunction != NULL)
            {
                kernelSwapHookFunction(taskDescriptors[currentTaskIndex].taskId, taskDescriptors[nextTask].taskId);
            }
            else
            {
                // Nothing to do.
            }
            taskDescriptors[currentTaskIndex].taskCsCounter++;
        }
        else
        {
            // Nothing to do.
        }

        // Calculate current task run-time.
        gos_timerDriverSysTimerGet(&sysTimerActVal);
        currentRunTime = sysTimerActVal - sysTimerValue;

        // Increase monitoring system time and current task runtime.
        (void_t) gos_runTimeAddMicroseconds(&monitoringTime, &taskDescriptors[currentTaskIndex].taskMonitoringRunTime, currentRunTime);

        // Refresh system timer value.
        (void_t) gos_timerDriverSysTimerGet(&sysTimerValue);

        // Set current task.
        currentTaskIndex = nextTask;

        // Update previous tick value.
        previousTick = sysTicks;
    }
    else
    {
        // Nothing to do.
    }
}

/**
 * @brief   Translates the task state to a string.
 * @details Based on the task state it returns a string with a printable form
 *          of the task state.
 *
 * @param   taskState : The task state variable to be translated.
 *
 * @return  String with the task state.
 */
GOS_STATIC char_t* gos_kernelGetTaskStateString (gos_taskState_t taskState)
{
    /*
     * Function code.
     */
    switch (taskState)
    {
        case GOS_TASK_READY:
        {
            return TRACE_FG_GREEN_START"ready"TRACE_FORMAT_RESET;
        }break;
        case GOS_TASK_SLEEPING:
        {
            return TRACE_FG_YELLOW_START"sleeping"TRACE_FORMAT_RESET;
        }break;
        case GOS_TASK_SUSPENDED:
        {
            return TRACE_FG_MAGENTA_START"suspended"TRACE_FORMAT_RESET;
        }break;
        case GOS_TASK_BLOCKED:
        {
            return TRACE_FG_CYAN_START"blocked"TRACE_FORMAT_RESET;
        }break;
        case GOS_TASK_ZOMBIE:
        {
            return TRACE_FG_RED_START"zombie"TRACE_FORMAT_RESET;
        }
        default:
        {
            return "";
        }
    }
}

/**
 * @brief   Kernel processor reset.
 * @details Resets the processor.
 *
 * @return    -
 */
GOS_STATIC void_t gos_kernelProcessorReset (void_t)
{
    /*
     * Function code.
     */
    // Reset processor.
    gos_ported_procReset();

    // Wait for reset.
    for (;;)
    {
        GOS_NOP;
    }
}

/*
 * Fault handlers
 */
/*
 * Function: NMI_Handler
 */
void_t NMI_Handler (void_t)
{
    /*
     * Function code.
     */
    for (;;)
    {
        GOS_NOP;
    }
}

/*
 * Function: HardFault_Handler
 */
void_t HardFault_Handler (void_t)
{
    /*
     * Function code.
     */

    for (;;)
    {
        GOS_NOP;
    }
}

/*
 * Function: MemManage_Handler
 */
void_t MemManage_Handler (void_t)
{
    /*
     * Function code.
     */
    for (;;)
    {
        GOS_NOP;
    }
}

/*
 * Function: BusFault_Handler
 */
void_t BusFault_Handler (void_t)
{
    /*
     * Function code.
     */
    for (;;)
    {
        GOS_NOP;
    }
}

/*
 * Function: UsageFault_Handler
 */
void_t UsageFault_Handler (void_t)
{
    /*
     * Function code.
     */

    for (;;)
    {
        GOS_NOP;
    }
}
