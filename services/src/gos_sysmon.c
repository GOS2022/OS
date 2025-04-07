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
//! @file       gos_sysmon.c
//! @author     Ahmed Gazar
//! @date       2025-03-26
//! @version    1.6
//!
//! @brief      GOS system monitoring service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_sysmon.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-07-12    Ahmed Gazar     Initial version created
// 1.1        2023-09-08    Ahmed Gazar     +    Missing comments added
//                                          *    Formatting fixed
// 1.2        2023-09-25    Ahmed Gazar     *    Local variables removed from sysmon task
//                                          +    New messages introduced
//                                          *    Component rework
// 1.3        2024-02-13    Ahmed Gazar     +    User message handling added
// 1.4        2024-04-22    Ahmed Gazar     *    Task modification options extended
// 1.5        2024-07-18    Ahmed Gazar     *    GCP update
//                                          *    Get all task data loop bug fixed
// 1.6        2025-03-26    Ahmed Gazar     *    Message IDs updated for proper separation
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
#include <gos_crc_driver.h>
#include <gos_time.h>
#include <gos_sysmon.h>
#include <gos_sysmon_driver.h>
#include <gos_gcp.h>
#include <string.h>

/*
 * Macros
 */
/**
 * Receive buffer size.
 */
#define RECEIVE_BUFFER_SIZE ( 4096u )

/*
 * Type definitions
 */
/**
 * System monitoring message ID enum.
 */
typedef enum
{
    GOS_SYSMON_MSG_UNKNOWN_ID                = 0,        //!< Unknown message ID.
    GOS_SYSMON_MSG_PING_ID                   = 0x0001,   //!< Ping message ID.
    GOS_SYSMON_MSG_PING_RESP_ID              = 0x0A01,   //!< Ping response message ID.
    GOS_SYSMON_MSG_CPU_USAGE_GET_ID          = 0x0002,   //!< CPU usage get message ID.
    GOS_SYSMON_MSG_CPU_USAGE_GET_RESP_ID     = 0x0A02,   //!< CPU usage get response message ID.
    GOS_SYSMON_MSG_TASK_GET_DATA_ID          = 0x0003,   //!< Task data get message ID.
    GOS_SYSMON_MSG_TASK_GET_DATA_RESP_ID     = 0x0A03,   //!< Task data get response message ID.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA_ID      = 0x0004,   //!< Task variable data get message ID.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP_ID = 0x0A04,   //!< Task variable data get response message ID.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE_ID      = 0x0005,   //!< Task modify state message ID.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP_ID = 0x0A05,   //!< Task modify state response ID.
    GOS_SYSMON_MSG_SYSRUNTIME_GET_ID         = 0x0006,   //!< System runtime get message ID.
    GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP_ID    = 0x0A06,   //!< System runtime get response message ID.
    GOS_SYSMON_MSG_SYSTIME_SET_ID            = 0x0007,   //!< System time set message ID.
    GOS_SYSMON_MSG_SYSTIME_SET_RESP_ID       = 0x0A07,   //!< System time set response ID.
    GOS_SYSMON_MSG_RESET_REQ_ID              = 0x0FFF,   //!< System reset request ID.
}gos_sysmonMessageId_t;

typedef enum
{
    GOS_SYSMON_MSG_UNKNOWN                   = 0,        //!< Unknown message LUT index.
    GOS_SYSMON_MSG_PING,                                 //!< Ping message LUT index.
    GOS_SYSMON_MSG_PING_RESP,                            //!< Ping response message LUT index.
    GOS_SYSMON_MSG_CPU_USAGE_GET,                        //!< CPU usage get message LUT index.
    GOS_SYSMON_MSG_CPU_USAGE_GET_RESP,                   //!< CPU usage get response message LUT index.
    GOS_SYSMON_MSG_TASK_GET_DATA,                        //!< Task data get message LUT index.
    GOS_SYSMON_MSG_TASK_GET_DATA_RESP,                   //!< Task data get response message LUT index.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA,                    //!< Task variable data get message LUT index.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP,               //!< Task variable data get response message LUT index.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE,                    //!< Task modify message LUT index.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP,               //!< Task modify response message LUT index.
    GOS_SYSMON_MSG_SYSRUNTIME_GET,                       //!< System runtime get message LUT index.
    GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP,                  //!< System runtime get response message LUT index.
    GOS_SYSMON_MSG_SYSTIME_SET,                          //!< System time set message LUT index.
    GOS_SYSMON_MSG_SYSTIME_SET_RESP,                     //!< System time set response message LUT index.
    GOS_SYSMON_MSG_RESET_REQ,                            //!< System reset message LUT index.
    GOS_SYSMON_MSG_NUM_OF_MESSAGES,                      //!< Number of messages.
}gos_sysmonMessageEnum_t;

/**
 * System monitoring message protocol version enum.
 */
