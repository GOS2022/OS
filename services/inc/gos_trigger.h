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
//! @file       gos_trigger.h
//! @author     Ahmed Gazar
//! @date       2023-11-15
//! @version    2.4
//!
//! @brief      GOS trigger service header.
//! @details    Trigger service is a way of synchronizing tasks. A trigger instance works as a
//!             counter. A trigger can be incremented, and it can be waited on. When waiting for
//!             a trigger, the caller must specify a desired trigger value to be reached. Until
//!             the value is reached, the caller will wait in a non-blocking way. Once the value is
//!             reached, the caller returns.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-01-11    Ahmed Gazar     Initial version created
// 2.0        2023-05-04    Ahmed Gazar     *    Service completely reworked
// 2.1        2023-09-07    Ahmed Gazar     *    Trigger variables renamed
//                                          +    Trigger mutex added
// 2.2        2023-11-06    Ahmed Gazar     +    gos_triggerDecrement added
//                                          -    Trigger mutex removed
// 2.3        2023-11-10    Ahmed Gazar     +    Return value added to gos_triggerReset
// 2.4        2023-11-15    Ahmed Gazar     *    gos_triggerDecrement description corrected
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
#ifndef GOS_TRIGGER_H
#define GOS_TRIGGER_H
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
#define GOS_TRIGGER_ENDLESS_TMO ( 0xFFFFFFFF )

/**
 * Mutex no timeout.
 */
#define GOS_TRIGGER_NO_TMO      ( 0x00000000 )

/*
 * Type definitions
 */
/**
 * Trigger descriptor type.
 */
typedef struct
{
    u32_t       valueCounter; //!< Value counter.
    u32_t       desiredValue; //!< Desired value.
    gos_tid_t   waiterTaskId; //!< Owner task ID.
}gos_trigger_t;

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the trigger instance.
 * @details Calls the initializer for the trigger mutex.
 *
 * @param   pTrigger : Pointer to the trigger to be initialized.
 *
 * @return  Result of trigger initializing.
 *
 * @retval  GOS_SUCCESS : Trigger initialized successfully.
 * @retval  GOS_ERROR   : Trigger descriptor or trigger mutex is NULL pointer.
 */
gos_result_t gos_triggerInit (
        gos_trigger_t* pTrigger
        );

/**
 * @brief   Resets the trigger counter of the given trigger instance.
 * @details Sets the trigger counter of the given trigger instance to zero.
 *
 * @param   pTrigger : Pointer to the trigger instance.
 *
 * @return  Result of trigger resetting.
 *
 * @retval  GOS_SUCCESS : Trigger resetting successful.
 * @retval  GOS_ERROR   : Trigger is NULL pointer.
 */
gos_result_t gos_triggerReset (
        gos_trigger_t* pTrigger
        );

/**
 * @brief   Waits for the trigger instance to reach the given trigger value.
 * @details Increases the trigger waiter number, and waits in a non-blocking way
 *          until the desired trigger value is reached or the timeout is reached.
 *
 * @param   pTrigger : Pointer to the trigger instance.
 * @param   value    : The desired trigger value.
 * @param   timeout  : Timeout value.
 *
 * @return  Result of trigger waiting.
 *
 * @retval  GOS_SUCCESS : Trigger value reached.
 * @retval  GOS_ERROR   : Trigger value was not reached within the timeout value.
 */
gos_result_t gos_triggerWait (
        gos_trigger_t* pTrigger,
        u32_t          value,
        u32_t          timeout
        );

/**
 * @brief   Increments the trigger value of the given trigger.
 * @details Increments the trigger value of the given trigger.
 *
 * @param   pTrigger : Pointer to the trigger instance.
 *
 * @return  Result of trigger incrementing.
 *
 * @retval  GOS_SUCCESS : Incrementing successful.
 * @retval  GOS_ERROR   : Trigger is NULL pointer.
 */
gos_result_t gos_triggerIncrement (
        gos_trigger_t* pTrigger
        );

/**
 * @brief   Decrements the trigger value of the given trigger.
 * @details Decrements the trigger value of the given trigger.
 *
 * @param   pTrigger : Pointer to the trigger instance.
 *
 * @return  Result of trigger decrementing.
 *
 * @retval  GOS_SUCCESS : Decrementing successful.
 * @retval  GOS_ERROR   : Trigger is NULL pointer or value is already zero.
 */
gos_result_t gos_triggerDecrement (
        gos_trigger_t* pTrigger
        );

#endif
