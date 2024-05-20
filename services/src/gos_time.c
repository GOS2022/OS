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
//! @file       gos_time.c
//! @author     Ahmed Gazar
//! @date       2023-10-04
//! @version    1.7
//!
//! @brief      GOS time service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_time.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-24    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    License added
//                                          -    Elapsed sender ID getter API functions removed
//                                          +    New elapsed sender ID enumerators are used
// 1.2        2022-12-08    Ahmed Gazar     +    Run-time handling added
// 1.3        2023-01-31    Ahmed Gazar     +    gos_runTimeAddMicroseconds added
// 1.4        2023-06-30    Ahmed Gazar     +    TIME_SLEEP_TIME_MS added
// 1.5        2023-07-25    Ahmed Gazar     +    gos_runTimeAddMilliseconds added
// 1.6        2023-09-08    Ahmed Gazar     *    gos_timeAddSeconds, gos_runTimeAddMicroseconds,
//                                               and gos_runTimeAddMilliseconds reworked for
//                                               optimized complexity and execution time
// 1.7        2023-10-04    Ahmed Gazar     *    System time handling strategy rework: now it is
//                                               systick-based and measured in milliseconds (system
//                                               runtime as well)
//                                          +    gos_timeAddMilliseconds added
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
#include <gos_time.h>
#include <gos_error.h>
#include <gos_signal.h>
#include <string.h>

/*
 * Macros
 */
/**
 * Default year.
 */
#define TIME_DEFAULT_YEAR     ( 2023 )

/**
 * Default month.
 */
#define TIME_DEFAULT_MONTH    ( GOS_TIME_JANUARY )

/**
 * Default day.
 */
#define TIME_DEFAULT_DAY      ( 1 )

/**
 * Time task sleep time in [ms].
 */
#define TIME_SLEEP_TIME_MS    ( 500u )

/*
 * Static variables
 */
/**
 * System time.
 */
GOS_STATIC gos_time_t systemTime =
{
    .years   = TIME_DEFAULT_YEAR,
    .months  = TIME_DEFAULT_MONTH,
    .days    = TIME_DEFAULT_DAY
};

/**
 * System run-time.
 */
GOS_STATIC gos_runtime_t systemRunTime;

/**
 * Number of days in each month - lookup table.
 */
GOS_STATIC GOS_CONST gos_day_t dayLookupTable [GOS_TIME_NUMBER_OF_MONTHS] =
{
        [GOS_TIME_JANUARY   - 1]    = 31,
        [GOS_TIME_FEBRUARY  - 1]    = 28,
        [GOS_TIME_MARCH     - 1]    = 31,
        [GOS_TIME_APRIL     - 1]    = 30,
        [GOS_TIME_MAY       - 1]    = 31,
        [GOS_TIME_JUNE      - 1]    = 30,
        [GOS_TIME_JULY      - 1]    = 31,
        [GOS_TIME_AUGUST    - 1]    = 31,
        [GOS_TIME_SEPTEMBER - 1]    = 30,
        [GOS_TIME_OCTOBER   - 1]    = 31,
        [GOS_TIME_NOVEMBER  - 1]    = 30,
        [GOS_TIME_DECEMBER  - 1]    = 31
};

/**
 * Time task ID.
 */
GOS_STATIC gos_tid_t timeDaemonTaskId;

/**
 * Time signal ID. This signal is invoked when a second has elapsed.
 */
gos_signalId_t timeSignalId;

/*
 * Function prototypes
 */
GOS_STATIC void_t gos_timeDaemonTask (void_t);

/**
 * Time task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t timeDaemonTaskDesc =
{
    .taskFunction         = gos_timeDaemonTask,
    .taskName             = "gos_time_daemon",
    .taskStackSize        = CFG_TASK_TIME_DAEMON_STACK,
    .taskPriority         = CFG_TASK_TIME_DAEMON_PRIO,
    .taskPrivilegeLevel   = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Function: gos_timeInit
 */
