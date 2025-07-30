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
//! @date       2025-04-06
//! @version    3.1
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
// 3.0        2024-07-18    Ahmed Gazar     Service rework
// 3.1        2025-04-06    Ahmed Gazar     *    GOS_CONCAT_RESULT used for init
//                                          *    Max. chunk handling added
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
#include <gos_gcp.h>
#include <gos_mutex.h>
#include <string.h>

/*
 * Macros
 */
/**
 * GCP protocol version high byte.
 */
#define GCP_PROTOCOL_VERSION_MAJOR    ( 2 )

/**
 * GCP protocol version low byte.
 */
#define GCP_PROTOCOL_VERSION_MINOR    ( 0 )

/*
 * Type definitions
 */
/**
 * GCP acknowledge type.
 */
typedef enum
{
    GCP_ACK_REQ           = 0, //!< Request.
    GCP_ACK_OK            = 1, //!< OK.
    GCP_ACK_CRC_ERROR     = 2, //!< CRC error.
    GCP_ACK_RESEND        = 3, //!< Re-send request.
    GCP_ACK_SIZE_ERROR    = 4, //!< Size error.
    GCP_ACK_PV_ERROR      = 5, //!< Protocol version error.
    GCP_ACK_INVALID       = 6, //!< Invalid message.
}gos_gcpAck_t;

/**
 * GCP frame header type.
 */
typedef struct
{
    u8_t  protocolMajor;   //!< Protocol version major.
    u8_t  protocolMinor;   //!< Protocol version minor.
    u8_t  ackType;         //!< Acknowledge type.
    u8_t  dummy;           //!< Dummy byte (padding).
    u16_t messageId;       //!< Message ID.
    u16_t dataSize;        //!< Data size.
    u32_t dataCrc;         //!< Data CRC.
    u32_t headerCrc;       //!< Header CRC.
}gos_gcpHeaderFrame_t;

/**
 * GCP channel functions type.
 */
typedef struct
{
    gos_gcpTransmitFunction_t gcpTransmitFunction; //!< GCP transmit function.
    gos_gcpReceiveFunction_t  gcpReceiveFunction;  //!< GCP receive function.
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
GOS_STATIC GOS_INLINE gos_result_t gos_gcpTransmitMessageInternal (
        gos_gcpChannelNumber_t  channel,
        u16_t                   messageId,
        void_t*                 pMessagePayload,
        u16_t                   payloadSize,
        u16_t                   maxChunkSize
        );

GOS_STATIC GOS_INLINE gos_result_t gos_gcpReceiveMessageInternal (
        gos_gcpChannelNumber_t  channel,
        u16_t*                  pMessageId,
        void_t*                 pPayloadTarget,
        u16_t                   targetSize,
        u16_t                   maxChunkSize
        );

GOS_STATIC gos_result_t gos_gcpValidateHeader (
        gos_gcpHeaderFrame_t*   pHeader,
        gos_gcpAck_t*           pAck
        );

GOS_STATIC gos_result_t gos_gcpValidateData (
        gos_gcpHeaderFrame_t*   pHeader,
        void_t*                 pData,
        gos_gcpAck_t*           pAck
        );

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
        GOS_CONCAT_RESULT(gcpInitResult, gos_mutexInit(&gcpRxMutexes[mutexIdx]));
        GOS_CONCAT_RESULT(gcpInitResult, gos_mutexInit(&gcpTxMutexes[mutexIdx]));
    }

    return gcpInitResult;
}

/*
 * Function: gos_gcpRegisterPhysicalDriver
 */
