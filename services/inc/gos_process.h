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
//! @file       gos_process.h
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.4
//!
//! @brief      GOS process service header.
//! @details    Process service is an alternative of tasks in the GOS system. Processes are
//!             lightweight functions that do not contain a blocking loop and are periodically
//!             called from the process daemon task. These functions are 'memory-less' so if
//!             it is required to have different behaviors based on a program state, a global
//!             variable is needed. Processes have similar API functions to tasks. The stack size
//!             of a process cannot be explicitly setup, the maximum stack size of a processes is
//!             approximately the stack size of the process daemon task.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-22    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    Function descriptions updated
//                                          +    Service description added
//                                          +    License added
// 1.2        2022-12-11    Ahmed Gazar     +    Function descriptions completed
// 1.3        2023-06-17    Ahmed Gazar     *    Proces dump moved to function
//                                          *    Process run-time type modified
// 1.4        2023-07-12    Ahmed Gazar     +    procPrivileges added to gos_procDescriptor_t
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
#ifndef GOS_PROCESS_H
#define GOS_PROCESS_H
/*
 * Includes
 */
#include <gos_kernel.h>
#if CFG_PROC_USE_SERVICE == 1
/*
 * Macros
 */
/**
 * Default process ID.
 */
#define GOS_DEFAULT_PROC_ID    ( (gos_pid_t)0x5000 )

/**
 * Invalid process ID.
 */
#define GOS_INVALID_PROC_ID    ( (gos_pid_t)0x0200 )

/*
 * Type definitions
 */
/*
 *  Process-related definitions.
 */
typedef u16_t   gos_pid_t;                                 //!< Process ID type.
typedef char_t  gos_procName_t [CFG_PROC_MAX_NAME_LENGTH]; //!< Process name type.
typedef void_t  (*gos_proc_t)(void_t);                     //!< Process function type.
typedef u32_t   gos_procSleepTick_t;                       //!< Sleep tick type.
typedef u8_t    gos_procPrio_t;                            //!< Process priority type.
typedef u32_t   gos_procRunCounter_t;                      //!< Run counter type.
typedef u64_t   gos_procRunTime_t;                         //!< Run-time type.

/**
 *  Process state enumerator.
 *
 *  @note Hamming distance of
 *  - ready and sleeping:     3
 *  - ready and suspended:    4
 *  - sleeping and suspended: 3
 */
typedef enum
{
    GOS_PROC_READY     = 0b01010,    //!< GOS_PROC_READY
    GOS_PROC_SLEEPING  = 0b10110,    //!< GOS_PROC_SLEEPING
    GOS_PROC_SUSPENDED = 0b00101     //!< GOS_PROC_SUSPENDED
}gos_procState_t;

/**
 * Process descriptor structure.
 */
typedef struct
{
    gos_proc_t               procFunction;   //!< Process function.
    gos_procState_t          procState;      //!< Process state.
    gos_procPrio_t           procPriority;   //!< Process priority.
    gos_procName_t           procName;       //!< Process name.
    gos_pid_t                procId;         //!< Process ID.
    gos_procSleepTick_t      procSleepTicks; //!< Process sleep ticks.
    gos_procRunCounter_t     procRunCounter; //!< Process run counter.
    gos_runtime_t            procRunTime;    //!< Process run-time.
    u16_t                    procCpuUsage;   //!< Process processor usage in [%].
    gos_taskPrivilegeLevel_t procPrivileges; //!< Process privileges.
}gos_procDescriptor_t;

/**
 * Hook function type definitions.
 */
