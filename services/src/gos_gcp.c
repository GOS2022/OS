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
//! @file       gos_gcp.c
//! @author     Ahmed Gazar
//! @date       2023-09-14
//! @version    2.4
//!
//! @brief      GOS General Communication Protocol handler service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_gcp.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-12-10    Ahmed Gazar     Initial version created
// 1.1        2022-12-15    Ahmed Gazar     *    Frame number calculation bugfix
//                                          +    Multiple channel handling added
// 2.0        2022-12-20    Ahmed Gazar     Released
// 2.1        2023-05-04    Ahmed Gazar     *    Lock calls replaced with mutex calls
// 2.2        2023-07-12    Ahmed Gazar     *    Channel blocking bug fixed
// 2.3        2023-07-25    Ahmed Gazar     *    TX and RX mutexes separated and added for each
//                                               channel
// 2.4        2023-09-14    Ahmed Gazar     +    Mutex initialization result processing added
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
#include <gos_crc_driver.h>
#include <gos_error.h>
#include <gos_gcp.h>
#include <gos_mutex.h>
#include <string.h>

/*
 * Macros
 */
/**
 * Message frame size (bytes).
 */
#define FRAME_SIZE                  ( 64u )

/**
 * Frame protocol version high byte.
 */
#define FRAME_PROTOCOL_VERSION_H    ( 1 )

/**
 * Frame protocol version low byte.
 */
#define FRAME_PROTOCOL_VERSION_L    ( 0 )

/**
 * Frame protocol version.
 */
#define FRAME_PROTOCOL_VERSION      ( ( FRAME_PROTOCOL_VERSION_H << 8 ) + FRAME_PROTOCOL_VERSION_L )

/*
 * Type definitions
 */
/**
 * GCP frame header type.
 */
typedef struct
{
    u16_t protocolVersion; //!< Protocol version.
    u16_t sessionId;       //!< Session ID.
    u16_t dataSize;        //!< Data size.
    u16_t frameSize;       //!< Frame size.
    u32_t dataCrc;         //!< Data CRC.
    u32_t headerCrc;       //!< Header CRC.
}gos_gcpFrameHeader_t;

/**
 * GCP channel functions type.
 */
typedef struct
{
    gos_gcpTransmitFunction_t gcpTransmitFunction; //!< GCP transmit function.
    gos_gcpReceiveFunction_t  gcpReceiveFunction;  //!< GCP receive function.
    u16_t                     gcpSessionCounter;   //!< GCP session counter.
}gos_gcpChannelFunctions_t;

/*
 * Static variables
 */
/**
 * Channel functions.
 */
GOS_STATIC gos_gcpChannelFunctions_t channelFunctions [CFG_GCP_CHANNELS_MAX_NUMBER];

/**
 * GCP RX mutex array.
 */
GOS_STATIC gos_mutex_t gcpRxMutexes [CFG_GCP_CHANNELS_MAX_NUMBER];

/**
 * GCP TX mutex array.
 */
GOS_STATIC gos_mutex_t gcpTxMutexes [CFG_GCP_CHANNELS_MAX_NUMBER];

/*
 * Function prototypes
 */
GOS_STATIC gos_result_t gos_gcpTransmitFrames (gos_gcpChannelNumber_t channel, u8_t* pMessageBytes, u16_t messageSize);
GOS_STATIC gos_result_t gos_gcpReceiveFrames  (gos_gcpChannelNumber_t channel, u8_t* pTarget, u16_t targetSize);

/*
 * Function: gos_gcpInit
 */
gos_result_t gos_gcpInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t gcpInitResult = GOS_SUCCESS;
    u16_t        mutexIdx      = 0u;

    /*
     * Function code.
     */
    for (mutexIdx = 0u; mutexIdx < CFG_GCP_CHANNELS_MAX_NUMBER; mutexIdx++)
    {
        gcpInitResult &= gos_mutexInit(&gcpRxMutexes[mutexIdx]);
        gcpInitResult &= gos_mutexInit(&gcpTxMutexes[mutexIdx]);
    }

    if (gcpInitResult != GOS_SUCCESS)
    {
        gcpInitResult = GOS_ERROR;
    }

    return gcpInitResult;
}

/*
 * Function: gos_gcpRegisterPhysicalDriver
 */