gos_result_t gos_timeInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t timeInitResult = GOS_SUCCESS;

    /*
     * Function code.
     */
    if (gos_signalCreate(&timeSignalId)                                != GOS_SUCCESS ||
        gos_taskRegister(&timeDaemonTaskDesc, &timeDaemonTaskId) != GOS_SUCCESS
        )
    {
        timeInitResult = GOS_ERROR;
    }
    else
    {
        // Nothing to do.
    }

    return timeInitResult;
}

/*
 * Function: gos_timeGet
 */
gos_result_t gos_timeGet (gos_time_t* pTime)
{
    /*
     * Local variables.
     */
    gos_result_t timeGetResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pTime != NULL)
    {
        (void_t) memcpy((void_t*)pTime, (GOS_CONST void_t*)&systemTime, sizeof(systemTime));

        timeGetResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return timeGetResult;
}

/*
 * Function: gos_timeSet
 */
gos_result_t gos_timeSet (gos_time_t* pTime)
{
    /*
     * Local variables.
     */
    gos_result_t timeSetResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pTime != NULL)
    {
        (void_t) memcpy((void_t*)&systemTime, (GOS_CONST void_t*)pTime, sizeof(*pTime));

        timeSetResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return timeSetResult;
}

/*
 * Function: gos_runTimeGet
 */
gos_result_t gos_runTimeGet (gos_runtime_t* pRunTime)
{
    /*
     * Local variables.
     */
    gos_result_t runtimeGetResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pRunTime != NULL)
    {
        (void_t) memcpy((void_t*)pRunTime, (void_t*)&systemRunTime, sizeof(systemRunTime));

        runtimeGetResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return runtimeGetResult;
}

/*
 * Function: gos_timeCompare
 */
gos_result_t gos_timeCompare (gos_time_t* pTime1, gos_time_t* pTime2, gos_timeComprareResult_t* result)
{
    /*
     * Local variables.
     */
    gos_result_t timeCompareResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pTime1 != NULL && pTime2 != NULL && result != NULL)
    {
        if (pTime1->hours   == pTime2->hours   &&
            pTime1->minutes == pTime2->minutes &&
            pTime1->seconds == pTime2->seconds)
        {
            *result = GOS_TIME_EQUAL;
        }
        else if (pTime1->hours < pTime2->hours || (
            pTime1->hours == pTime2->hours &&
            pTime1->minutes < pTime2->minutes) || (
            pTime1->hours == pTime2->hours &&
            pTime1->minutes == pTime2->minutes &&
            pTime1->seconds < pTime2->seconds))
        {
            *result = GOS_TIME_EARLIER;
        }
        else
        {
            *result = GOS_TIME_LATER;
        }
        timeCompareResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return timeCompareResult;
}

/*
 * Function: gos_timeAddMilliseconds
 */