typedef enum
{
    GOS_SYSMON_MSG_UNKNOWN_PV                = 1,        //!< Unknown protocol version.
    GOS_SYSMON_MSG_PING_PV                   = 1,        //!< Ping message protocol version.
    GOS_SYSMON_MSG_PING_ACK_PV               = 1,        //!< Ping acknowledge message protocol version.
    GOS_SYSMON_MSG_CPU_USAGE_GET_PV          = 1,        //!< CPU usage get message protocol version.
    GOS_SYSMON_MSG_CPU_USAGE_GET_RESP_PV     = 1,        //!< CPU usage get response message protocol version.
    GOS_SYSMON_MSG_TASK_GET_DATA_PV          = 1,        //!< Task data get message protocol version.
    GOS_SYSMON_MSG_TASK_GET_DATA_RESP_PV     = 1,        //!< Task data get response message protocol version.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA_PV      = 1,        //!< Task variable data get message protocol version.
    GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP_PV = 1,        //!< Task variable data get response message protocol version.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE_PV      = 1,        //!< Task modify state message protocol version.
    GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP_PV = 1,        //!< Task modify state response message protocol version.
    GOS_SYSMON_MSG_SYSRUNTIME_GET_PV         = 1,        //!< System runtime get message protocol version.
    GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP_PV    = 1,        //!< System runtime get response protocol version.
    GOS_SYSMON_MSG_SYSTIME_SET_PV            = 1,        //!< System time set message protocol version.
    GOS_SYSMON_MSG_SYSTIME_SET_RESP_PV       = 1,        //!< System time set response message protocol version.
    GOS_SYSMON_MSG_RESET_REQ_PV              = 1,        //!< System reset request message protocol version.
}gos_sysmonMessagePv_t;

/**
 * Message result enum.
 */
typedef enum
{
    GOS_SYSMON_MSG_RES_OK          = 40,                 //!< Message result OK.
    GOS_SYSMON_MSG_RES_ERROR       = 99,                 //!< Message result ERROR.
    GOS_SYSMON_MSG_INV_PV          = 35,                 //!< Invalid protocol version.
    GOS_SYSMON_MSG_INV_PAYLOAD_CRC = 28                  //!< Invalid payload CRC.
}gos_sysmonMessageResult_t;

/**
 * State modification enum.
 */
typedef enum
{
    GOS_SYSMON_TASK_MOD_TYPE_SUSPEND = 12,               //!< Task suspend.
    GOS_SYSMON_TASK_MOD_TYPE_RESUME  = 34,               //!< Task resume.
    GOS_SYSMON_TASK_MOD_TYPE_DELETE  = 49,               //!< Task delete.
    GOS_SYSMON_TASK_MOD_TYPE_BLOCK   = 52,               //!< Task block.
    GOS_SYSMON_TASK_MOD_TYPE_UNBLOCK = 63,               //!< Task unblock.
    GOS_SYSMON_TASK_MOD_TYPE_WAKEUP  = 74                //!< Task wakeup.
}gos_sysmonTaskModifyType_t;

/**
 * Task data message structure.
 */
typedef struct __attribute__((packed))
{
    gos_taskState_t          taskState;                  //!< Task state.
    gos_taskPrio_t           taskPriority;               //!< Task priority.
    gos_taskPrio_t           taskOriginalPriority;       //!< Task original priority.
    gos_taskPrivilegeLevel_t taskPrivilegeLevel;         //!< Task privilege level.
    gos_taskName_t           taskName;                   //!< Task name.
    gos_tid_t                taskId;                     //!< Task ID (internal).
    gos_taskCSCounter_t      taskCsCounter;              //!< Task context-switch counter.
    gos_taskStackSize_t      taskStackSize;              //!< Task stack size.
    gos_runtime_t            taskRunTime;                //!< Task run-time.
    u16_t                    taskCpuUsageLimit;          //!< Task CPU usage limit in [% x 100].
    u16_t                    taskCpuUsageMax;            //!< Task CPU usage max value in [% x 100].
    u16_t                    taskCpuUsage;               //!< Task processor usage in [% x 100].
    gos_taskStackSize_t      taskStackMaxUsage;          //!< Task max. stack usage.
}gos_sysmonTaskData_t;

/**
 * Task variable data message structure.
 */
typedef struct __attribute__((packed))
{
    gos_taskState_t          taskState;                  //!< Task state.
    gos_taskPrio_t           taskPriority;               //!< Task priority.
    gos_taskCSCounter_t      taskCsCounter;              //!< Task context-switch counter.
    gos_runtime_t            taskRunTime;                //!< Task run-time.
    u16_t                    taskCpuUsageMax;            //!< Task CPU usage max value in [% x 100].
    u16_t                    taskCpuUsage;               //!< Task processor usage in [% x 100].
    gos_taskStackSize_t      taskStackMaxUsage;          //!< Task stack usage.
}gos_sysmonTaskVariableData;

/**
 * Ping message structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
}gos_sysmonPingMessage_t;

/**
 * CPU usage message structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
    u16_t                     cpuUsage;                  //!< CPU usage x100[%].
}gos_sysmonCpuUsageMessage_t;

/**
 * Task data get message structure.
 */
