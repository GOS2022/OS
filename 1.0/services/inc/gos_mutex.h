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
//                                      (c) Ahmed Gazar, 2023
//
//*************************************************************************************************
//! @file       gos_mutex.h
//! @author     Ahmed Gazar
//! @date       2023-09-14
//! @version    1.1
//!
//! @brief      GOS mutex service header.
//! @details    Mutex (Mutual Exclusion) service is provided for protecting shared resources.
//!             A mutex has two states: locked or unlocked. When a task calls the lock function,
//!             the service checks the mutex state, and it locks the mutex if it is unlocked.
//!             If a mutex is locked by another task, it waits in a non-blocking way (yields) for
//!             the mutex to be unlocked.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-05-04    Ahmed Gazar     Initial version created
// 1.1        2023-09-14    Ahmed Gazar     *    gos_mutexInit return value modified
//*************************************************************************************************
//
// Copyright (c) 2023 Ahmed Gazar
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
#ifndef GOS_MUTEX_H
#define GOS_MUTEX_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Macros
 */
/**
 * Mutex endless timeout.
 */
#define GOS_MUTEX_ENDLESS_TMO ( 0xFFFFFFFFu )

/**
 * Mutex no timeout.
 */
#define GOS_MUTEX_NO_TMO      ( 0x00000000u )

/*
 * Type definitions
 */
/**
 * Mutex state type.
 */
typedef enum
{
    GOS_MUTEX_UNLOCKED = 0b11010010, //!< Mutex unlocked.
    GOS_MUTEX_LOCKED   = 0b01101011  //!< Mutex locked.
}gos_mutexState_t;

/**
 * Mutex type.
 */
typedef struct
{
    gos_mutexState_t mutexState; //!< Mutex state.
    gos_tid_t        owner;      //!< Mutex owner task.
}gos_mutex_t;

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the mutex instance.
 * @details Sets the mutex state to unlocked.
 *
 * @param   pMutex : Pointer to the mutex to be initialized.
 *
 * @return  Result of initialization.
 *
 * @retval  GOS_SUCCESS : Mutex initialized successfully.
 * @retval  GOS_ERROR   : Mutex pointer is NULL.
 */
gos_result_t gos_mutexInit (
        gos_mutex_t* pMutex
        );

/**
 * @brief   Tries to lock the given mutex with the given timeout.
 * @details Waits in an unblocking way until the mutex is unlocked or
 *          the timeout value is reached. If the mutex becomes unlocked
 *          within the timeout value, it locks it.
 *
 * @param   pMutex  : Pointer to the mutex to be locked.
 * @param   timeout : Timeout value.
 *
 * @return  Result of mutex locking.
 *
 * @retval  GOS_SUCCESS : Mutex locked successfully.
 * @retval  GOS_ERROR   : Mutex could not be locked within the timeout value.
 */
gos_result_t gos_mutexLock (
        gos_mutex_t* pMutex,
        u32_t        timeout
        );

/**
 * @brief   Unlocks the mutex instance.
 * @details Sets the mutex state to unlocked.
 *
 * @param   pMutex : Pointer to the mutex to be unlocked.
 *
 * @return  Result of mutex unlocking.
 *
 * @retval  GOS_SUCCESS : Unlocking successful.
 * @retval  GOS_ERROR   : Mutex is NULL or caller is not the owner of the mutex.
 */
gos_result_t gos_mutexUnlock (
        gos_mutex_t* pMutex
        );
#endif
