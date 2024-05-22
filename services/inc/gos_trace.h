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
//! @file       gos_trace.h
//! @author     Ahmed Gazar
//! @date       2023-01-13
//! @version    2.1
//!
//! @brief      GOS trace service header.
//! @details    Trace service is a simple interface to send out strings via the configured trace
//!             periphery.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-24    Ahmed Gazar     Initial version created
// 1.1        2022-11-14    Ahmed Gazar     +    Formatted log function added
//                                          +    Descriptions added
// 1.2        2022-11-15    Ahmed Gazar     +    License added
// 1.3        2022-12-11    Ahmed Gazar     +    Font formatting macros added
//                                          +    Unsafe formatted log function added
//                                          *    Function descriptions updated
// 2.0        2022-12-20    Ahmed Gazar     Released
// 2.1        2023-01-13    Ahmed Gazar     *    Service renamed to trace
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
#ifndef GOS_TRACE_H
#define GOS_TRACE_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Macros
 */
#define TRACE_FORMAT_RESET        "\x1B[0m"     //!< Reset formatting.
#define TRACE_FG_RED_START        "\x1B[31m"    //!< Red foreground start.
#define TRACE_FG_GREEN_START      "\x1B[32m"    //!< Green foreground start.
#define TRACE_FG_YELLOW_START     "\x1B[33m"    //!< Yellow foreground start.
#define TRACE_FG_BLUE_START       "\x1B[34m"    //!< Blue foreground start.
#define TRACE_FG_MAGENTA_START    "\x1B[35m"    //!< Magenta foreground start.
#define TRACE_FG_CYAN_START       "\x1B[36m"    //!< Cyan foreground start.
#define TRACE_FG_WHITE_START      "\x1B[37m"    //!< White foreground start.

#define TRACE_BG_RED_START        "\x1b[41m"    //!< Red background start.
#define TRACE_BG_GREEN_START      "\x1b[42m"    //!< Green background start.
#define TRACE_BG_YELLOW_START     "\x1b[43m"    //!< Yellow background start.
#define TRACE_BG_BLUE_START       "\x1b[44m"    //!< Blue background start.
#define TRACE_BG_MAGENTA_START    "\x1b[45m"    //!< Magenta background start.
#define TRACE_BG_CYAN_START       "\x1b[46m"    //!< Cyan background start.
#define TRACE_BG_WHITE_START      "\x1b[47m"    //!< White background start.

#define TRACE_BOLD_START          "\x1B[1m"     //!< Bold start.
#define TRACE_ITALIC_START        "\x1B[3m"     //!< Italic start.
#define TRACE_UNDERLINE_START     "\x1B[4m"     //!< Underline start.
#define TRACE_STRIKETHROUGH_START "\x1B[9m"     //!< Strikethrough start.

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the trace service.
 * @details Creates a trace queue and registers the trace daemon in the kernel.
 *
 * @return  Result of initialization.
 *
 * @retval  GOS_SUCCESS : Initialization successful.
 * @retval  GOS_ERROR   : Queue creation or task registration error.
 */
gos_result_t gos_traceInit (
        void_t
        );

/**
 * @brief   Traces a given message.
 * @details Places the given message to the trace queue (for the trace daemon to print it).
 *
 * @param   addTimeStamp : Flag to indicate whether to add time-stamp or not.
 * @param   traceMessage : String to trace.
 *
 * @return  Result of tracing.
 *
 * @retval  GOS_SUCCESS : Tracing successful.
 * @retval  GOS_ERROR   : Queue put error.
 *
 * @remark  This function uses the queue service.
 */
gos_result_t gos_traceTrace (
        bool_t  addTimeStamp,
        char_t* traceMessage
        );

/**
 * @brief   Traces a given formatted message.
 * @details Prints the formatted message into a local buffer and places it
 *          to the trace queue (for the trace daemon to print it).
 *
 * @param   addTimeStamp : Flag to indicate whether to add time-stamp or not.
 * @param   traceFormat  : Formatter string.
 * @param   ...          : Optional parameters.
 *
 * @return  Result of formatted tracing.
 *
 * @retval  GOS_SUCCESS : Formatted tracing successful.
 * @retval  GOS_ERROR   : Queue put error.
 */
gos_result_t gos_traceTraceFormatted (
        bool_t            addTimeStamp,
        GOS_CONST char_t* traceFormat,
        ...
        );

/**
 * @brief   Traces a given formatted message.
 * @details Prints the formatted message into a local buffer and transmits it
 *          via the trace port.
 *
 * @param   traceFormat : Formatter string.
 * @param   ...         : Optional parameters.
 *
 * @return  Result of formatted tracing.
 *
 * @retval  GOS_SUCCESS : Message traced successfully.
 * @retval  GOS_ERROR   : Transmit error.
 */
gos_result_t gos_traceTraceFormattedUnsafe (
        GOS_CONST char_t* traceFormat,
        ...
        );

#endif
