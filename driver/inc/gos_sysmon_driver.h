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
//! @file       gos_sysmon_driver.h
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.0
//!
//! @brief      GOS SYSMON driver header.
//! @details    This driver provides a skeleton for the driver for the sysmon service.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-07-12    Ahmed Gazar     Initial version created.
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
#ifndef GOS_SYSMON_DRIVER_H
#define GOS_SYSMON_DRIVER_H
/*
 * Includes
 */
#include "gos_kernel.h"

/*
 * Type definitions
 */
/**
 * Shell driver receive character function type.
 */
typedef gos_result_t (*gos_sysmonDriverTransmit_t)(u8_t*, u16_t);

/**
 * Shell driver transmit string function type.
 */
typedef gos_result_t (*gos_sysmonDriverReceive_t)(u8_t*, u16_t);

/*
 * Function prototypes
 */
/**
 * @brief   It receives to the given buffer.
 * @details If registered, it calls the receiver function.
 *
 * @param[out] pBuffer    Target buffer to receive to.
 * @param[in]  bufferSize Size of the target buffer.
 *
 * @return  Result of reception.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_sysmonDriverReceive (
        u8_t* pBuffer,
        u16_t bufferSize
        );

/**
 * @brief   It transmits the given buffer.
 * @details If registered, it calls the transmitter function.
 *
 * @param[in] pBuffer    Buffer to transmit.
 * @param[in] bufferSize Size of the buffer.
 *
 * @return  Result of string transmission.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_sysmonDriverTransmit (
        u8_t* pBuffer,
        u16_t bufferSize
        );

#endif
