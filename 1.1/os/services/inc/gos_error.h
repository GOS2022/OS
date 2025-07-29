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
//! @file       gos_error.h
//! @author     Ahmed Gazar
//! @date       2022-12-20
//! @version    2.0
//!
//! @brief      GOS error handler header.
//! @details    This service is used for tracing error and initialization messages on the log
//!             output.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-11    Ahmed Gazar     Initial version created
// 1.1        2022-12-20    Ahmed Gazar     * Function descriptions modified
// 2.0        2022-12-20    Ahmed Gazar     Released
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
#ifndef GOS_ERROR_H
#define GOS_ERROR_H
/*
 * Includes
 */
#include <gos_kernel.h>
#include <gos_trace.h>
#include <stdarg.h>

/*
 * Type definitions
 */
/**
 * Error level enumerator.
 */
typedef enum
{
    GOS_ERROR_LEVEL_OS_FATAL     = 0b110100,    //!< OS-level fatal error causing the system to stop.
    GOS_ERROR_LEVEL_OS_WARNING   = 0b101110,    //!< OS-level warning message will be logged, but system will not be stopped.
    GOS_ERROR_LEVEL_USER_FATAL   = 0b011010,    //!< User-level fatal error causing system stop.
    GOS_ERROR_LEVEL_USER_WARNING = 0b111010     //!< User-level warning.
}gos_errorLevel_t;

/*
 * Function prototypes
 */
/**
 * @brief   Prints the startup logo on the trace output.
 * @details Prints the GOS logo (similar to the ones used in the file headers) on the log output.
 *
 * @return    -
 */
void_t gos_printStartupLogo (void_t);

/**
 * @brief   Handles the given error.
 * @details Prints the formatted error message on the trace output, and based on the error
 *          level, it returns or stays in an infinite loop and disables scheduling.
 *
 * @param[in] errorLevel   Level of error (OS/user, warning/fatal).
 * @param[in] function     Function name.
 * @param[in] line         Line number.
 * @param[in] errorMessage Error message.
 * @param[in] ...          Formatter variable arguments.
 *
 * @return  -
 */
void_t gos_errorHandler (
        gos_errorLevel_t errorLevel,
        const char_t*    function,
        u32_t            line,
        const char_t*    errorMessage,
        ...
        );

/**
 * @brief   Traces an initialization message.
 * @details Writes the formatted initialization message on the trace output.
 *
 * @param[in] initDescription Message to describe the initialization step.
 * @param[in] initResult      Result of initialization.
 *
 * @return  Result of initialization.
 */
gos_result_t gos_errorTraceInit (
        const char_t* initDescription,
        gos_result_t  initResult
        );

#endif
