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
//! @file       gos_crc_driver.h
//! @author     Ahmed Gazar
//! @date       2022-12-10
//! @version    1.0
//!
//! @brief      GOS Cyclic Redundancy Check driver header.
//! @details    This driver provides a simple 32-bit CRC calculator algorithm.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-10    Ahmed Gazar     Initial version created.
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
#ifndef GOS_CRC_DRIVER_H
#define GOS_CRC_DRIVER_H
/*
 * Includes
 */
#include "gos_kernel.h"

/*
 * Function prototypes
 */
/**
 * @brief   Calculates the 32-bit CRC value of the given data buffer.
 * @details Calculates the 32-bit CRC value of the given data buffer.
 *
 * @param   pData    : Pointer to the data buffer.
 * @param   dataSize : Size of the data buffer in bytes.
 *
 * @return  32-bit CRC value.
 */
u32_t gos_crcDriverGetCrc (u8_t* pData, u32_t dataSize);

#endif