typedef void_t (*gos_procSwapHook_t    )(gos_pid_t, gos_pid_t ); //!< Process swap hook type.
typedef void_t (*gos_procIdleHook_t    )(void_t               ); //!< Process idle hook type.
typedef void_t (*gos_procSleepHook_t   )(gos_pid_t            ); //!< Process sleep hook type.
typedef void_t (*gos_procWakeupHook_t  )(gos_pid_t            ); //!< Process wake-up hook type.
typedef void_t (*gos_procSuspendHook_t )(gos_pid_t            ); //!< Process suspend hook type.
typedef void_t (*gos_procResumeHook_t  )(gos_pid_t            ); //!< Process resume hook type.

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the process service.
 * @details Initializes the internal process descriptor array, registers the
 *          process daemon and the process dump tasks, and suspends the process dump task.
 *
 * @return  Result of initialization.
 *
 * @retval  GOS_SUCCESS : Initialization successful.
 * @retval  GOS_ERROR   : Daemon/dump task registration or task suspension error.
 */
gos_result_t gos_procInit (
        void_t
        );

/**
 * @brief   Registers a new process for scheduling.
 * @details Checks the parameters and tries to find the next empty slot in the internal
 *          process array. If the array is not full and the process descriptor is valid,
 *          it registers the process in the free slot.
 *
 * @param   procDescriptor : Pointer to the corresponding process descriptor structure.
 * @param   procId         : Pointer to a process ID variable (can be NULL).
 *
 * @return  Result of process registration.
 *
 * @retval  GOS_SUCCESS    : Registration successful.
 * @retval  GOS_ERROR      : Process descriptor invalid or NULL or process array is full.
 */
gos_result_t gos_procRegister (
        gos_procDescriptor_t* procDescriptor,
        gos_pid_t*            procId
        );

/**
 * @brief   Sends the given process to sleeping state.
 * @details Checks whether the process is in ready state and if so, it sends it to
 *          sleeping state.
 *
 * @param   sleepTicks  : Ticks to sleep the process. After the system has increased with
 *                        the number of ticks, the process will be waken-up and ready for
 *                        scheduling.
 *
 * @return  Result of sleeping.
 *
 * @retval  GOS_SUCCESS : Process sent to sleeping state successfully.
 * @retval  GOS_ERROR   : Process is not in ready state or called from idle process.
 */
gos_result_t gos_procSleep (
        gos_procSleepTick_t sleepTicks
        );

/**
 * @brief   Wakes up a process.
 * @details Checks whether the process is in sleeping state and if so, it sends it to
 *          ready state.
 *
 * @param   procId      : Process ID.
 *
 * @return  Result of waking up.
 *
 * @retval  GOS_SUCCESS : Wake-up successful.
 * @retval  GOS_ERROR   : Invalid process ID or process is not in sleeping state.
 */
gos_result_t gos_procWakeup (
        gos_pid_t procId
        );

/**
 * @brief   Sends the given process to suspended state.
 * @details Checks whether the process is in ready or sleeping state and if so, it sends it to
 *          suspended state.
 *
 * @param   procId      : Process ID.
 *
 * @return  Result of process suspending.
 *
 * @retval  GOS_SUCCESS : Suspending successful.
 * @retval  GOS_ERROR   : Invalid process ID or process is not ready or sleeping.
 */
gos_result_t gos_procSuspend (
        gos_pid_t procId
        );

/**
 * @brief   Resumes a process.
 * @details Checks whether the process is in suspended state and if so, it sends it to
 *          ready state.
 *
 * @param   procId      : Process ID.
 *
 * @return  Result of process resuming.
 *
 * @retval  GOS_SUCCESS : Resuming successful.
 * @retval  GOS_ERROR   : Invalid process ID or process is not suspended.
 */
gos_result_t gos_procResume (
        gos_pid_t procId
        );

/**
 * @brief   Gets the process name.
 * @details Checks the process ID and copies the process name to the given variable.
 *
 * @param   procId      : Process ID.
 * @param   procName    : Process name character array (to store the
 *                        returned process name).
 *
 * @return  Result of name getting.
 *
 * @retval  GOS_SUCCESS : Name getting successful.
 * @retval  GOS_ERROR   : Invalid process ID or process name variable is NULL.
 */
