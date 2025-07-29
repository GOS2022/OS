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
//! @file       gos_shell_driver.h
//! @author     Ahmed Gazar
//! @date       2023-06-17
//! @version    1.1
//!
//! @brief      GOS SHELL driver header.
//! @details    This driver provides a skeleton for the driver for the shell service.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-11    Ahmed Gazar     Initial version created.
// 1.1        2023-06-17    Ahmed Gazar     +    Variable parameters added to
//                                               gos_shellDriverTransmitString
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
#ifndef GOS_SHELL_DRIVER_H
#define GOS_SHELL_DRIVER_H
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
typedef gos_result_t (*gos_shellDriverReceiveChar_t)(char_t*);

/**
 * Shell driver transmit string function type.
 */
typedef gos_result_t (*gos_shellDriverTransmitString_t)(char_t*);

/*
 * Function prototypes
 */
/**
 * @brief   Receives a character.
 * @details If registered, it calls the custom character receiver function.
 *
 * @param[out] pChar Pointer to a character variable to store the received character in.
 *
 * @return  Result of character reception.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_shellDriverReceiveChar (char_t* pChar);

/**
 * @brief   Transmits a string.
 * @details If registered, it calls the custom string transmitter function.
 *
 * @param[in] pString Pointer to the string to be transmitted.
 * @param[in] ...     Variable parameters for formatted strings.
 *
 * @return  Result of string transmission.
 *
 * @retval  #GOS_SUCCESS According to user implementation.
 * @retval  #GOS_ERROR   According to user implementation / function not registered.
 */
gos_result_t gos_shellDriverTransmitString (char_t* pString, ...);

#endif