gos_result_t gos_gcpRegisterPhysicalDriver (gos_gcpChannelNumber_t channelNumber,
                                            gos_gcpTransmitFunction_t transmitFunction,
                                            gos_gcpReceiveFunction_t receiveFunction)
{
    /*
     * Local variables.
     */
    gos_result_t registerPhysicalDriverResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (channelNumber < CFG_GCP_CHANNELS_MAX_NUMBER && transmitFunction != NULL && receiveFunction != NULL)
    {
        channelFunctions[channelNumber].gcpReceiveFunction  = receiveFunction;
        channelFunctions[channelNumber].gcpTransmitFunction = transmitFunction;
        registerPhysicalDriverResult                        = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return registerPhysicalDriverResult;
}

/*
 * Function: gos_gcpTransmitMessage
 */
gos_result_t gos_gcpTransmitMessage (
        gos_gcpChannelNumber_t  channel,
        gos_gcpMessageHeader_t* pMessageHeader,
        void_t*                 pMessagePayload
        )
{
    /*
     * Local variables.
     */
    gos_result_t transmitMessageResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (gos_mutexLock(&gcpTxMutexes[channel], GOS_MUTEX_ENDLESS_TMO) == GOS_SUCCESS    &&
        pMessageHeader                                  != NULL                        &&
        (pMessagePayload                                != NULL                        ||
        (pMessagePayload                                == NULL                        &&
        pMessageHeader->payloadSize                     == 0u))                        &&
        channel                                         <  CFG_GCP_CHANNELS_MAX_NUMBER &&
        channelFunctions[channel].gcpTransmitFunction   != NULL)
    {
        // Calculate payload CRC.
        pMessageHeader->payloadCrc = gos_crcDriverGetCrc((u8_t*)pMessagePayload, pMessageHeader->payloadSize);

        // Transmit message header and payload.
        if (gos_gcpTransmitFrames(channel, (u8_t*)pMessageHeader, (u16_t)sizeof(*pMessageHeader)) == GOS_SUCCESS &&
            gos_gcpTransmitFrames(channel, (u8_t*)pMessagePayload, pMessageHeader->payloadSize)   == GOS_SUCCESS
            )
        {
            transmitMessageResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    gos_mutexUnlock(&gcpTxMutexes[channel]);

    return transmitMessageResult;
}

/*
 * Function: gos_gcpReceiveMessage
 */
gos_result_t gos_gcpReceiveMessage (
        gos_gcpChannelNumber_t  channel,
        gos_gcpMessageHeader_t* pTargetMessageHeader,
        void_t*                 pPayloadTarget
        )
{
    /*
     * Local variables.
     */
    gos_result_t receiveMessageResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (gos_mutexLock(&gcpRxMutexes[channel], GOS_MUTEX_ENDLESS_TMO) == GOS_SUCCESS    &&
        pTargetMessageHeader                            != NULL                        &&
        pPayloadTarget                                  != NULL                        &&
        channel                                         <  CFG_GCP_CHANNELS_MAX_NUMBER &&
        channelFunctions[channel].gcpReceiveFunction    != NULL
        )
    {
        // Receive header and frames, check CRC.
        if (gos_gcpReceiveFrames(channel, (u8_t*)pTargetMessageHeader, (u16_t)sizeof(*pTargetMessageHeader)) == GOS_SUCCESS &&
            gos_gcpReceiveFrames(channel, (u8_t*)pPayloadTarget,       pTargetMessageHeader->payloadSize)    == GOS_SUCCESS &&
            pTargetMessageHeader->payloadCrc == gos_crcDriverGetCrc((u8_t*)pPayloadTarget, (u32_t)pTargetMessageHeader->payloadSize))
        {
            receiveMessageResult = GOS_SUCCESS;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    gos_mutexUnlock(&gcpRxMutexes[channel]);

    return receiveMessageResult;
}

/**
 * @brief   This function transmits the message frames.
 * @details This function prepares and transmits the header frame, and then it
 *          transmits the data frames.
 *
 * @param   channel       : GCP channel number.
 * @param   pMessageBytes : Pointer to the message buffer to be transmitted.
 * @param   messageSize   : Size of the message.
 *
 * @return  Result of frame transmission.
 *
 * @retval  GOS_SUCCESS   : Frame transmission successful.
 * @retval  GOS_ERROR     : Transmit function is NULL pointer or frame transmission failed.
 */
GOS_STATIC GOS_INLINE gos_result_t gos_gcpTransmitFrames (
        gos_gcpChannelNumber_t channel,
        u8_t* pMessageBytes,
        u16_t messageSize
        )
{
    /*
     * Local variables.
     */
    gos_result_t         transmitFramesResult = GOS_ERROR;
    gos_gcpFrameHeader_t frameHeader          = {0};
    u16_t                frameNumber          = 0u;
    u16_t                frameCounter         = 0u;

    /*
     * Function code.
     */
    if (channelFunctions[channel].gcpTransmitFunction != NULL)
    {
        // Fill out frame header.
        frameHeader.protocolVersion = FRAME_PROTOCOL_VERSION;
        frameHeader.dataSize        = messageSize;
        frameHeader.frameSize       = FRAME_SIZE;
        frameHeader.sessionId       = channelFunctions[channel].gcpSessionCounter++;
        frameHeader.dataCrc         = gos_crcDriverGetCrc(pMessageBytes, messageSize);
        frameHeader.headerCrc       = gos_crcDriverGetCrc((u8_t*)&frameHeader, (u32_t)(sizeof(frameHeader) - sizeof(frameHeader.headerCrc)));

        // Calculate frame number (with padding).
        frameNumber = messageSize / FRAME_SIZE + (messageSize % FRAME_SIZE != 0);

        // Transmit header.
        if (channelFunctions[channel].gcpTransmitFunction((u8_t*)&frameHeader, (u16_t)sizeof(frameHeader)) == GOS_SUCCESS)
        {
            // Transmit frames.
            for (frameCounter = 0u; frameCounter < frameNumber; frameCounter++)
            {
                // Transmit frame.
                if (channelFunctions[channel].gcpTransmitFunction((u8_t*)(pMessageBytes + (frameCounter * FRAME_SIZE)), (u16_t)FRAME_SIZE) != GOS_SUCCESS)
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if (frameCounter == frameNumber)
            {
                transmitFramesResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return transmitFramesResult;
}

/**
 * @brief   This function receives the message frames.
 * @details This function receives the header frame, and based on the data in the
 *          header, it receives the data frames.
 *
 * @param   channel     : GCP channel number.
 * @param   pTarget     : Target buffer to store the frames in.
 * @param   targetSize  : Size of target buffer.
 *
 * @return  Result of frame reception.
 *
 * @retval  GOS_SUCCESS : Frames received successfully.
 * @retval  GOS_ERROR   : Receive function is NULL pointer, frame reception error or
 *                        invalid message CRC.
 */
GOS_STATIC GOS_INLINE gos_result_t gos_gcpReceiveFrames (
        gos_gcpChannelNumber_t channel,
        u8_t* pTarget,
        u16_t targetSize
        )
{
    /*
     * Local variables.
     */
    gos_result_t         receiveFramesResult = GOS_ERROR;
    gos_gcpFrameHeader_t frameHeader         = {0};
    u16_t                frameNumber         = 0u;
    u16_t                frameCounter        = 0u;

    /*
     * Function code.
     */
    // Receive header.
    if (channelFunctions[channel].gcpReceiveFunction((u8_t*)&frameHeader, (u16_t)sizeof(frameHeader)) == GOS_SUCCESS)
    {
        // Check header integrity, protocol version and target size.
        if (frameHeader.headerCrc == gos_crcDriverGetCrc((u8_t*)&frameHeader, (u32_t)(sizeof(frameHeader) - sizeof(frameHeader.headerCrc))) &&
            frameHeader.protocolVersion == FRAME_PROTOCOL_VERSION &&
            targetSize >= frameHeader.dataSize)
        {
            // Calculate frame number.
            frameNumber = frameHeader.dataSize / frameHeader.frameSize + (frameHeader.dataSize % frameHeader.frameSize != 0);

            // Receive frames.
            for (frameCounter = 0u; frameCounter < frameNumber; frameCounter++)
            {
                // Receive frame.
                if (channelFunctions[channel].gcpReceiveFunction((u8_t*)(pTarget + frameCounter * frameHeader.frameSize), frameHeader.frameSize) != GOS_SUCCESS)
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            // Check data integrity.
            if (frameCounter == frameNumber &&
                (frameHeader.dataCrc == gos_crcDriverGetCrc(pTarget, (u32_t)frameHeader.dataSize) ||
                frameHeader.dataSize == 0u))
            {
                receiveFramesResult = GOS_SUCCESS;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return receiveFramesResult;
}
