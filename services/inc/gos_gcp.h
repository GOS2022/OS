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
//! @file       gos_gcp.h
//! @author     Ahmed Gazar
//! @date       2022-12-20
//! @version    2.0
//!
//! @brief      GOS General Communication Protocol header.
//! @details    This service implements the GCP frame and message layers.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-10    Ahmed Gazar     Initial version created
// 1.1        2022-12-20    Ahmed Gazar     * Function description modified
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
#ifndef GOS_GCP_H
#define GOS_GCP_H
/*
 * Includes
 */
#include <gos_kernel.h>

/*
 * Type definitions
 */
#if CFG_GCP_CHANNELS_MAX_NUMBER < 255
typedef u8_t  gos_gcpChannelNumber_t;    //!< GCP channel number.
#else
typedef u16_t gos_gcpChannelNumber_t;    //!< GCP channel number.
#endif

/**
 * GCP physical layer transmit function type.
 */
typedef gos_result_t (*gos_gcpTransmitFunction_t) (u8_t*, u16_t);

/**
 * GCP physical layer receive function type.
 */
typedef gos_result_t (*gos_gcpReceiveFunction_t) (u8_t*, u16_t);

/**
 * GCP message header.
 */
typedef struct __attribute__ ((packed))
{
    u16_t protocolVersion; //!< Message protocol version.
    u16_t messageId;       //!< Message ID.
    u16_t payloadSize;     //!< Payload size.
    u32_t payloadCrc;      //!< Payload CRC.
}gos_gcpMessageHeader_t;

/*
 * Function prototypes
 */
/**
 * @brief   Initializes the GCP service.
 * @details Creates the GCP lock.
 *
 * @return  Result of initialization.
 *
 * @retval  GOS_SUCCESS : GCP service initialized successfully.
 * @retval  GOS_ERROR   : Lock creation error.
 */
gos_result_t gos_gcpInit (
        void_t
        );

/**
 * @brief   Registers the physical-layer transmit and receive driver functions.
 * @details Registers the physical-layer transmit and receive driver functions.
 *
 * @param   channel          : GCP channel.
 * @param   transmitFunction : Transmit function to register.
 * @param   receiveFunction  : Receive function to register.
 *
 * @return  Result of physical driver registration.
 *
 * @retval  GOS_SUCCESS      : Physical driver registration successful.
 * @retval  GOS_ERROR        : Transmit or receive function is NULL.
 */
gos_result_t gos_gcpRegisterPhysicalDriver (
        gos_gcpChannelNumber_t    channel,
        gos_gcpTransmitFunction_t transmitFunction,
        gos_gcpReceiveFunction_t  receiveFunction
        );

/**
 * @brief   Transmits the given message via the GCP protocol.
 * @details Calculates the payload CRC (if previously calculated in the header, it overwrites it!),
 *          transmits the header frames, and then transmits the payload frames.
 *
 * @param   channel         : GCP channel.
 * @param   pMessageHeader  : Pointer to the message header describing the message parameters.
 * @param   pMessagePayload : Pointer to the message payload.
 *
 * @return  Result of message transmission.
 *
 * @retval  GOS_SUCCESS     : Message transmitted successfully.
 * @retval  GOS_ERROR       : Message header, message payload or transmit function is NULL pointer or
 *                            frame transmission error occurred.
 */
gos_result_t gos_gcpTransmitMessage (
        gos_gcpChannelNumber_t  channel,
        gos_gcpMessageHeader_t* pMessageHeader,
        void_t*                 pMessagePayload
        );

/**
 * @brief   Receives the given message via the GCP protocol.
 * @details Receives the header frames and based on that, it receives the payload frames. Finally,
 *          it checks the payload CRC.
 *
 * @param   channel              : GCP channel.
 * @param   pTargetMessageHeader : Pointer to the message header describing the message parameters.
 * @param   pPayloadTarget       : Pointer to the payload target buffer.
 *
 * @return  Result of message reception.
 *
 * @retval  GOS_SUCCESS          : Message received successfully.
 * @retval  GOS_ERROR            : Message header target, payload target or receive function is NULL pointer or
 *                                 frame reception error occurred.
 */
gos_result_t gos_gcpReceiveMessage (
        gos_gcpChannelNumber_t  channel,
        gos_gcpMessageHeader_t* pTargetMessageHeader,
        void_t*                 pPayloadTarget
        );
#endif
