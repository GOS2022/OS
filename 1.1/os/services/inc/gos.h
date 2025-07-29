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
//! @file       gos.h
//! @author     Ahmed Gazar
//! @date       2025-04-06
//! @version    1.14
//!
//! @brief      GOS header.
//! @details    This header is a wrapper for the inclusion of all OS services and drivers for
//!             GOS2022 v1.1
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-10-28    Ahmed Gazar     Initial version created
// 1.1        2022-11-14    Ahmed Gazar     +    Header description added
//                                          +    Function descriptions added
// 1.2        2022-11-15    Ahmed Gazar     +    License added
// 1.3        2022-12-11    Ahmed Gazar     -    Function prototypes removed
// 1.4        2022-12-15    Ahmed Gazar     *    OS version number changed to v0.3
// 1.5        2023-06-17    Ahmed Gazar     +    gos_Dump added
//                                          *    OS version number changed to v0.4
// 1.6        2023-07-12    Ahmed Gazar     +    gos_sysmon.h include added
//                                          *    OS version number changed to v0.5
// 1.7        2023-11-06    Ahmed Gazar     *    OS version number changed to v0.6
// 1.8        2024-02-13    Ahmed Gazar     +    GOS_VERSION_MAJOR and GOS_VERSION_MINOR added
//                                          *    OS version number changed to v0.7
// 1.9        2024-04-02    Ahmed Gazar     *    OS version number changed to v0.8
// 1.10       2024-04-22    Ahmed Gazar     *    OS version number changed to v0.9
// 1.11       2024-04-24    Ahmed Gazar     -    Process service include removed
//                                          *    OS version number changed to v0.10
// 1.12       2024-07-18    Ahmed Gazar     *    OS version number changed to v0.11
// 1.13       2024-12-17    Ahmed Gazar     *    OS version number changed to v1.0
// 1.14       2025-04-06    Ahmed Gazar     *    OS version number changed to v1.1
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
#ifndef GOS_H
#define GOS_H
/*
 * Includes
 */
#include <gos_driver.h>
#include <gos_error.h>
#include <gos_gcp.h>
#include <gos_message.h>
#include <gos_mutex.h>
#include <gos_queue.h>
#include <gos_shell.h>
#include <gos_signal.h>
#include <gos_time.h>
#include <gos_trace.h>
#include <gos_trigger.h>

/*
 * Macros
 */
/**
 * OS major version.
 */
#define GOS_VERSION_MAJOR    ( 1 )

/**
 * OS minor version.
 */
#define GOS_VERSION_MINOR    ( 1 )

/*
 * Function prototypes
 */
/**
 * @brief   GOS dump.
 * @details Sets the dump required flag and wakes up the system task
 *          that calls the individual dump functions.
 *
 * @return  -
 */
void_t gos_Dump (
        void_t
        );

/**
 * @defgroup PlatformInitFunc Platform initializer weak functions
 * @{
 */
/**
 * @brief   Platform driver initializer. Used for the platform-specific driver initializations.
 * @details This function shall be defined by the user.
 *
 * @return  -
 *
 * @retval  GOS_ERROR -
 */
GOS_EXTERN gos_result_t gos_platformDriverInit (void_t);

/**
 * @brief   User application initializer. Used for the application-related initializations.
 * @details This function shall be defined by the user.
 *
 * @return  -
 *
 * @retval  GOS_ERROR -
 */
GOS_EXTERN gos_result_t gos_userApplicationInit (void_t);

/**
 * @}
 */
#endif
