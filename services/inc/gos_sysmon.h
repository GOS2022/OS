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
//! @file       gos_sysmon.h
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.1
//!
//! @brief      GOS system monitoring service header.
//! @details    This service is used to send and receive system information to an external client
//!             such as a PC tool.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-07-12    Ahmed Gazar     Initial version created
// 1.1        2024-02-13    Ahmed Gazar     +    gos_sysmonMessageReceivedCallback,
//                                               gos_sysmonUserMessageDescriptor_t, and
//                                               gos_sysmonRegisterUserMessage added
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
#ifndef GOS_SYSMON_H
#define GOS_SYSMON_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Type definitions
 */
/**
 * Sysmon message received callback function type.
 */
typedef void_t (*gos_sysmonMessageReceivedCallback) (void_t);

/**
 * User sysmon message descriptor.
 */
typedef struct
{
	u16_t                             messageId;       //!< Message ID.
	u16_t                             protocolVersion; //!< Message PV.
	void_t*                           payload;         //!< Pointer to payload target.
	u32_t                             payloadSize;     //!< Size of payload.
	gos_sysmonMessageReceivedCallback callback;        //!< Callback function pointer.
}gos_sysmonUserMessageDescriptor_t;

/*
 * Function prototypes
 */
/**
 * @brief   This function initializes the system monitoring service.
 * @details Registers the GCP physical driver and the sysmon daemon task in the kernel.
 *
 * @return  Result of initialization.
 *
 * @retval  #GOS_SUCCESS Sysmon initialization successful.
 * @retval  #GOS_ERROR   Sysmon daemon task registration failed.
 */
gos_result_t gos_sysmonInit (void_t);

/**
 * @brief   This function registers a custom user sysmon message.
 * @details Registers a custom sysmon message given by its ID and PV. The message
 *          will only be processed if the required ID is not an existing sysmon ID.
 *          When a message is received with the given ID and PV, the message content
 *          will be copied into the buffer defined in the descriptor structure, and
 *          the registered callback function will be called.
 *
 *          Recommended ID range: 0x6000 ... 0x9999.
 *
 * @param[in] pDesc Pointer to a sysmon user message descriptor.
 *
 * @return  Result of registration.
 *
 * @retval  #GOS_SUCCESS User message registered successfully.
 * @retval  #GOS_ERROR   Descriptor or callback is NULL or maximum number of user
 *                       messages has been reached.
 */
gos_result_t gos_sysmonRegisterUserMessage (gos_sysmonUserMessageDescriptor_t* pDesc);

#endif