gos_result_t gos_gcpRegisterPhysicalDriver (
        gos_gcpChannelNumber_t    channelNumber,
        gos_gcpTransmitFunction_t transmitFunction,
        gos_gcpReceiveFunction_t  receiveFunction
        )
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
        u16_t                   messageId,
        void_t*                 pMessagePayload,
        u16_t                   payloadSize,
        u16_t                   maxChunkSize
        )
{
    /*
     * Local variables.
     */
    gos_result_t transmitMessageResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (gos_mutexLock(&gcpTxMutexes[channel], GOS_MUTEX_ENDLESS_TMO) == GOS_SUCCESS)
    {
        transmitMessageResult = gos_gcpTransmitMessageInternal(channel, messageId, pMessagePayload, payloadSize, maxChunkSize);
    }
    else
    {
        // Mutex error.
    }

    (void_t) gos_mutexUnlock(&gcpTxMutexes[channel]);

    return transmitMessageResult;
}

/*
 * Function: gos_gcpReceiveMessage
 */
gos_result_t gos_gcpReceiveMessage (
        gos_gcpChannelNumber_t  channel,
        u16_t*                  pMessageId,
        void_t*                 pPayloadTarget,
        u16_t                   targetSize,
        u16_t                   maxChunkSize
        )
{
    /*
     * Local variables.
     */
    gos_result_t receiveMessageResult = GOS_ERROR;

    /*
     * Function code.
     */
    if (gos_mutexLock(&gcpRxMutexes[channel], GOS_MUTEX_ENDLESS_TMO) == GOS_SUCCESS)
    {
        receiveMessageResult = gos_gcpReceiveMessageInternal(channel, pMessageId, pPayloadTarget, targetSize, maxChunkSize);
    }
    else
    {
        // Nothing to do.
    }

    gos_mutexUnlock(&gcpRxMutexes[channel]);

    return receiveMessageResult;
}

/**
 * @brief   Internal transmitter function (re-entrant).
 * @details Transmits a message over GCP (header request, payload, and
 *          then receives a response header).
 *
 * @param[in] channel         GCP channel number.
 * @param[in] messageId       ID of the message.
 * @param[in] pMessagePayload Pointer to the payload buffer.
 * @param[in] payloadSize     Size of the payload (number of bytes).
 * @param[in] maxChunkSize    Maximum chunk size (in bytes).
 *
 * @return  Result of message transmission.
 *
 * @retval  #GOS_SUCCESS Transmission successful.
 * @retval  #GOS_ERROR   One of the function parameters are invalid or
 *                       there was a transmission or reception error.
 */
