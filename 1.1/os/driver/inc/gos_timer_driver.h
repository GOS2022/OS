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
//! @file       gos_timer_driver.h
//! @author     Ahmed Gazar
//! @date       2022-12-09
//! @version    1.0
//!
//! @brief      GOS timer driver header.
//! @details    This is the timer driver skeleton. It contains the required interface functions for
//!             the OS. These functions shall be implemented by the user in order to be able to use
//!             microsecond delay and task run-time monitoring.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-09    Ahmed Gazar     Initial version created.
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
#ifndef GOS_TIMER_DRIVER_H
#define GOS_TIMER_DRIVER_H
/*
 * Includes
 */
#include "gos_kernel.h"

/*
 * Type definitions
 */
/**
 * System timer value get function type.
 */
typedef gos_result_t (*gos_timerDriverSysTimerGetVal_t)(u16_t*);

/*
 * Function prototypes
 */
/**
 * @brief   System timer value getter skeleton.
 * @details If registered, it calls the custom system timer getter function.
 *
 * @param[out] pValue Pointer to the variable to store the timer value in.
 *
 * @return  Result of system timer value getting.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_timerDriverSysTimerGet (u16_t* pValue);
#endif
