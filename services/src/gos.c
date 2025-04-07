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
//! @file       gos.c
//! @author     Ahmed Gazar
//! @date       2025-04-06
//! @version    1.10
//!
//! @brief      GOS source.
//! @details    For a more detailed description of this service, please refer to @ref gos.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-31    Ahmed Gazar     Initial version created
// 1.1        2022-11-14    Ahmed Gazar     +    Initialization error flag setting modified
// 1.2        2022-11-15    Ahmed Gazar     +    License added
// 1.3        2022-12-11    Ahmed Gazar     +    main function added here
//                                          *    gos_Init and gos_Start made local static functions
//                                          +    platform and user initializers added
// 1.4        2023-01-12    Ahmed Gazar     *    OS service initialization and user application
//                                               initialization moved to a new initializer task
// 1.5        2023-01-31    Ahmed Gazar     *    Initializer task renamed to system task
//                                          +    Periodic CPU statistics calculation added to
//                                               system task
// 1.6        2023-05-04    Ahmed Gazar     -    Lock and trigger service initialization removed
// 1.7        2023-06-17    Ahmed Gazar     +    Dump added
//                                          +    GOS_SYS_TASK_SLEEP_TIME added
// 1.8        2023-06-28    Ahmed Gazar     +    Dump ready signal invoking added
// 1.9        2023-07-12    Ahmed Gazar     +    gos_sysmonInit added to initializers
// 1.10       2025-04-06    Ahmed Gazar     *    GOS_CONCAT_RESULT usage added
//                                          *    Sleep removed from system task dump handling
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
#include <gos.h>

/*
 * Macros
 */
/**
 * System task sleep time.
 */
#define GOS_SYS_TASK_SLEEP_TIME ( 100u )

/*
 * Type definitions
 */
/**
 * Initializer function type.
 */
typedef gos_result_t (*gos_initFunc_t) (void_t);

/**
 * Initializer structure type.
 */
typedef struct
{
    char_t         initDesc [32]; //!< Initialization descriptor text.
    gos_initFunc_t initFunc;      //!< Initializer function.
}gos_initStruct_t;

/*
 * External variables
 */
GOS_EXTERN gos_signalId_t kernelDumpReadySignal;

/*
 * Static variables
 */
/**
 * Initialization error flag.
 */
GOS_STATIC bool_t initError;

/**
 * Dump required flag.
 */
GOS_STATIC bool_t dumpRequired;

/**
 * Initializer function and description lookup table.
 */
GOS_STATIC gos_initStruct_t initializers [] =
{
    {"Queue service initialization"   , gos_queueInit},
    {"Trace service initialization"   , gos_traceInit},
    {"Signal service initialization"  , gos_signalInit},
    {"Time service initialization"    , gos_timeInit},
#if CFG_SHELL_USE_SERVICE == 1
    {"Shell service initialization"   , gos_shellInit},
#endif
    {"Message service initialization" , gos_messageInit},
    {"GCP service initialization"     , gos_gcpInit},
#if CFG_SYSMON_USE_SERVICE == 1
    {"Sysmon service initialization"  , gos_sysmonInit},
#endif
    {"User application initialization", gos_userApplicationInit}
};

/**
 * Initializer task ID.
 */
GOS_STATIC gos_tid_t systemTaskId;

/*
 * Function prototypes
 */
GOS_STATIC void_t       gos_systemTask (void_t);
GOS_STATIC gos_result_t gos_Start      (void_t);

/**
 * Initializer task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t systemTaskDesc =
{
    .taskFunction       = gos_systemTask,
    .taskName           = "gos_system_task",
    .taskPriority       = 0u,
    .taskStackSize      = CFG_SYSTEM_TASK_STACK_SIZE,
    .taskPrivilegeLevel = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Main
 */