gos_result_t gos_procGetName (
        gos_pid_t      procId,
        gos_procName_t procName
        );

/**
 * @brief   Gets the process ID.
 * @details Loops through the internal process array and compares each
 *          process name with the given process name. If there is a match,
 *          the process ID is copied to the given process ID variable.
 *
 * @param   procName    : Name of the process.
 * @param   procId      : Pointer to a process ID.
 *
 * @return  Result of ID getting.
 *
 * @retval  GOS_SUCCESS : ID getting successful.
 * @retval  GOS_ERROR   : Name not found or procId is NULL pointer.
 *
 * @remark  It is possible that multiple processes have the same name.
 *          In that case the process ID of the first match will be returned.
 */
gos_result_t gos_procGetId (
        gos_procName_t procName,
        gos_pid_t*     procId
        );

/**
 * @brief   Gets the process data.
 * @details Checks the process ID and copies the process data to the given variable.
 *
 * @param   procId      : Process ID.
 * @param   procData    : Pointer to a process descriptor structure.
 *
 * @return  Result of process data getting.
 *
 * @retval  GOS_SUCCESS : Data getting successful.
 * @retval  GOS_ERROR   : Invalid process ID or NULL pointer to data structure.
 */
gos_result_t gos_procGetData (
        gos_pid_t             procId,
        gos_procDescriptor_t* procData
        );

/**
 * @brief   Registers a swap hook.
 * @details Checks the given function and if another swap hook has already been registered,
 *          and it saves the given function.
 *
 * @param   swapHookFunction : Swap hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS      : Hook registration successful.
 * @retval  GOS_ERROR        : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterSwapHook (
        gos_procSwapHook_t swapHookFunction
        );

/**
 * @brief   Registers an idle hook.
 * @details Checks the given function and if another idle hook has already been registered,
 *          and it saves the given function.
 *
 * @param   idleHookFunction : Idle hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS      : Hook registration successful.
 * @retval  GOS_ERROR        : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterIdleHook (
        gos_procIdleHook_t idleHookFunction
        );

/**
 * @brief   Registers a sleep hook.
 * @details Checks the given function and if another sleep hook has already been registered,
 *          and it saves the given function.
 *
 * @param   sleepHookFunction : Sleep hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS       : Hook registration successful.
 * @retval  GOS_ERROR         : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterSleepHook (
        gos_procSleepHook_t sleepHookFunction
        );

/**
 * @brief   Registers a wake-up hook.
 * @details Checks the given function and if another wake-up hook has already been registered,
 *          and it saves the given function.
 *
 * @param   wakeupHookFunction : Wake-up hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS        : Hook registration successful.
 * @retval  GOS_ERROR          : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterWakeupHook (
        gos_procWakeupHook_t wakeupHookFunction
        );

/**
 * @brief   Registers a suspend hook.
 * @details Checks the given function and if another suspend hook has already been registered,
 *          and it saves the given function.
 *
 * @param   suspendHookFunction : Suspend hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS         : Hook registration successful.
 * @retval  GOS_ERROR           : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterSuspendHook (
        gos_procSuspendHook_t suspendHookFunction
        );

/**
 * @brief   Registers a resume hook.
 * @details Checks the given function and if another resume hook has already been registered,
 *          and it saves the given function.
 *
 * @param   resumeHookFunction : Resume hook function.
 *
 * @return  Result of hook function registration.
 *
 * @retval  GOS_SUCCESS        : Hook registration successful.
 * @retval  GOS_ERROR          : Hook already exists or given hook function is NULL pointer.
 */
gos_result_t gos_procRegisterResumeHook (
        gos_procResumeHook_t resumeHookFunction
        );

/**
 * @brief   Process dump.
 * @details Prints the process data of all processes to the trace output.
 *
 * @return  -
 */
void_t gos_procDump (
        void_t
        );

#endif
#endif
