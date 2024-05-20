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
//! @file       gos_shell_driver.c
//! @author     Ahmed Gazar
//! @date       2023-06-17
//! @version    1.1
//!
//! @brief      GOS SHELL driver source.
//! @details    For a more detailed description of this driver, please refer to
//!             @ref gos_shell_driver.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-11    Ahmed Gazar     Initial version created.
// 1.1        2023-06-17    Ahmed Gazar     +    Formatted string support added
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
#include "gos_shell_driver.h"
#include "gos_driver.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
 * Static variables
 */
/**
 * Buffer for formatted strings.
 */
GOS_STATIC char_t formattedBuffer [CFG_SHELL_COMMAND_BUFFER_SIZE];

/*
 * External variables
 */
GOS_EXTERN gos_driver_functions_t driverFunctions;

/*
 * Function: gos_shellDriverReceiveChar
 */
gos_result_t gos_shellDriverReceiveChar (char_t* pChar)
{
    /*
     * Local variables.
     */
    gos_result_t shellDriverReceiveResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (driverFunctions.shellDriverReceiveChar != NULL)
    {
        shellDriverReceiveResult = driverFunctions.shellDriverReceiveChar(pChar);
    }

    return shellDriverReceiveResult;
}

/*
 * Function: gos_shellDriverTransmitString
 */
gos_result_t gos_shellDriverTransmitString (char_t* pString, ...)
{
    /*
     * Local variables.
     */
    gos_result_t shellDriverTransmitResult = GOS_ERROR;
    va_list      args;

    /*
     * Function code.
     */
    va_start(args, pString);
    (void_t) vsprintf(formattedBuffer, pString, args);
    va_end(args);

    if (driverFunctions.shellDriverTransmitString != NULL)
    {
        shellDriverTransmitResult = driverFunctions.shellDriverTransmitString(formattedBuffer);
    }

    return shellDriverTransmitResult;
}