int main (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t platformDriverInitResult;

    /*
     * Function code.
     */
    GOS_DISABLE_SCHED

    // Initialize platform drivers.
    platformDriverInitResult = gos_platformDriverInit();

    // Print startup logo.
    gos_printStartupLogo();

    // Pre-initialize.
    (void_t) gos_errorTraceInit("Platform driver initialization", platformDriverInitResult);

    // Initialize the kernel and register initializer task.
    if (gos_errorTraceInit("Kernel initialization", gos_kernelInit()) == GOS_SUCCESS &&
        gos_taskRegister(&systemTaskDesc, &systemTaskId) == GOS_SUCCESS)
    {
        initError = GOS_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    // Start OS.
    if (gos_Start() == GOS_ERROR)
    {
        (void_t) gos_errorHandler(GOS_ERROR_LEVEL_OS_FATAL, __func__, __LINE__, "Kernel could not be started.");
    }
    else
    {
        (void_t) gos_errorHandler(GOS_ERROR_LEVEL_OS_FATAL, __func__, __LINE__, "Initializer task could not be registered.");
    }

    for (;;);
}

/*
 * Function: gos_Dump
 */
void_t gos_Dump (void_t)
{
    /*
     * Local variables.
     */
    gos_taskPrivilegeLevel_t privileges    = 0u;
    gos_tid_t                currentTaskId = GOS_INVALID_TASK_ID;

    /*
     * Function code.
     */
    dumpRequired = GOS_TRUE;

    (void_t) gos_taskGetCurrentId(&currentTaskId);
    (void_t) gos_taskGetPrivileges(currentTaskId, &privileges);

    if ((privileges & GOS_PRIV_TASK_MANIPULATE) != GOS_PRIV_TASK_MANIPULATE)
    {
        (void_t) gos_taskAddPrivilege(currentTaskId, GOS_PRIV_TASK_MANIPULATE);
        (void_t) gos_taskWakeup(systemTaskId);
        (void_t) gos_taskRemovePrivilege(currentTaskId, GOS_PRIV_TASK_MANIPULATE);
    }
    else
    {
        (void_t) gos_taskWakeup(systemTaskId);
    }

    (void_t) gos_shellSuspend();
}

/*
 * Function: gos_platformDriverInit
 */
__attribute__((weak)) gos_result_t gos_platformDriverInit (void_t)
{
    /*
     * Function code.
     */
    (void_t) gos_errorHandler(GOS_ERROR_LEVEL_OS_WARNING, __func__, __LINE__, "Platform driver initializer missing!");
    return GOS_ERROR;
}

/*
 * Function: gos_userApplicationInit
 */
__attribute__((weak)) gos_result_t gos_userApplicationInit (void_t)
{
    /*
     * Function code.
     */
    (void_t) gos_errorHandler(GOS_ERROR_LEVEL_OS_WARNING, __func__, __LINE__, "User application initializer missing!");
    return GOS_ERROR;
}

/**
 * @brief   Starts the OS.
 * @details Checks whether the initializer function has set the error flag to GOS_FALSE,
 *          and if so, it starts the kernel (and thus the scheduling of tasks).
 *
 * @return  Result of OS starting.
 *
 * @retval  GOS_ERROR : OS not started due to initialization error or kernel start error.
 *
 * @remark  This function should only return with error. If the initialization is successful,
 *          the function is not expected to return.
 */
GOS_STATIC gos_result_t gos_Start (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t startStatus = GOS_BUSY;

    /*
     * Function code.
     */
    if (initError == GOS_FALSE)
    {
        startStatus = gos_kernelStart();
    }
    else
    {
        // Nothing to do.
    }

    return startStatus;
}

/**
 * @brief   Initializes the system.
 * @details Calls the OS service initializer functions and the user application initializer, and
 *          deletes itself.

 * @return  -
 */
GOS_STATIC void_t gos_systemTask (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t sysInitResult = GOS_SUCCESS;
    u8_t         initIndex     = 0u;

    /*
     * Function code.
     */
    GOS_DISABLE_SCHED

    // Loop through the initializers and call them while tracing the results.
    for (initIndex = 0u; initIndex < sizeof(initializers) / sizeof(gos_initStruct_t); initIndex++)
    {
    	GOS_CONCAT_RESULT(sysInitResult, gos_errorTraceInit(initializers[initIndex].initDesc, initializers[initIndex].initFunc()));
    }

    // Trace overall result.
    (void_t) gos_errorTraceInit("System initialization", sysInitResult);

    GOS_ENABLE_SCHED

    // Set priority to kernel-level.
    (void_t) gos_taskSetOriginalPriority(systemTaskId, CFG_TASK_SYS_PRIO);
    (void_t) gos_taskSetPriority(systemTaskId, CFG_TASK_SYS_PRIO);

    for (;;)
    {
        // Refresh task statistics.
        gos_kernelCalculateTaskCpuUsages(GOS_TRUE);

        if (dumpRequired == GOS_TRUE)
        {
            gos_kernelDump();
            gos_queueDump();

            // Invoke dump ready signal.
            (void_t) gos_signalInvoke(kernelDumpReadySignal, 0u);
            dumpRequired = GOS_FALSE;
            (void_t) gos_shellResume();
        }
        else
        {
            // Nothing to do.
        }

        (void_t) gos_taskSleep(GOS_SYS_TASK_SLEEP_TIME);
    }
}
