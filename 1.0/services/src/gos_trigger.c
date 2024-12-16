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
//! @file       gos_trigger.c
//! @author     Ahmed Gazar
//! @date       2023-11-15
//! @version    2.9
//!
//! @brief      GOS trigger service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_trigger.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-01-11    Ahmed Gazar     Initial version created
// 2.0        2023-05-04    Ahmed Gazar     *    Service completely reworked
// 2.1        2023-06-20    Ahmed Gazar     *    Timeout check moved to end of loop
// 2.2        2023-06-30    Ahmed Gazar     +    TRIGGER_WAIT_SLEEP_MS added
// 2.3        2023-07-12    Ahmed Gazar     *    TRIGGER_WAIT_SLEEP_MS renamed to
//                                               TRIGGER_WAIT_BLOCK_MS
//                                          *    Waiting changed from sleep to block
// 2.4        2023-09-07    Ahmed Gazar     *    Timeout handling rework
//                                          -    TRIGGER_WAIT_BLOCK_MS removed
// 2.5        2023-09-14    Ahmed Gazar     +    Mutex initialization result processing added
// 2.6        2023-10-04    Ahmed Gazar     +    gos_triggerDecrement added
// 2.7        2023-11-06    Ahmed Gazar     *    Trigger mutex replaced with atomic operations
//                                          *    gos_triggerDecrement return value added
//                                          +    gos_triggerInit parameter settings added
//                                          *    gos_triggerIncrement rework
//                                          +    Return value added to gos_triggerIncrement
// 2.8        2023-11-10    Ahmed Gazar     +    Return value added to gos_triggerReset
// 2.9        2023-11-15    Ahmed Gazar     *    Increment task unblock condition changed from
//                                               greater or equal to equal only
//                                          +    Decrement task unblock added if value equals
//                                               desired value
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
/*
 * Includes
 */
#include <gos_trigger.h>

/*
 * Function: gos_triggerInit
 */
gos_result_t gos_triggerInit (gos_trigger_t* pTrigger)
{
    /*
     * Local variables.
     */
    gos_result_t triggerInitResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Null pointer check.
    if (pTrigger != NULL)
    {
        pTrigger->valueCounter = 0u;
        pTrigger->desiredValue = 0u;
        pTrigger->waiterTaskId = GOS_INVALID_TASK_ID;
        triggerInitResult      = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return triggerInitResult;
}

/*
 * Function: gos_triggerReset
 */
GOS_INLINE gos_result_t gos_triggerReset (gos_trigger_t* pTrigger)
{
    /*
     * Local variables.
     */
    gos_result_t resetResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Null pointer check.
    if (pTrigger != NULL)
    {
        GOS_ATOMIC_ENTER

        pTrigger->valueCounter = 0u;
        pTrigger->desiredValue = 0u;

        resetResult = GOS_SUCCESS;

        GOS_ATOMIC_EXIT
    }
    else
    {
        // Nothing to do.
    }

    return resetResult;
}

/*
 * Function: gos_triggerWait
 */
GOS_INLINE gos_result_t gos_triggerWait (gos_trigger_t* pTrigger, u32_t value, u32_t timeout)
{
    /*
     * Local variables.
     */
    gos_result_t triggerWaitResult = GOS_ERROR;
    gos_tid_t    currentId         = GOS_INVALID_TASK_ID;

    /*
     * Function code.
     */
    // Null pointer check.
    if (pTrigger != NULL)
    {
        // Set owner task ID.
        GOS_ATOMIC_ENTER

        (void_t) gos_taskGetCurrentId(&currentId);
        pTrigger->waiterTaskId = currentId;
        pTrigger->desiredValue = value;

        GOS_ATOMIC_EXIT

        // Block until timeout in case trigger value has not been reached yet.
        if (pTrigger->valueCounter < pTrigger->desiredValue)
        {
            (void_t) gos_taskBlock(currentId, timeout);
        }
        else
        {
            // Trigger value already reached.
        }

        // Check if the trigger value is reached after unblock.
        GOS_ATOMIC_ENTER

        if (pTrigger->valueCounter >= pTrigger->desiredValue)
        {
            triggerWaitResult      = GOS_SUCCESS;
            pTrigger->waiterTaskId = GOS_INVALID_TASK_ID;
        }
        else
        {
            // Nothing to do.
        }

        GOS_ATOMIC_EXIT
    }
    else
    {
        // Nothing to do.
    }

    return triggerWaitResult;
}

/*
 * Function: gos_triggerIncrement
 */
GOS_INLINE gos_result_t gos_triggerIncrement (gos_trigger_t* pTrigger)
{
    /*
    * Local variables.
    */
    gos_result_t triggerIncrementResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Null pointer check.
    if (pTrigger != NULL)
    {
        GOS_ATOMIC_ENTER

        // Increment trigger value.
        pTrigger->valueCounter++;

        if (pTrigger->valueCounter == pTrigger->desiredValue)
        {
            GOS_ISR_ENTER

            // Unblock owner task.
            (void_t) gos_taskUnblock(pTrigger->waiterTaskId);

            GOS_ISR_EXIT
        }
        else
        {
            // Trigger value not reached yet.
        }

        triggerIncrementResult = GOS_SUCCESS;

        GOS_ATOMIC_EXIT
    }
    else
    {
        // Nothing to do.
    }

    return triggerIncrementResult;
}

/*
 * Function: gos_triggerDecrement
 */
GOS_INLINE gos_result_t gos_triggerDecrement (gos_trigger_t* pTrigger)
{
    /*
     * Local variables.
     */
    gos_result_t triggerDecrementResult = GOS_ERROR;

    /*
     * Function code.
     */
    // Null pointer check.
    if (pTrigger != NULL)
    {
        GOS_ATOMIC_ENTER

        if (pTrigger->valueCounter > 0u)
        {
            pTrigger->valueCounter -= 1u;

            if (pTrigger->valueCounter == pTrigger->desiredValue)
            {
                GOS_ISR_ENTER

                // Unblock owner task.
                (void_t) gos_taskUnblock(pTrigger->waiterTaskId);

                GOS_ISR_EXIT
            }
            else
            {
                // Trigger value not reached yet.
            }

            triggerDecrementResult  = GOS_SUCCESS;
        }
        else
        {
            // Trigger is already zero.
        }

        GOS_ATOMIC_EXIT
    }
    else
    {
        // Nothing to do.
    }

    return triggerDecrementResult;
}
