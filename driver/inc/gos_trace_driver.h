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
//! @file       gos_trace_driver.h
//! @author     Ahmed Gazar
//! @date       2023-01-13
//! @version    1.2
//!
//! @brief      GOS trace driver header.
//! @details    This driver provides a skeleton for the driver for the trace service.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-11    Ahmed Gazar     Initial version created.
// 1.1        2022-12-13    Ahmed Gazar     +    Unsafe transmit string function added
// 1.2        2023-01-13    Ahmed Gazar     *    Service renamed to trace
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
#ifndef GOS_TRACE_DRIVER_H
#define GOS_TRACE_DRIVER_H
/*
 * Includes
 */
#include "gos_kernel.h"

/*
 * Type definitions
 */
/**
 * Trace driver transmit string function type.
 */
typedef gos_result_t (*gos_traceDriverTransmitString_t)(char_t*);

/**
 * Trace driver unsafe transmit string function type.
 */
typedef gos_result_t (*gos_traceDriverTransmitString_Unsafe_t)(char_t*);

/*
 * Function prototypes
 */
/**
 * @brief   Trace driver transmit string function skeleton.
 * @details If registered, it calls the custom transmit function.
 *
 * @param[in] pString String to trace.
 *
 * @return  Result of string transmission.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_traceDriverTransmitString (char_t* pString);

/**
 * @brief   Trace driver unsafe transmit string function skeleton.
 * @details If registered, it calls the custom unsafe transmit function.
 *
 * @param[in] pString String to trace.
 *
 * @return  Result of string transmission.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_traceDriverTransmitString_Unsafe (char_t* pString);

#endif
