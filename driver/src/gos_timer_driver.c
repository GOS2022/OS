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
//! @file       gos_tmr.c
//! @author     Ahmed Gazar
//! @date       2022-11-15
//! @version    1.1
//!
//! @brief      GOS timer driver source.
//! @details    For a more detailed description of this driver, please refer to
//!             @ref gos_timer_driver.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-11-10    Ahmed Gazar     Initial version created.
// 1.1        2022-11-15    Ahmed Gazar     +    License added
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
#include "gos_timer_driver.h"
#include "gos_driver.h"

/*
 * External variables
 */
GOS_EXTERN gos_driver_functions_t driverFunctions;

/*
 * Function: gos_timerDriverSysTimerGet
 */
gos_result_t gos_timerDriverSysTimerGet (u16_t* pValue)
{
    /*
     * Local variables.
     */
    gos_result_t timerDriverSysTimerGetResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (driverFunctions.timerDriverSysTimerGetValue != NULL)
    {
        timerDriverSysTimerGetResult = driverFunctions.timerDriverSysTimerGetValue(pValue);
    }

    return timerDriverSysTimerGetResult;
}
