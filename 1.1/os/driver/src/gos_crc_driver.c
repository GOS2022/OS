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
//! @file       gos_crc_driver.c
//! @author     Ahmed Gazar
//! @date       2022-12-10
//! @version    1.0
//!
//! @brief      GOS Cyclic Redundancy Check driver source.
//! @details    For a more detailed description of this driver, please refer to
//!             @ref gos_crc_driver.h
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
/*
 * Includes
 */
#include "gos_crc_driver.h"

/*
 * Macros
 */
/**
 * CRC initializer value.
 */
#define CRC_INITIAL_VALUE       ( 0xFFFFFFFF )

/**
 * CRC polynomial value.
 */
#define CRC_POLYNOMIAL_VALUE    ( 0xEDB88320 )

/*
 * Function: gos_crcDriverGetCrc
 */
u32_t gos_crcDriverGetCrc (u8_t* pData, u32_t dataSize)
{
    /*
     * Local variables.
     */
    u32_t   crcValue   = CRC_INITIAL_VALUE;
    u32_t   index      = 0u;
    u32_t   mask       = 0u;
    u8_t    bitCounter = 0u;

    /*
     * Function code.
     */
    for (index = 0u; index < dataSize; index++)
    {
        crcValue ^= pData[index];

        for (bitCounter = 8u; bitCounter > 0u; bitCounter--)
        {
            mask = -(crcValue & 1);
            crcValue = (crcValue) ^ (CRC_POLYNOMIAL_VALUE & mask);
        }
    }

    return ~crcValue;
}