gos_result_t gos_timeAddMilliseconds (gos_time_t* pTime, u16_t milliseconds)
{
    /*
     * Local variables.
     */
    gos_result_t timeAddMillisecondsResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pTime != NULL)
    {
        // 1. Raw increment.
        pTime->milliseconds += milliseconds;

        // 2. Post-adjustment.
        if (pTime->milliseconds >= 1000u)
        {
            pTime->seconds += pTime->milliseconds / 1000u;
            pTime->milliseconds %= 1000u;

            if (pTime->seconds >= 60u)
            {
                pTime->minutes += pTime->seconds / 60u;
                pTime->seconds %= 60u;

                if (pTime->minutes >= 60u)
                {
                    pTime->hours += pTime->minutes / 60u;
                    pTime->minutes %= 60u;

                    if (pTime->hours >= 24u)
                    {
                        pTime->days += pTime->hours / 24u;
                        pTime->hours %= 24u;

                        // Check days.
                        if (pTime->years % 4 == 0  && pTime->months == GOS_TIME_FEBRUARY && pTime->days >= 30)
                        {
                            pTime->days = 1u;
                            pTime->months++;
                        }
                        else if (pTime->years % 4 == 0 && pTime->months == GOS_TIME_FEBRUARY)
                        {
                            // Wait.
                        }
                        else if (pTime->days >= (dayLookupTable[pTime->months - 1] + 1))
                        {
                            pTime->days = 1u;
                            pTime->months++;
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        // Check months.
                        if (pTime->months == 13)
                        {
                            pTime->months = 1;
                            pTime->years++;
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

        timeAddMillisecondsResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return timeAddMillisecondsResult;
}

/*
 * Function: gos_timeAddSeconds
 */
gos_result_t gos_timeAddSeconds (gos_time_t* pTime, u16_t seconds)
{
    /*
     * Local variables.
     */
    gos_result_t timeAddSecondsResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pTime != NULL)
    {
        // 1. Raw increment.
        pTime->seconds += seconds;

        // 2. Post-adjustment.
        if (pTime->seconds >= 60u)
        {
            pTime->minutes += pTime->seconds / 60;
            pTime->seconds %= 60u;

            if (pTime->minutes >= 60u)
            {
                pTime->hours += pTime->minutes / 60;
                pTime->minutes %= 60u;

                if (pTime->hours >= 24u)
                {
                    pTime->days += pTime->hours / 24u;
                    pTime->hours %= 24u;

                    // Check days.
                    if (pTime->years % 4 == 0  && pTime->months == GOS_TIME_FEBRUARY && pTime->days >= 30)
                    {
                        pTime->days = 1u;
                        pTime->months++;
                    }
                    else if (pTime->years % 4 == 0 && pTime->months == GOS_TIME_FEBRUARY)
                    {
                        // Wait.
                    }
                    else if (pTime->days >= (dayLookupTable[pTime->months - 1] + 1))
                    {
                        pTime->days = 1u;
                        pTime->months++;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    // Check months.
                    if (pTime->months == 13)
                    {
                        pTime->months = 1;
                        pTime->years++;
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
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        timeAddSecondsResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return timeAddSecondsResult;
}

/*
 * Function: gos_runTimeAddMicroseconds
 */
gos_result_t gos_runTimeAddMicroseconds (gos_runtime_t* pRunTime1, gos_runtime_t* pRunTime2, u16_t microseconds)
{
    /*
     * Local variables.
     */
    gos_result_t runtimeAddMicrosecondsResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pRunTime1 != NULL)
    {
        // 1. Raw increment.
        pRunTime1->microseconds += microseconds;

        // 2. Post-adjustment.
        if (pRunTime1->microseconds >= 1000u)
        {
            pRunTime1->microseconds -= 1000u;
            pRunTime1->milliseconds += 1u;

            if (pRunTime1->milliseconds >= 1000u)
            {
                pRunTime1->milliseconds -= 1000u;
                pRunTime1->seconds += 1u;

                if (pRunTime1->seconds >= 60u)
                {
                    pRunTime1->seconds -= 60u;
                    pRunTime1->minutes += 1u;

                    if (pRunTime1->minutes >= 60u)
                    {
                        pRunTime1->minutes -= 60u;
                        pRunTime1->hours += 1u;

                        if (pRunTime1->hours >= 24u)
                        {
                            pRunTime1->hours -= 24u;
                            pRunTime1->days += 1u;
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
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if (pRunTime2 != NULL)
    {
        // 1. Raw increment.
        pRunTime2->microseconds += microseconds;

        // 2. Post-adjustment.
        if (pRunTime2->microseconds >= 1000u)
        {
            pRunTime2->microseconds -= 1000u;
            pRunTime2->milliseconds += 1u;

            if (pRunTime2->milliseconds >= 1000u)
            {
                pRunTime2->milliseconds -= 1000u;
                pRunTime2->seconds += 1u;

                if (pRunTime2->seconds >= 60u)
                {
                    pRunTime2->seconds -= 60u;
                    pRunTime2->minutes += 1u;

                    if (pRunTime2->minutes >= 60u)
                    {
                        pRunTime2->minutes -= 60u;
                        pRunTime2->hours += 1u;

                        if (pRunTime2->hours >= 24u)
                        {
                            pRunTime2->hours -= 24u;
                            pRunTime2->days += 1u;
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
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    runtimeAddMicrosecondsResult = GOS_SUCCESS;

    return runtimeAddMicrosecondsResult;
}

/*
 * Function: gos_runTimeAddMilliseconds
 */
gos_result_t gos_runTimeAddMilliseconds (gos_runtime_t* pRunTime, u16_t milliseconds)
{
    /*
     * Local variables.
     */
    gos_result_t runtimeAddMillisecondsResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (pRunTime != NULL)
    {
        // 1. Raw increment.
        pRunTime->milliseconds += milliseconds;

        // 2. Post-adjustment.
        if (pRunTime->milliseconds >= 1000u)
        {
            pRunTime->seconds += pRunTime->milliseconds / 1000u;
            pRunTime->milliseconds %= 1000u;

            if (pRunTime->seconds >= 60u)
            {
                pRunTime->minutes += pRunTime->seconds / 60u;
                pRunTime->seconds %= 60u;

                if (pRunTime->minutes >= 60u)
                {
                    pRunTime->hours += pRunTime->minutes / 60u;
                    pRunTime->minutes %= 60u;

                    if (pRunTime->hours >= 24u)
                    {
                        pRunTime->days += pRunTime->hours / 24u;
                        pRunTime->hours %= 24u;
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
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        runtimeAddMillisecondsResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return runtimeAddMillisecondsResult;
}

/*
 * Function: gos_runTimeAddSeconds
 */
gos_result_t gos_runTimeAddSeconds (gos_runtime_t* pRunTime, u32_t seconds)
{
    /*
     * Local variables.
     */
    gos_result_t runtimeAddSecondsResult = GOS_ERROR;
    u32_t        secondCounter           = 0u;

    /*
     * Function code.
     */
    if (pRunTime != NULL)
    {
        while (secondCounter++ < seconds)
        {
            pRunTime->seconds++;
            if (pRunTime->seconds >= 60)
            {
                pRunTime->seconds = 0U;
                pRunTime->minutes++;

                // Check minutes.
                if (pRunTime->minutes >= 60)
                {
                    pRunTime->minutes = 0U;
                    pRunTime->hours++;

                    // Check hours.
                    if (pRunTime->hours >= 24)
                    {
                        pRunTime->hours = 0U;
                        pRunTime->days++;
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
            else
            {
                // Nothing to do.
            }
        }
        runtimeAddSecondsResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return runtimeAddSecondsResult;
}

/*
 * Function: gos_timeIncreaseSystemTime
 */
gos_result_t gos_timeIncreaseSystemTime (u16_t milliseconds)
{
    /*
     * Local variables.
     */
    gos_result_t increaseSystemTimeResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (gos_runTimeAddMilliseconds(&systemRunTime, milliseconds) == GOS_SUCCESS &&
        gos_timeAddMilliseconds(&systemTime, milliseconds) == GOS_SUCCESS)
    {
        increaseSystemTimeResult = GOS_SUCCESS;
    }
    else
    {
        // One of the operations failed.
    }

    return increaseSystemTimeResult;
}

/**
 * @brief   Time daemon task.
 * @details Increases the system time approximately every second and invokes the
 *          elapsed signals.
 *
 * @return    -
 */
GOS_STATIC void_t gos_timeDaemonTask (void_t)
{
    /*
     * Local variables.
     */
    gos_time_t previousTime = {0};

    /*
     * Function code.
     */
    for (;;)
    {
        // Invoke second elapsed signal.
        if (systemTime.seconds > previousTime.seconds)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_SECOND_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Invoke minute elapsed signal.
        if (systemTime.minutes > previousTime.minutes)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_MINUTE_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Invoke hour elapsed signal.
        if (systemTime.hours > previousTime.hours)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_HOUR_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Invoke day elapsed signal.
        if (systemTime.days > previousTime.days)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_DAY_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Invoke month elapsed signal.
        if (systemTime.months > previousTime.months)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_MONTH_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Invoke year elapsed signal.
        if (systemTime.years > previousTime.years)
        {
            (void_t) gos_signalInvoke(timeSignalId, GOS_TIME_YEAR_ELAPSED_SENDER_ID);
        }
        else
        {
            // Nothing to do.
        }

        // Initialize previous time.
        (void_t) gos_timeGet(&previousTime);
        (void_t) gos_taskSleep(TIME_SLEEP_TIME_MS);
    }
}
