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
//! @file       gos_sysmon_driver.c
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.0
//!
//! @brief      GOS SYSMON driver source.
//! @details    For a more detailed description of this driver, please refer to
//!             @ref gos_sysmon_driver.h
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
/*
 * Includes
 */
#include "gos_sysmon_driver.h"
#include "gos_driver.h"

/*
 * External variables
 */
GOS_EXTERN gos_driver_functions_t driverFunctions;

/*
 * Function: gos_sysmonDriverReceive
 */
gos_result_t gos_sysmonDriverReceive (u8_t* pBuffer, u16_t bufferSize)
{
    /*
     * Local variables.
     */
    gos_result_t sysmonDriverReceiveResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (driverFunctions.sysmonDriverReceive != NULL)
    {
        sysmonDriverReceiveResult = driverFunctions.sysmonDriverReceive(pBuffer, bufferSize);
    }

    return sysmonDriverReceiveResult;
}

/*
 * Function: gos_sysmonDriverTransmit
 */
gos_result_t gos_sysmonDriverTransmit (u8_t* pBuffer, u16_t bufferSize)
{
    /*
     * Local variables.
     */
    gos_result_t sysmonDriverTransmitResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (driverFunctions.sysmonDriverTransmit != NULL)
    {
        sysmonDriverTransmitResult = driverFunctions.sysmonDriverTransmit(pBuffer, bufferSize);
    }

    return sysmonDriverTransmitResult;
}
