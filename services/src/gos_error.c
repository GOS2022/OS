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
//! @file       gos_error.c
//! @author     Ahmed Gazar
//! @date       2025-04-06
//! @version    2.5
//!
//! @brief      GOS error handler service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_error.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-10    Ahmed Gazar     Initial version created
// 1.1        2022-12-13    Ahmed Gazar     *    Continuous scheduling disabling added to error
//                                               handler
// 2.0        2022-12-20    Ahmed Gazar     Released
// 2.1        2023-01-12    Ahmed Gazar     +    gos_traceResultToString added
//                                               Logo printing simplified
// 2.2        2023-06-30    Ahmed Gazar     *    Logo printing modified
//                                          +    CFG_RESET_ON_ERROR check added
// 2.3        2023-10-20    Ahmed Gazar     *    gos_printStartupLogo made weak
// 2.4        2024-04-24    Ahmed Gazar     *    Year in startup logo changed to 2022-2024
// 2.5        2025-04-06    Ahmed Gazar     *    Year in startup logo changed to 2022-2025
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
#include <gos_error.h>
#include <stdio.h>

/*
 * Macros
 */
/**
 * Separator line.
 */
#define SEPARATOR_LINE        "+------------------------------------------------------------------------+\r\n"

/**
 * Error buffer size.
 */
#define ERROR_BUFFER_SIZE     ( 80u )

/**
 * Success result string.
 */
#define RESULT_STRING_SUCCESS "  OK   "

/**
 * Error result string.
 */
#define RESULT_STRING_ERROR   " ERROR "

/**
 * Unknown result string.
 */
#define RESULT_STRING_UNKNOWN "UNKNOWN"

/*
 * Static variables
 */
/**
 * Buffer for error message formatting.
 */
GOS_STATIC char_t errorBuffer [ERROR_BUFFER_SIZE];

/*
 * Function prototypes
 */
GOS_STATIC char_t* gos_traceResultToString (gos_result_t* pResult);

/*
 * Function: gos_printStartupLogo
 */
__attribute__((weak)) void_t gos_printStartupLogo (void_t)
{
    /*
     * Function code.
     */
    (void_t) gos_traceTraceFormattedUnsafe("**************************************************************************\r\n\r\n");
    (void_t) gos_traceTraceFormattedUnsafe("                 #####             #####             #####                \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("               #########         #########         #########              \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("              ##                ##       ##       ##                      \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("             ##                ##         ##        #####                 \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("             ##     #####      ##         ##           #####              \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("              ##       ##       ##       ##                ##             \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("               #########         #########         #########              \r\n");
    (void_t) gos_traceTraceFormattedUnsafe("                 #####             #####             #####                \r\n\r\n");
    (void_t) gos_traceTraceFormattedUnsafe("                        (c) Ahmed Gazar, 2022-2025                        \r\n\r\n");
    (void_t) gos_traceTraceFormattedUnsafe("**************************************************************************\r\n");
}

/*
 * Function: gos_errorHandler
 */
void_t gos_errorHandler (gos_errorLevel_t errorLevel, const char_t* function, u32_t line, const char_t* errorMessage, ...)
{
    /*
     * Local variables.
     */
    va_list args;

    /*
     * Function code.
     */
    (void_t) gos_traceTraceFormattedUnsafe(TRACE_FORMAT_RESET"\r\n"SEPARATOR_LINE);

    if (errorLevel == GOS_ERROR_LEVEL_OS_FATAL)
    {
        (void_t) gos_traceTraceFormattedUnsafe(
                TRACE_FG_RED_START
                "OS-level error - system stopped.\r\n"
                TRACE_FORMAT_RESET
                );
    }
    else if (errorLevel == GOS_ERROR_LEVEL_OS_WARNING)
    {
        (void_t) gos_traceTraceFormattedUnsafe(
                TRACE_FG_YELLOW_START
                "OS-level warning.\r\n"
                TRACE_FORMAT_RESET
                );
    }
    else if (errorLevel == GOS_ERROR_LEVEL_USER_FATAL)
    {
        (void_t) gos_traceTraceFormattedUnsafe(
                TRACE_FG_RED_START
                "User-level error - system stopped.\r\n"
                TRACE_FORMAT_RESET
                );
    }
    else if (errorLevel == GOS_ERROR_LEVEL_USER_WARNING)
    {
        (void_t) gos_traceTraceFormattedUnsafe(
                TRACE_FG_YELLOW_START
                "User-level warning.\r\n"
                TRACE_FORMAT_RESET
                );
    }
    else
    {
        // Nothing to do.
    }

    if (function != NULL)
    {
        (void_t) gos_traceTraceFormattedUnsafe("Function: <"
                                  TRACE_FG_YELLOW_START
                                  "%s"
                                  TRACE_FORMAT_RESET
                                  ">, line: %d\r\n", function, line);
    }
    else if (line != 0)
    {
        (void_t) gos_traceTraceFormattedUnsafe("Function: <"
                                  TRACE_FG_YELLOW_START
                                  "unknown"
                                  TRACE_FORMAT_RESET
                                  ">, line: %d\r\n", line);
    }
    else
    {
        // Nothing to do.
    }

    if (errorMessage != NULL)
    {
        va_start(args, errorMessage);
        (void_t) vsprintf(errorBuffer, errorMessage, args);
        va_end(args);

        (void_t) gos_traceTraceFormattedUnsafe("%s\r\n", errorBuffer);
    }
    else
    {
        // Nothing to do.
    }

    (void_t) gos_traceTraceFormattedUnsafe(SEPARATOR_LINE);

    if (errorLevel == GOS_ERROR_LEVEL_OS_FATAL || errorLevel == GOS_ERROR_LEVEL_USER_FATAL)
    {
        GOS_DISABLE_SCHED
        GOS_ATOMIC_ENTER
        for (;;)
        {
            GOS_NOP;
#if CFG_RESET_ON_ERROR == 1

            gos_kernelDelayMs(CFG_RESET_ON_ERROR_DELAY_MS);
            gos_kernelReset();
#endif
        }
    }
    else
    {
        // Nothing to do.
    }
}

/*
 * Function: gos_errorTraceInit
 */
gos_result_t gos_errorTraceInit (const char_t* initDescription, gos_result_t initResult)
{
    /*
     * Function code.
     */
    (void_t) gos_traceTraceFormattedUnsafe("[%-5lu]    %-51s [ "
            "%s"
            " ]\r\n",
            gos_kernelGetSysTicks(),
            initDescription,
            gos_traceResultToString(&initResult));

    return initResult;
}

/**
 * @brief   Converts the result enumerator to a string.
 * @details Returns the formatted string version of the result enumerator.
 *
 * @param[out] pResult Pointer to the result variable.
 *
 * @return  Formatted string.
 */
GOS_STATIC char_t* gos_traceResultToString (gos_result_t* pResult)
{
    /*
     * Function code.
     */
    switch (*pResult)
    {
        case GOS_SUCCESS:
        {
            return TRACE_FG_GREEN_START RESULT_STRING_SUCCESS TRACE_FORMAT_RESET;
        }
        case GOS_ERROR:
        {
            return TRACE_FG_RED_START RESULT_STRING_ERROR TRACE_FORMAT_RESET;
        }
        default:
        {
            return TRACE_FG_YELLOW_START RESULT_STRING_UNKNOWN TRACE_FORMAT_RESET;
        }
    }
}