typedef struct __attribute__((packed))
{
    u16_t taskIndex;                                     //!< Task index.
}gos_sysmonTaskDataGetMessage_t;

/**
 * Task data message structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
    gos_sysmonTaskData_t      taskData;                  //!< Task data.
}gos_sysmonTaskDataMessage_t;

/**
 * Task variable data message structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t  messageResult;            //!< Message result.
    gos_sysmonTaskVariableData taskVariableData;         //!< Task variable data.
}gos_sysmonTaskVariableDataMessage_t;

/**
 * Task modify message structure.
 */
typedef struct __attribute__((packed))
{
    u16_t                      taskIndex;                //!< Task index.
    gos_sysmonTaskModifyType_t modificationType;         //!< Task modification type.
    u32_t                      param;                    //!< Parameter for functions where timeout is required.
}gos_sysmonTaskModifyMessage_t;

/**
 * Task modify message result structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
}gos_sysmonTaskModifyResultMessage_t;

/**
 * System runtime get message result structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
    gos_runtime_t             sysRuntime;                //!< System runtime.
}gos_sysmonSysruntimeGetResultMessage_t;

/**
 * System time set message structure.
 */
typedef struct __attribute__((packed))
{
    gos_time_t desiredSystemTime;                        //!< Desired system time.
}gos_sysmonSystimeSetMessage_t;

/**
 * System time set message result structure.
 */
typedef struct __attribute__((packed))
{
    gos_sysmonMessageResult_t messageResult;             //!< Message result.
}gos_sysmonSystimeSetResultMessage_t;

/**
 * Message handler function type.
 */
typedef void_t (*gos_sysmonMessageHandler_t)(gos_sysmonMessageEnum_t);

/**
 * Look-up table entry structure.
 */
typedef struct
{
    gos_sysmonMessageId_t      messageId;                //!< Message ID.
    gos_sysmonMessagePv_t      messagePv;                //!< Message PV.
    void_t*                    pMessagePayload;          //!< Payload pointer.
    u16_t                      payloadSize;              //!< Payload size.
    gos_sysmonMessageHandler_t pHandler;                 //!< Handler function pointer.
}gos_sysmonLut_t;

/*
 * Static variables
 */
/**
 * Receive buffer.
 */
GOS_STATIC u8_t receiveBuffer [RECEIVE_BUFFER_SIZE];

/**
 * Ping message structure.
 */
GOS_STATIC gos_sysmonPingMessage_t                pingMessage                = {0};

/**
 * CPU load message structure.
 */
GOS_STATIC gos_sysmonCpuUsageMessage_t            cpuMessage                 = {0};

/**
 * Task data get message structure.
 */
GOS_STATIC gos_sysmonTaskDataGetMessage_t         taskDataGetMsg             = {0};

/**
 * Task data message structure.
 */
GOS_STATIC gos_sysmonTaskDataMessage_t            taskDataMsg                = {0};

/**
 * Task variable data message structure.
 */
GOS_STATIC gos_sysmonTaskVariableDataMessage_t    taskVariableDataMsg        = {0};

/**
 * Task descriptor structure.
 */
GOS_STATIC gos_taskDescriptor_t                   taskDesc                   = {0};

/**
 * Task modify message.
 */
GOS_STATIC gos_sysmonTaskModifyMessage_t          taskModifyMessage          = {0};

/**
 * Task modify result message.
 */
GOS_STATIC gos_sysmonTaskModifyResultMessage_t    taskModifyResultMessage    = {0};

/**
 * System runtime get message.
 */
GOS_STATIC gos_sysmonSysruntimeGetResultMessage_t sysRuntimeGetResultMessage = {0};

/**
 * System time set message.
 */
GOS_STATIC gos_sysmonSystimeSetMessage_t          sysTimeSetMessage          = {0};

/**
 * System time set result message.
 */
GOS_STATIC gos_sysmonSystimeSetResultMessage_t    sysTimeSetResultMessage    = {0};

/**
 * Sysmon user messages.
 */
GOS_STATIC gos_sysmonUserMessageDescriptor_t      userMessages [CFG_SYSMON_MAX_USER_MESSAGES];

/*
 * Function prototypes
 */