GOS_STATIC GOS_INLINE gos_result_t gos_gcpTransmitMessageInternal (
        gos_gcpChannelNumber_t  channel,
        u16_t                   messageId,
        void_t*                 pMessagePayload,
        u16_t                   payloadSize,
        u16_t                   maxChunkSize
)
{
    /*
     * Local variables.
     */
    gos_result_t         transmitMessageResult = GOS_ERROR;
    gos_gcpHeaderFrame_t requestHeaderFrame    = {0};
    gos_gcpHeaderFrame_t responseHeaderFrame   = {0};
    gos_gcpAck_t         headerAck             = (gos_gcpAck_t)0u;
    u8_t                 dataChunks            = 0u;
    u8_t                 chunkIndex            = 0u;
    u16_t                tempSize              = 0u;

    /*
     * Function code.
     */
    if ((pMessagePayload                              != NULL                        ||
        (pMessagePayload                              == NULL                        &&
        payloadSize                                   == 0u))                        &&
        channel                                       <  CFG_GCP_CHANNELS_MAX_NUMBER &&
        channelFunctions[channel].gcpTransmitFunction != NULL)
    {
        // Fill out header frame.
        requestHeaderFrame.ackType       = GCP_ACK_REQ;
        requestHeaderFrame.protocolMajor = GCP_PROTOCOL_VERSION_MAJOR;
        requestHeaderFrame.protocolMinor = GCP_PROTOCOL_VERSION_MINOR;
        requestHeaderFrame.dataSize      = payloadSize;
        requestHeaderFrame.messageId     = messageId;
        requestHeaderFrame.dataCrc       = gos_crcDriverGetCrc((u8_t*)pMessagePayload, payloadSize);
        requestHeaderFrame.headerCrc     = gos_crcDriverGetCrc((u8_t*)&requestHeaderFrame, (u32_t)(sizeof(requestHeaderFrame) - sizeof(requestHeaderFrame.headerCrc)));

        if (channelFunctions[channel].gcpTransmitFunction((u8_t*)&requestHeaderFrame, (u16_t)sizeof(requestHeaderFrame)) == GOS_SUCCESS)
        {
            if (requestHeaderFrame.dataSize == 0u)
            {
                if (channelFunctions[channel].gcpReceiveFunction((u8_t*)&responseHeaderFrame, (u16_t)sizeof(responseHeaderFrame)) == GOS_SUCCESS &&
                    gos_gcpValidateHeader(&responseHeaderFrame, &headerAck) == GOS_SUCCESS &&
                    responseHeaderFrame.ackType == GCP_ACK_OK    )
                {
                    // Transmission successful.
                    transmitMessageResult = GOS_SUCCESS;
                }
                else
                {
                    // Error.
                }
            }
            else
            {
                dataChunks = requestHeaderFrame.dataSize / maxChunkSize;

                if (requestHeaderFrame.dataSize % maxChunkSize != 0)
                {
                    dataChunks++;
                }
                else
                {
                    // Chunk number is exact.
                }

                for (chunkIndex = 0u; chunkIndex < dataChunks; chunkIndex++)
                {
                    if ((chunkIndex + 1) * maxChunkSize > requestHeaderFrame.dataSize)
                    {
                        tempSize = requestHeaderFrame.dataSize - chunkIndex * maxChunkSize;
                    }
                    else
                    {
                        tempSize = maxChunkSize;
                    }

                    if (channelFunctions[channel].gcpTransmitFunction((u8_t*)(pMessagePayload + chunkIndex * maxChunkSize), tempSize) == GOS_SUCCESS &&
                        channelFunctions[channel].gcpReceiveFunction((u8_t*)&responseHeaderFrame, (u16_t)sizeof(responseHeaderFrame)) == GOS_SUCCESS &&
                        gos_gcpValidateHeader(&responseHeaderFrame, &headerAck) == GOS_SUCCESS &&
                        responseHeaderFrame.ackType == GCP_ACK_OK    )
                    {
                        // Transmission successful.
                        // Set temporary success.
                        transmitMessageResult = GOS_SUCCESS;
                    }
                    else
                    {
                        // Error.
                        transmitMessageResult = GOS_ERROR;
                        break;
                    }
                }
            }
        }
        else
        {
            // Header frame transmit error.
        }
    }
    else
    {
        // Nothing to do.
    }

    return transmitMessageResult;
}


/**
 * @brief   Internal receiver function (re-entrant).
 * @details Receives a message over GCP (header, payload, and
 *          then transmits a response header).
 *
 * @param[in]  channel        GCP channel number.
 * @param[out] pMessageId     Pointer to a variable to store the message ID.
 * @param[out] pPayloadTarget Pointer to a buffer to store the payload.
 * @param[in]  targetSize     Size of the payload buffer (in bytes).
 * @param[in]  maxChunkSize   Maximum chunk size (in bytes).
 *
 * @return  Result of message reception.
 *
 * @retval  #GOS_SUCCESS Reception successful.
 * @retval  #GOS_ERROR   One of the function parameters are invalid or
 *                       request header validation failed or payload
 *                       validation failed.
 */