GOS_STATIC void_t                    gos_sysmonDaemonTask                (void_t);
GOS_STATIC gos_sysmonMessageEnum_t   gos_sysmonGetLutIndex               (gos_sysmonMessageId_t messageId);
GOS_STATIC void_t                    gos_sysmonSendResponse              (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandlePingRequest         (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleCpuUsageGet         (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleTaskDataGet         (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleTaskVariableDataGet (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleTaskModification    (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleSysRuntimeGet       (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleSystimeSet          (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC void_t                    gos_sysmonHandleResetRequest        (gos_sysmonMessageEnum_t lutIndex);
GOS_STATIC gos_sysmonMessageResult_t gos_sysmonCheckMessage              (gos_sysmonMessageEnum_t lutIndex);

/**
 * Sysmon daemon task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t sysmonDaemonTaskDesc =
{
    .taskFunction        = gos_sysmonDaemonTask,
    .taskName            = "gos_sysmon_daemon",
    .taskPriority        = CFG_TASK_SYSMON_DAEMON_PRIO,
    .taskStackSize       = CFG_TASK_SYSMON_DAEMON_STACK,
    .taskPrivilegeLevel  = GOS_TASK_PRIVILEGE_KERNEL
};

/**
 * Sysmon look-up table.
 */
GOS_STATIC GOS_CONST gos_sysmonLut_t sysmonLut [GOS_SYSMON_MSG_NUM_OF_MESSAGES] =
{
    [GOS_SYSMON_MSG_UNKNOWN]                =
    {
            .messageId       = GOS_SYSMON_MSG_UNKNOWN_ID,
            .messagePv       = GOS_SYSMON_MSG_UNKNOWN_PV,
            .pMessagePayload = NULL,
            .payloadSize     = 0u
    },
    [GOS_SYSMON_MSG_PING]                   =
    {
            .messageId       = GOS_SYSMON_MSG_PING_ID,
            .messagePv       = GOS_SYSMON_MSG_PING_PV,
            .pMessagePayload = NULL,
            .payloadSize     = 0u,
            .pHandler        = gos_sysmonHandlePingRequest
    },
    [GOS_SYSMON_MSG_PING_RESP]              =
    {
            .messageId       = GOS_SYSMON_MSG_PING_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_PING_ACK_PV,
            .pMessagePayload = (void_t*)&pingMessage,
            .payloadSize     = sizeof(pingMessage)
    },
    [GOS_SYSMON_MSG_CPU_USAGE_GET]          =
    {
            .messageId       = GOS_SYSMON_MSG_CPU_USAGE_GET_ID,
            .messagePv       = GOS_SYSMON_MSG_CPU_USAGE_GET_PV,
            .pMessagePayload = NULL,
            .payloadSize     = 0u,
            .pHandler        = gos_sysmonHandleCpuUsageGet
    },
    [GOS_SYSMON_MSG_CPU_USAGE_GET_RESP]     =
    {
            .messageId       = GOS_SYSMON_MSG_CPU_USAGE_GET_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_CPU_USAGE_GET_RESP_PV,
            .pMessagePayload = (void_t*)&cpuMessage,
            .payloadSize     = sizeof(cpuMessage)
    },
    [GOS_SYSMON_MSG_TASK_GET_DATA]          =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_GET_DATA_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_GET_DATA_PV,
            .pMessagePayload = (void_t*)&taskDataGetMsg,
            .payloadSize     = sizeof(taskDataGetMsg),
            .pHandler        = gos_sysmonHandleTaskDataGet
    },
    [GOS_SYSMON_MSG_TASK_GET_DATA_RESP]     =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_GET_DATA_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_GET_DATA_RESP_PV,
            .pMessagePayload = (void_t*)&taskDataMsg,
            .payloadSize     = sizeof(taskDataMsg)
    },
    [GOS_SYSMON_MSG_TASK_GET_VAR_DATA]      =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_GET_VAR_DATA_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_GET_VAR_DATA_PV,
            .pMessagePayload = (void_t*)&taskDataGetMsg,
            .payloadSize     = sizeof(taskDataGetMsg),
            .pHandler        = gos_sysmonHandleTaskVariableDataGet
    },
    [GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP] =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_GET_VAR_DATA_RESP_PV,
            .pMessagePayload = (void_t*)&taskVariableDataMsg,
            .payloadSize     = sizeof(taskVariableDataMsg)
    },
    [GOS_SYSMON_MSG_TASK_MODIFY_STATE]      =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_MODIFY_STATE_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_MODIFY_STATE_PV,
            .pMessagePayload = (void_t*)&taskModifyMessage,
            .payloadSize     = sizeof(taskModifyMessage),
            .pHandler        = gos_sysmonHandleTaskModification
    },
    [GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP] =
    {
            .messageId       = GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_TASK_MODIFY_STATE_RESP_PV,
            .pMessagePayload = (void_t*)&taskModifyResultMessage,
            .payloadSize     = sizeof(taskModifyResultMessage)
    },
    [GOS_SYSMON_MSG_SYSRUNTIME_GET]         =
    {
            .messageId       = GOS_SYSMON_MSG_SYSRUNTIME_GET_ID,
            .messagePv       = GOS_SYSMON_MSG_SYSRUNTIME_GET_PV,
            .pMessagePayload = NULL,
            .payloadSize     = 0u,
            .pHandler        = gos_sysmonHandleSysRuntimeGet
    },
    [GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP]    =
    {
            .messageId       = GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_SYSRUNTIME_GET_RESP_PV,
            .pMessagePayload = (void_t*)&sysRuntimeGetResultMessage,
            .payloadSize     = sizeof(sysRuntimeGetResultMessage)
    },
    [GOS_SYSMON_MSG_SYSTIME_SET]            =
    {
            .messageId       = GOS_SYSMON_MSG_SYSTIME_SET_ID,
            .messagePv       = GOS_SYSMON_MSG_SYSTIME_SET_PV,
            .pMessagePayload = (void_t*)&sysTimeSetMessage,
            .payloadSize     = sizeof(sysTimeSetMessage),
            .pHandler        = gos_sysmonHandleSystimeSet
    },
    [GOS_SYSMON_MSG_SYSTIME_SET_RESP]       =
    {
            .messageId       = GOS_SYSMON_MSG_SYSTIME_SET_RESP_ID,
            .messagePv       = GOS_SYSMON_MSG_SYSTIME_SET_RESP_PV,
            .pMessagePayload = (void_t*)&sysTimeSetResultMessage,
            .payloadSize     = sizeof(sysTimeSetResultMessage)
    },
    [GOS_SYSMON_MSG_RESET_REQ]              =
    {
            .messageId       = GOS_SYSMON_MSG_RESET_REQ_ID,
            .messagePv       = GOS_SYSMON_MSG_RESET_REQ_PV,
            .pMessagePayload = NULL,
            .payloadSize     = 0u,
            .pHandler        = gos_sysmonHandleResetRequest
    },
};

/*
 * Function: gos_sysmonInit
 */
gos_result_t gos_sysmonInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t sysmonInitResult = GOS_ERROR;
    u8_t         userMessageIndex = 0u;

    /*
     * Function code.
     */
    for (userMessageIndex = 0u; userMessageIndex < CFG_SYSMON_MAX_USER_MESSAGES; userMessageIndex++)
    {
        userMessages[userMessageIndex].callback        = NULL;
        userMessages[userMessageIndex].payload         = NULL;
        userMessages[userMessageIndex].payloadSize     = 0u;
        userMessages[userMessageIndex].protocolVersion = 0u;
        userMessages[userMessageIndex].messageId       = 0u;
    }

    if (gos_gcpRegisterPhysicalDriver(CFG_SYSMON_GCP_CHANNEL_NUM, gos_sysmonDriverTransmit,
        gos_sysmonDriverReceive) == GOS_SUCCESS &&
        gos_taskRegister(&sysmonDaemonTaskDesc, NULL) == GOS_SUCCESS)
    {
        sysmonInitResult = GOS_SUCCESS;
    }
    else
    {
        // Initialization failed.
    }

    return sysmonInitResult;
}

/*
 * Function: gos_sysmonRegisterUserMessage
 */
gos_result_t gos_sysmonRegisterUserMessage (gos_sysmonUserMessageDescriptor_t* pDesc)
{
    /*
     * Local variables.
     */
    gos_result_t registerResult   = GOS_ERROR;
    u8_t         userMessageIndex = 0u;

    /*
     * Function code.
     */
    if (pDesc != NULL && pDesc->callback != NULL)
    {
        for (userMessageIndex = 0u; userMessageIndex < CFG_SYSMON_MAX_USER_MESSAGES; userMessageIndex++)
        {
            if (userMessages[userMessageIndex].messageId == 0u &&
                userMessages[userMessageIndex].callback == NULL)
            {
                // Store user message descriptor.
                (void_t) memcpy((void_t*)&userMessages[userMessageIndex], (void_t*)pDesc, sizeof(*pDesc));
                registerResult = GOS_SUCCESS;
                break;
            }
        }
    }
    else
    {
        // Error.
    }

    return registerResult;
}

/**
 * @brief   System monitoring daemon task.
 * @details Serves the incoming system monitoring messages.
 *
 * @return -
 */
GOS_STATIC void_t gos_sysmonDaemonTask (void_t)
{
    /*
     * Local variables.
     */
    gos_sysmonMessageEnum_t lutIndex         = 0u;
    u8_t                    userMessageIndex = 0u;
    u16_t                   messageId        = 0u;

    /*
     * Function code.
     */
    for (;;)
    {
        // Reset message ID.
        messageId = 0u;

        // Check if a message was received.
        if (gos_gcpReceiveMessage(CFG_SYSMON_GCP_CHANNEL_NUM, &messageId, receiveBuffer, RECEIVE_BUFFER_SIZE, 0xFFFF) == GOS_SUCCESS)
        {
            // Get LUT index.
            lutIndex = gos_sysmonGetLutIndex(messageId);

            // Check user registered messages.
            if (lutIndex == GOS_SYSMON_MSG_UNKNOWN || lutIndex == GOS_SYSMON_MSG_NUM_OF_MESSAGES)
            {
                for (userMessageIndex = 0u; userMessageIndex < CFG_SYSMON_MAX_USER_MESSAGES; userMessageIndex++)
                {
                    if (userMessages[userMessageIndex].messageId == messageId)
                    {
                        // If payload is not NULL, copy it.
                        if (userMessages[userMessageIndex].payload != NULL)
                        {
                            (void_t) memcpy(userMessages[userMessageIndex].payload, (void_t*)receiveBuffer, userMessages[userMessageIndex].payloadSize);
                        }
                        else
                        {
                            // Message has no payload.
                        }

                        // Call callback function.
                        if (userMessages[userMessageIndex].callback != NULL)
                        {
                        	userMessages[userMessageIndex].callback();
                        }
                        else
                        {
                        	// NULL pointer.
                        }
                    }
                    else
                    {
                        // Message error.
                    }
                }
            }
            else
            {
                if (sysmonLut[lutIndex].pHandler != NULL)
                {
                    sysmonLut[lutIndex].pHandler(lutIndex);
                }
                else
                {
                    gos_sysmonSendResponse(GOS_SYSMON_MSG_UNKNOWN);
                }
            }
        }
        else
        {
            // Reception error.
            (void_t) gos_taskSleep(10);
        }
    }
}

/**
 * @brief   Gets the LUT index of the given message.
 * @details Returns the look-up table index that belongs to the given message ID.
 *
 * @param   messageId : ID of the message to get the index for.
 *
 * @return  Look-up table index of the message.
 */
GOS_STATIC gos_sysmonMessageEnum_t gos_sysmonGetLutIndex (gos_sysmonMessageId_t messageId)
{
    /*
     * Local variables.
     */
    gos_sysmonMessageEnum_t index = 0u;

    /*
     * Function code.
     */
    for (index = 0u; index < GOS_SYSMON_MSG_NUM_OF_MESSAGES; index++)
    {
        if (sysmonLut[index].messageId == messageId)
        {
            break;
        }
        else
        {
            // Continue searching.
        }
    }

    return index;
}

/**
 * @brief   Sends the response to the given message.
 * @details Fills out the remaining transmit header parameters (except for the result field),
 *          and transmits the message with the corresponding payload.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonSendResponse (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    (void_t) gos_gcpTransmitMessage(
            CFG_SYSMON_GCP_CHANNEL_NUM,          sysmonLut[lutIndex].messageId,
            sysmonLut[lutIndex].pMessagePayload, sysmonLut[lutIndex].payloadSize,
			0xFFFF
            );
}

/**
 * @brief   Handles the ping request.
 * @details Sends a ping response.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandlePingRequest (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    pingMessage.messageResult = gos_sysmonCheckMessage(lutIndex);
    gos_sysmonSendResponse(lutIndex + 1);
}

/**
 * @brief   Handles the CPU usage get request.
 * @details Sends out the current CPU usage.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleCpuUsageGet (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    cpuMessage.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (cpuMessage.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        cpuMessage.cpuUsage = gos_kernelGetCpuUsage();
    }
    else
    {
        // Message error.
    }

    gos_sysmonSendResponse(lutIndex + 1);
}

/**
 * @brief   Handles the task data get request.
 * @details Sends out the static task data for the requested task.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleTaskDataGet (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Local variables.
     */
    u16_t  taskIndex = 0u;
    bool_t breakLoop = GOS_FALSE;

    /*
     * Function code.
     */
    taskDataMsg.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (taskDataMsg.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        // Task get data message OK.
        if (taskDataGetMsg.taskIndex == 0xFFFF)
        {
            // Send all task data.
            for (taskIndex = 0; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
            {
                if (breakLoop == GOS_TRUE)
                {
                    break;
                }
                else
                {
                    // Nothing to do, continue.
                }

                (void_t) gos_taskGetDataByIndex(taskIndex, &taskDesc);

                if (taskDesc.taskId != GOS_INVALID_TASK_ID)
                {
                    taskDataMsg.taskData.taskId               = taskDesc.taskId;
                    taskDataMsg.taskData.taskStackSize        = taskDesc.taskStackSize;
                    taskDataMsg.taskData.taskStackMaxUsage    = taskDesc.taskStackSizeMaxUsage;
                    taskDataMsg.taskData.taskCpuUsageLimit    = taskDesc.taskCpuUsageLimit;
                    taskDataMsg.taskData.taskCpuUsage         = taskDesc.taskCpuUsage;
                    taskDataMsg.taskData.taskCpuUsageMax      = taskDesc.taskCpuUsageMax;
                    taskDataMsg.taskData.taskOriginalPriority = taskDesc.taskOriginalPriority;
                    taskDataMsg.taskData.taskPriority         = taskDesc.taskPriority;
                    taskDataMsg.taskData.taskCsCounter        = taskDesc.taskCsCounter;
                    taskDataMsg.taskData.taskPrivilegeLevel   = taskDesc.taskPrivilegeLevel;
                    taskDataMsg.taskData.taskState            = taskDesc.taskState;

                    (void_t) memcpy((void_t*)&taskDataMsg.taskData.taskRunTime, (void_t*)&taskDesc.taskRunTime, sizeof(taskDesc.taskRunTime));
                    (void_t) strcpy(taskDataMsg.taskData.taskName, taskDesc.taskName);

                    taskDataMsg.messageResult = GOS_SYSMON_MSG_RES_OK;
                }
                else
                {
                    // Last task found.
                    taskDataMsg.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                    breakLoop = GOS_TRUE;
                }

                gos_sysmonSendResponse(lutIndex + 1);
            }
        }
        else
        {
            // Send specific task data.
            (void_t) gos_taskGetDataByIndex(taskDataGetMsg.taskIndex, &taskDesc);

            taskDataMsg.taskData.taskId               = taskDesc.taskId;
            taskDataMsg.taskData.taskStackSize        = taskDesc.taskStackSize;
            taskDataMsg.taskData.taskStackMaxUsage    = taskDesc.taskStackSizeMaxUsage;
            taskDataMsg.taskData.taskCpuUsageLimit    = taskDesc.taskCpuUsageLimit;
            taskDataMsg.taskData.taskCpuUsage         = taskDesc.taskCpuUsage;
            taskDataMsg.taskData.taskCpuUsageMax      = taskDesc.taskCpuUsageMax;
            taskDataMsg.taskData.taskOriginalPriority = taskDesc.taskOriginalPriority;
            taskDataMsg.taskData.taskPriority         = taskDesc.taskPriority;
            taskDataMsg.taskData.taskCsCounter        = taskDesc.taskCsCounter;
            taskDataMsg.taskData.taskPrivilegeLevel   = taskDesc.taskPrivilegeLevel;
            taskDataMsg.taskData.taskState            = taskDesc.taskState;

            (void_t) memcpy((void_t*)&taskDataMsg.taskData.taskRunTime, (void_t*)&taskDesc.taskRunTime, sizeof(taskDesc.taskRunTime));
            (void_t) strcpy(taskDataMsg.taskData.taskName, taskDesc.taskName);

            taskDataMsg.messageResult = GOS_SYSMON_MSG_RES_OK;
            gos_sysmonSendResponse(lutIndex + 1);
        }
    }
    else
    {
        // Message error.
    }
}

/**
 * @brief   Handles the task variable data get request.
 * @details Sends out the variable task data related to the request task.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleTaskVariableDataGet (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Local variables.
     */
    u16_t  taskIndex = 0u;
    bool_t breakLoop = GOS_FALSE;

    /*
     * Function code.
     */
    taskVariableDataMsg.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (taskVariableDataMsg.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        // Task get data message OK.
        if (taskDataGetMsg.taskIndex == 0xFFFF)
        {
            // Send all task data.
            for (taskIndex = 0; taskIndex < CFG_TASK_MAX_NUMBER; taskIndex++)
            {
                if (breakLoop == GOS_TRUE)
                {
                    break;
                }
                else
                {
                    // Nothing to do, continue.
                }

                (void_t) gos_taskGetDataByIndex(taskIndex, &taskDesc);

                if (taskDesc.taskId != GOS_INVALID_TASK_ID)
                {
                    taskVariableDataMsg.taskVariableData.taskStackMaxUsage = taskDesc.taskStackSizeMaxUsage;
                    taskVariableDataMsg.taskVariableData.taskCpuUsage      = taskDesc.taskCpuUsage;
                    taskVariableDataMsg.taskVariableData.taskCpuUsageMax   = taskDesc.taskCpuUsageMax;
                    taskVariableDataMsg.taskVariableData.taskPriority      = taskDesc.taskPriority;
                    taskVariableDataMsg.taskVariableData.taskCsCounter     = taskDesc.taskCsCounter;
                    taskVariableDataMsg.taskVariableData.taskState         = taskDesc.taskState;

                    (void_t) memcpy((void_t*)&taskVariableDataMsg.taskVariableData.taskRunTime, (void_t*)&taskDesc.taskRunTime, sizeof(taskDesc.taskRunTime));

                    taskVariableDataMsg.messageResult = GOS_SYSMON_MSG_RES_OK;

                }
                else
                {
                    // Last task found.
                    taskVariableDataMsg.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                    breakLoop = GOS_TRUE;
                }

                gos_sysmonSendResponse(lutIndex + 1);
            }
        }
        else
        {
            // Send specific task data.
            (void_t) gos_taskGetDataByIndex(taskDataGetMsg.taskIndex, &taskDesc);

            taskVariableDataMsg.taskVariableData.taskStackMaxUsage = taskDesc.taskStackSizeMaxUsage;
            taskVariableDataMsg.taskVariableData.taskCpuUsage      = taskDesc.taskCpuUsage;
            taskVariableDataMsg.taskVariableData.taskCpuUsageMax   = taskDesc.taskCpuUsageMax;
            taskVariableDataMsg.taskVariableData.taskPriority      = taskDesc.taskPriority;
            taskVariableDataMsg.taskVariableData.taskCsCounter     = taskDesc.taskCsCounter;
            taskVariableDataMsg.taskVariableData.taskState         = taskDesc.taskState;

            (void_t) memcpy((void_t*)&taskVariableDataMsg.taskVariableData.taskRunTime, (void_t*)&taskDesc.taskRunTime, sizeof(taskDesc.taskRunTime));

            taskVariableDataMsg.messageResult = GOS_SYSMON_MSG_RES_OK;
            gos_sysmonSendResponse(lutIndex + 1);
        }
    }
    else
    {
        // Message error.
    }
}

/**
 * @brief   Handles the task modification request.
 * @details Performs the requested kind of modification on the requested task.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleTaskModification (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    taskModifyResultMessage.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (taskModifyResultMessage.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        // Send specific task data.
        (void_t) gos_taskGetDataByIndex(taskModifyMessage.taskIndex, &taskDesc);

         // Perform request based on modification type.
         switch (taskModifyMessage.modificationType)
         {
             case GOS_SYSMON_TASK_MOD_TYPE_SUSPEND:
             {
                 if (gos_taskSuspend(taskDesc.taskId) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             case GOS_SYSMON_TASK_MOD_TYPE_RESUME:
             {
                 if (gos_taskResume(taskDesc.taskId) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             case GOS_SYSMON_TASK_MOD_TYPE_DELETE:
             {
                 if (gos_taskDelete(taskDesc.taskId) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             case GOS_SYSMON_TASK_MOD_TYPE_BLOCK:
             {
                 if (gos_taskBlock(taskDesc.taskId, taskModifyMessage.param) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             case GOS_SYSMON_TASK_MOD_TYPE_UNBLOCK:
             {
                 if (gos_taskUnblock(taskDesc.taskId) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             case GOS_SYSMON_TASK_MOD_TYPE_WAKEUP:
             {
                 if (gos_taskWakeup(taskDesc.taskId) == GOS_SUCCESS)
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
                 }
                 else
                 {
                     taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
                 }
                 break;
             }
             default:
             {
                 taskModifyResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
             }
         }
    }
    else
    {
        // Message error.
    }

    gos_sysmonSendResponse(lutIndex + 1);
}

/**
 * @brief   Handles the system runtime get request.
 * @details Sends out the total system runtime since startup.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleSysRuntimeGet (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    sysRuntimeGetResultMessage.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (sysRuntimeGetResultMessage.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        if (gos_runTimeGet(&sysRuntimeGetResultMessage.sysRuntime) == GOS_SUCCESS)
        {
            sysRuntimeGetResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
        }
        else
        {
            sysRuntimeGetResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
        }
    }
    else
    {
        // Message error.
    }

    gos_sysmonSendResponse(lutIndex + 1);
}

/**
 * @brief   Handles the system time set request.
 * @details Sets the system time to the given value.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleSystimeSet (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    sysTimeSetResultMessage.messageResult = gos_sysmonCheckMessage(lutIndex);

    if (sysTimeSetResultMessage.messageResult == GOS_SYSMON_MSG_RES_OK)
    {
        if (gos_timeSet(&sysTimeSetMessage.desiredSystemTime) == GOS_SUCCESS)
        {
            sysTimeSetResultMessage.messageResult = GOS_SYSMON_MSG_RES_OK;
        }
        else
        {
            sysTimeSetResultMessage.messageResult = GOS_SYSMON_MSG_RES_ERROR;
        }
    }
    else
    {
        // Message error.
    }

    gos_sysmonSendResponse(lutIndex + 1);
}

/**
 * @brief   Handles the system reset request.
 * @details Resets the system.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  -
 */
GOS_STATIC void_t gos_sysmonHandleResetRequest (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Function code.
     */
    if (gos_sysmonCheckMessage(lutIndex) == GOS_SYSMON_MSG_RES_OK)
    {
        gos_kernelReset();
    }
    else
    {
        // Nothing to do.
    }
}

/**
 * @brief   Checks the high-level message parameters.
 * @details Checks the protocol version and the payload CRC value.
 *
 * @param   lutIndex : Look-up table index of the message.
 *
 * @return  Result of message checking.
 *
 * @retval  GOS_SYSMON_MSG_RES_OK          : Message OK.
 * @retval  GOS_SYSMON_MSG_INV_PV          : Invalid protocol version.
 * @retval  GOS_SYSMON_MSG_INV_PAYLOAD_CRC : Payload CRC mismatch.
 */
GOS_STATIC gos_sysmonMessageResult_t gos_sysmonCheckMessage (gos_sysmonMessageEnum_t lutIndex)
{
    /*
     * Local variables.
     */
    gos_sysmonMessageResult_t result = GOS_SYSMON_MSG_RES_OK;

    /*
     * Function code.
     */
    // Check if there is a payload to be copied.
    if (sysmonLut[lutIndex].pMessagePayload != NULL)
    {
        (void_t) memcpy(sysmonLut[lutIndex].pMessagePayload, (void_t*)receiveBuffer, sysmonLut[lutIndex].payloadSize);
    }
    else
    {
        // Nothing to do.
    }

    return result;
}