GOS_STATIC GOS_INLINE gos_result_t gos_gcpReceiveMessageInternal (
        gos_gcpChannelNumber_t  channel,
        u16_t*                  pMessageId,
        void_t*                 pPayloadTarget,
        u16_t                   targetSize,
        u16_t                   maxChunkSize
        )
{
    /*
     * Local variables.
     */
    gos_result_t         receiveMessageResult  = GOS_ERROR;
    gos_gcpHeaderFrame_t requestHeaderFrame    = {0};
    gos_gcpHeaderFrame_t responseHeaderFrame   = {0};
    gos_gcpAck_t         headerAck             = (gos_gcpAck_t)0u;
    u8_t                 dataChunks            = 0u;
    u8_t                 chunkIndex            = 0u;
    u16_t                tempSize              = 0u;

    /*
     * Function code.
     */
    if (pMessageId                                   != NULL                        &&
        pPayloadTarget                               != NULL                        &&
        channel                                      <  CFG_GCP_CHANNELS_MAX_NUMBER &&
        channelFunctions[channel].gcpReceiveFunction != NULL
        )
    {
        // Prepare response header frame.
        responseHeaderFrame.dataSize      = 0u;
        responseHeaderFrame.dataCrc       = 0u;
        responseHeaderFrame.protocolMajor = GCP_PROTOCOL_VERSION_MAJOR;
        responseHeaderFrame.protocolMinor = GCP_PROTOCOL_VERSION_MINOR;

        if (channelFunctions[channel].gcpReceiveFunction((u8_t*)&requestHeaderFrame, (u16_t)sizeof(requestHeaderFrame)) == GOS_SUCCESS &&
            gos_gcpValidateHeader(&requestHeaderFrame, &headerAck) == GOS_SUCCESS)
        {
            if (requestHeaderFrame.dataSize == 0)
            {
                // OK.
                // Data OK. Send response.
                *pMessageId = requestHeaderFrame.messageId;
                responseHeaderFrame.ackType = GCP_ACK_OK;
                responseHeaderFrame.headerCrc = gos_crcDriverGetCrc((u8_t*)&responseHeaderFrame, (u16_t)(sizeof(responseHeaderFrame) - sizeof(responseHeaderFrame.headerCrc)));
                if (channelFunctions[channel].gcpTransmitFunction((u8_t*)&responseHeaderFrame, (u16_t)sizeof(responseHeaderFrame)) == GOS_SUCCESS)
                {
                    // Reception successful.
                    receiveMessageResult = GOS_SUCCESS;
                }
                else
                {
                    // Transmit error.
                }
            }
            else
            {
                dataChunks = requestHeaderFrame.dataSize / maxChunkSize;

                if (requestHeaderFrame.dataSize % maxChunkSize != 0)
                {
                    dataChunks++;
                }
                else
                {
                    // Chunk number is exact.
                }

                for (chunkIndex = 0u; chunkIndex < dataChunks; chunkIndex++)
                {
                    if ((chunkIndex + 1) * maxChunkSize > requestHeaderFrame.dataSize)
                    {
                        tempSize = requestHeaderFrame.dataSize - chunkIndex * maxChunkSize;
                    }
                    else
                    {
                        tempSize = maxChunkSize;
                    }

                    if (channelFunctions[channel].gcpReceiveFunction((u8_t*)(pPayloadTarget + chunkIndex * maxChunkSize), tempSize) == GOS_SUCCESS)
                    {
                        // OK, send response.
                        // Data OK. Send response.
                        *pMessageId = requestHeaderFrame.messageId;
                        responseHeaderFrame.ackType = GCP_ACK_OK;
                        responseHeaderFrame.headerCrc = gos_crcDriverGetCrc((u8_t*)&responseHeaderFrame, (u16_t)(sizeof(responseHeaderFrame) - sizeof(responseHeaderFrame.headerCrc)));
                        if (channelFunctions[channel].gcpTransmitFunction((u8_t*)&responseHeaderFrame, (u16_t)sizeof(responseHeaderFrame)) == GOS_SUCCESS)
                        {
                            // OK continue.
                        }
                        else
                        {
                            // Transmit error.
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                // Integrity check.
                if (gos_gcpValidateData(&requestHeaderFrame, pPayloadTarget, &headerAck) == GOS_SUCCESS)
                {
                    receiveMessageResult = GOS_SUCCESS;
                }
                else
                {
                    // Integrity error.
                }
            }
        }
        else
        {
            // Send response.
            if (requestHeaderFrame.messageId != 0u)
            {
                responseHeaderFrame.ackType   = (u8_t)headerAck;
                responseHeaderFrame.headerCrc = gos_crcDriverGetCrc((u8_t*)&responseHeaderFrame, (u16_t)(sizeof(responseHeaderFrame) - sizeof(responseHeaderFrame.headerCrc)));
                (void_t) channelFunctions[channel].gcpTransmitFunction((u8_t*)&responseHeaderFrame, (u16_t)sizeof(responseHeaderFrame));
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return receiveMessageResult;
}

/**
 * @brief   Validates the given GCP header.
 * @details Checks the CRC and the protocol version of the header. Returns the acknowledge
 *          code in the variable passed as a pointer in case the validation fails.
 *
 * @param[in]  pHeader Pointer to the GCP header to validate.
 * @param[out] pAck    Pointer to an acknowledge variable to store the result in.
 *
 * @return  Result of validation.
 *
 * @retval  #GOS_SUCCESS Validation successful.
 * @retval  #GOS_ERROR   CRC or PV error or NULL pointer parameter.
 */
GOS_STATIC gos_result_t gos_gcpValidateHeader (gos_gcpHeaderFrame_t* pHeader, gos_gcpAck_t* pAck)
{
    /*
     * Local variables.
     */
    gos_result_t validateSuccess = GOS_ERROR;

    /*
     * Function code.
     */
    if (pHeader != NULL && pAck != NULL)
    {
        // Check header CRC.
        if (gos_crcDriverGetCrc((u8_t*)pHeader, (u16_t)(sizeof(*pHeader) - sizeof(pHeader->headerCrc))) == pHeader->headerCrc)
        {
            // Validate protocol version
            if (pHeader->protocolMajor == GCP_PROTOCOL_VERSION_MAJOR &&
                pHeader->protocolMinor == GCP_PROTOCOL_VERSION_MINOR)
            {
                validateSuccess = GOS_SUCCESS;
            }
            else
            {
                // Protocol version error.
                *pAck = GCP_ACK_PV_ERROR;
            }
        }
        else
        {
            *pAck = GCP_ACK_CRC_ERROR;
        }
    }
    else
    {
        // NULL pointer error.
    }

    return validateSuccess;
}

/**
 * @brief   Validates the given GCP payload/data.
 * @details Checks the CRC of the given payload buffer based on the GCP header passed as a
 *          parameter. Returns the acknowledge code in the variable passed as a pointer in
 *          case the validation fails.
 *
 * @param[in]  pHeader Pointer to the GCP header containing the payload data.
 * @param[in]  pData   Pointer to the data buffer to validate.
 * @param[out] pAck    Pointer to an acknowledge variable to store the result in.
 *
 * @return  Result of validation.
 *
 * @retval  #GOS_SUCCESS Validation successful.
 * @retval  #GOS_ERROR   CRC error or NULL pointer parameter.
 */
GOS_STATIC gos_result_t gos_gcpValidateData (gos_gcpHeaderFrame_t* pHeader, void_t* pData, gos_gcpAck_t* pAck)
{
    /*
     * Local variables.
     */
    gos_result_t validateSuccess = GOS_ERROR;

    /*
     * Function code.
     */
    if (pHeader != NULL && pData != NULL && pAck != NULL)
    {
        // Check data CRC.
        if (gos_crcDriverGetCrc((u8_t*)pData, (u16_t)(pHeader->dataSize)) == pHeader->dataCrc)
        {
            // Data OK.
            validateSuccess = GOS_SUCCESS;
        }
        else
        {
            *pAck = GCP_ACK_CRC_ERROR;
        }
    }
    else
    {
        // NULL pointer error.
    }

    return validateSuccess;
}
