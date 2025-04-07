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
//! @file       gos_shell.c
//! @author     Ahmed Gazar
//! @date       2024-06-28
//! @version    1.9
//!
//! @brief      GOS shell service source.
//! @details    For a more detailed description of this service, please refer to @ref gos_shell.h
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2022-11-04    Ahmed Gazar     Initial version created
// 1.1        2022-11-15    Ahmed Gazar     +    License added
// 1.2        2022-12-03    Ahmed Gazar     +    gos_shellRegisterCommands added
//                                          *    Actual command and command parameter buffers
//                                               made modul global instead of task local
// 1.3        2022-12-11    Ahmed Gazar     +    Built-in shell commands (dump and reset) added
// 1.4        2022-12-13    Ahmed Gazar     +    Privilege handling added
// 1.5        2023-01-12    Ahmed Gazar     +    Task delete, suspend, resume, help commands added
// 1.6        2023-05-19    Ahmed Gazar     *    Command buffer size modified
//                                          +    Buffer overflow protection added
// 1.7        2023-07-12    Ahmed Gazar     +    Command handler privilege-handling added
// 1.8        2023-09-08    Ahmed Gazar     +    Shell help: list of shell commands added
//                                          +    Shell CPU and runtime commands added
// 1.9        2024-06-28    Ahmed Gazar     +    Task unblock commands added
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
#include <gos.h>
#include <gos_shell.h>
#include <gos_shell_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Macros
 */
/**
 * Shell daemon poll time [ms].
 */
#define GOS_SHELL_DAEMON_POLL_TIME_MS    ( 50u )

/**
 * Shell display text.
 */
#define GOS_SHELL_DISPLAY_TEXT           ("[\x1B[1m\x1B[33mgos shell\x1B[0m]>> ")

/*
 * Static variables
 */
/**
 * Shell command array.
 */
GOS_STATIC gos_shellCommand_t shellCommands        [CFG_SHELL_MAX_COMMAND_NUMBER];

/**
 * Shell daemon task ID.
 */
GOS_STATIC gos_tid_t          shellDaemonTaskId;

/**
 * Command buffer.
 */
GOS_STATIC char_t             commandBuffer        [CFG_SHELL_COMMAND_BUFFER_SIZE];

/**
 * Command buffer index.
 */
GOS_STATIC u16_t              commandBufferIndex;

/**
 * Shell echo flag.
 */
GOS_STATIC bool_t             useEcho;

/**
 * Actual command buffer.
 */
GOS_STATIC char_t             actualCommand        [CFG_SHELL_MAX_COMMAND_LENGTH];

/**
 * Command parameters buffer.
 */
GOS_STATIC char_t             commandParams        [CFG_SHELL_MAX_PARAMS_LENGTH];

/*
 * Function prototypes
 */
GOS_STATIC void_t gos_shellDaemonTask     (void_t);
GOS_STATIC void_t gos_shellCommandHandler (char_t* params);

/**
 * Shell daemon task descriptor.
 */
GOS_STATIC gos_taskDescriptor_t shellDaemonTaskDesc =
{
    .taskFunction        = gos_shellDaemonTask,
    .taskName            = "gos_shell_daemon",
    .taskPriority        = CFG_TASK_SHELL_DAEMON_PRIO,
    .taskStackSize       = CFG_TASK_SHELL_DAEMON_STACK,
    .taskPrivilegeLevel  = GOS_TASK_PRIVILEGE_KERNEL
};

/**
 * Shell info command.
 */
GOS_STATIC gos_shellCommand_t shellCommand =
{
    .command        = "shell",
    .commandHandler = gos_shellCommandHandler,
    .commandHandlerPrivileges = GOS_TASK_PRIVILEGE_KERNEL
};

/*
 * Function: gos_shellInit
 */
gos_result_t gos_shellInit (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t            shellInitResult = GOS_ERROR;
    gos_shellCommandIndex_t index           = 0u;

    /*
     * Function code.
     */
    useEcho = GOS_TRUE;

    for (index = 0u; index < CFG_SHELL_MAX_COMMAND_NUMBER; index++)
    {
        shellCommands[index].commandHandler = NULL;
    }

    if (gos_taskRegister(&shellDaemonTaskDesc, &shellDaemonTaskId) == GOS_SUCCESS &&
        gos_shellRegisterCommand(&shellCommand) == GOS_SUCCESS)
    {
        shellInitResult = GOS_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return shellInitResult;
}

/*
 * Function: gos_shellRegisterCommands
 */
gos_result_t gos_shellRegisterCommands (gos_shellCommand_t* commands, u16_t arraySize)
{
    /*
     * Local variables.
     */
    gos_result_t registerResult   = GOS_ERROR;
    u16_t        numberOfCommands = 0u;
    u16_t        index            = 0u;

    /*
     * Function code.
     */
    if (commands != NULL)
    {
        numberOfCommands = arraySize / sizeof(commands[0]);

        for (index = 0u; index < numberOfCommands; index++)
        {
            if (gos_shellRegisterCommand(&commands[index]) != GOS_SUCCESS)
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if (index == numberOfCommands)
        {
            registerResult = GOS_SUCCESS;
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

    return registerResult;
}

/*
 * Function: gos_shellRegisterCommand
 */
gos_result_t gos_shellRegisterCommand (gos_shellCommand_t* command)
{
    /*
     * Local variables.
     */
    gos_result_t            shellRegisterCommandResult = GOS_ERROR;
    gos_shellCommandIndex_t index                      = 0u;

    /*
     * Function code.
     */
    if (command->commandHandler != NULL && command->command != NULL)
    {
        for (index = 0u; index < CFG_SHELL_MAX_COMMAND_NUMBER; index++)
        {
            if (shellCommands[index].commandHandler == NULL)
            {
                shellCommands[index].commandHandler = command->commandHandler;
                shellCommands[index].commandHandlerPrivileges = command->commandHandlerPrivileges;
                (void_t) strcpy(shellCommands[index].command, command->command);
                shellRegisterCommandResult = GOS_SUCCESS;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    return shellRegisterCommandResult;
}

/*
 * Function: gos_shellSuspend
 */
gos_result_t gos_shellSuspend (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t shellSuspendResult = GOS_ERROR;

    /*
     * Function code.
     */
    shellSuspendResult = gos_taskSuspend(shellDaemonTaskId);

    return shellSuspendResult;
}

/*
 * Function: gos_shellResume
 */
gos_result_t gos_shellResume (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t shellResumeResult = GOS_ERROR;

    /*
     * Function code.
     */
    shellResumeResult = gos_taskResume(shellDaemonTaskId);

    return shellResumeResult;
}

/*
 * Function: gos_shellEchoOn
 */
gos_result_t gos_shellEchoOn (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t shellEchoOnResult = GOS_SUCCESS;

    /*
     * Function code.
     */
    useEcho = GOS_TRUE;

    return shellEchoOnResult;
}

/*
 * Function: gos_shellEchoOff
 */
gos_result_t gos_shellEchoOff (void_t)
{
    /*
     * Local variables.
     */
    gos_result_t shellEchoOffResult = GOS_SUCCESS;

    /*
     * Function code.
     */
    useEcho = GOS_FALSE;

    return shellEchoOffResult;
}

/**
 * @brief   Shell daemon task.
 * @details Receives a character from the log serial line, if the echoing is on, then
 *          sends the same character back. When an enter key is received, it processes
 *          the command typed in, and loops through the command array. When the command
 *          is found, it calls the command handler function with the parameter list as
 *          a string.
 *
 * @return    -
 */
GOS_STATIC void_t gos_shellDaemonTask (void_t)
{
    /*
     * Local variables.
     */
    gos_shellCommandIndex_t index              = 0u;
    u16_t                   actualCommandIndex = 0u;
    u16_t                   paramIndex         = 0u;

    /*
     * Function code.
     */
    (void_t) gos_taskSleep(CFG_SHELL_STARTUP_DELAY_MS);
    (void_t) gos_shellDriverTransmitString(GOS_SHELL_DISPLAY_TEXT);

    for (;;)
    {
        if (gos_shellDriverReceiveChar(&commandBuffer[commandBufferIndex]) == GOS_SUCCESS)
        {
            if (useEcho == GOS_TRUE)
            {
                (void_t) gos_shellDriverTransmitString(&commandBuffer[commandBufferIndex]);
            }
            else
            {
                // Nothing to do.
            }

            /*
             * Backspace character check
             */
            if (commandBuffer[commandBufferIndex] == '\177')
            {
                commandBuffer[commandBufferIndex] = '\0';
                commandBufferIndex--;
            }
            /*
             * Enter character check
             */
            else if (commandBuffer[commandBufferIndex] == '\r')
            {
                if (useEcho == GOS_TRUE)
                {
                    (void_t) gos_shellDriverTransmitString("\n");
                }
                else
                {
                    // Nothing to do.
                }

                commandBuffer[commandBufferIndex] = '\0';
                actualCommandIndex = 0u;
                paramIndex = 0u;

                // Get command.
                while (commandBuffer[actualCommandIndex] != ' ' && commandBuffer[actualCommandIndex] != '\0')
                {
                    actualCommand[actualCommandIndex] = commandBuffer[actualCommandIndex];
                    actualCommandIndex++;
                }
                actualCommand[actualCommandIndex] = '\0';
                actualCommandIndex++;

                // Get parameters.
                while (commandBuffer[actualCommandIndex] != '\0')
                {
                    commandParams[paramIndex++] = commandBuffer[actualCommandIndex++];
                }
                commandParams[paramIndex] = '\0';

                for (index = 0u; index < CFG_SHELL_MAX_COMMAND_NUMBER; index++)
                {
                    if (strcmp(shellCommands[index].command, actualCommand) == 0)
                    {
                        if (shellCommands[index].commandHandler != NULL)
                        {
                            (void_t) gos_taskSetPrivileges(shellDaemonTaskId, shellCommands[index].commandHandlerPrivileges);
                            shellCommands[index].commandHandler(commandParams);
                            (void_t) gos_taskSetPrivileges(shellDaemonTaskId, GOS_TASK_PRIVILEGE_KERNEL);
                        }
                        else
                        {
                            // Nothing to do.
                        }
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // If command not found.
                if (index == CFG_SHELL_MAX_COMMAND_NUMBER)
                {
                    (void_t) gos_shellDriverTransmitString("Unrecognized command!\r\n");
                }
                else
                {
                    // Nothing to do.
                }

                (void_t) memset((void_t*)commandBuffer, '\0', CFG_SHELL_COMMAND_BUFFER_SIZE);
                commandBufferIndex = 0u;

                (void_t) gos_shellDriverTransmitString(GOS_SHELL_DISPLAY_TEXT);
            }
            else
            {
                commandBufferIndex++;

                if (commandBufferIndex >= CFG_SHELL_COMMAND_BUFFER_SIZE)
                {
                    (void_t) memset((void_t*)commandBuffer, '\0', CFG_SHELL_COMMAND_BUFFER_SIZE);
                    commandBufferIndex = 0u;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        (void_t) gos_taskSleep(GOS_SHELL_DAEMON_POLL_TIME_MS);
    }
}

/**
 * @brief   Shell command handler.
 * @details Handles the built-in shell command.
 *
 * @return    -
 */
GOS_STATIC void_t gos_shellCommandHandler (char_t* params)
{
    /*
     * Local variables.
     */
    u8_t                    index        = 0u;
    gos_shellCommandIndex_t commandIndex = 0u;
    gos_tid_t               taskId       = GOS_INVALID_TASK_ID;
    gos_runtime_t           totalRunTime = {0};

    /*
     * Function code.
     */
    if (strcmp(params, "dump") == 0)
    {
        gos_Dump();
    }
    else if (strcmp(params, "reset") == 0)
    {
        gos_kernelReset();
    }
    else if (strcmp(params, "help") == 0)
    {
        (void_t) gos_shellDriverTransmitString("List of registered shell commands: \r\n");
        for (commandIndex = 0u; commandIndex < CFG_SHELL_MAX_COMMAND_NUMBER; commandIndex++)
        {
            if (strcmp(shellCommands[commandIndex].command, "") == 0)
            {
                break;
            }
            else if (strcmp(shellCommands[commandIndex].command, "shell") == 0)
            {
                (void_t) gos_shellDriverTransmitString("\t"
                        "- shell\r\n\t\t"
                        "- dump\r\n\t\t"
                        "- reset\r\n\t\t"
                        "- delete_tid\r\n\t\t"
                        "- suspend_tid\r\n\t\t"
                        "- resume_tid\r\n\t\t"
                		"- unblock_tid\r\n\t\t"
                        "- delete\r\n\t\t"
                        "- suspend\r\n\t\t"
                        "- resume\r\n\t\t"
                		"- unblock\r\n\t\t"
                        "- runtime\r\n\t\t"
                        "- cpu\r\n");
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("\t- %s\r\n", shellCommands[commandIndex].command);
            }
        }
    }
    else
    {
        while (params[index] != ' ' && params[index])
        {
            index++;
        }

        params[index] = '\0';

        if (strcmp(params, "delete_tid") == 0)
        {
            taskId = (gos_tid_t)strtol(&params[++index], NULL, 16);

            if (gos_taskDelete(taskId) == GOS_SUCCESS)
            {
                (void_t) gos_shellDriverTransmitString("0x%X task has been deleted.\r\n", taskId);
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("0x%X task could not be deleted.\r\n", taskId);
            }
        }
        else if (strcmp(params, "delete") == 0)
        {
            if (gos_taskGetId(&params[++index], &taskId) == GOS_SUCCESS)
            {
                if (gos_taskDelete(taskId) == GOS_SUCCESS)
                {
                    (void_t) gos_shellDriverTransmitString("%s has been deleted.\r\n", &params[index]);
                }
                else
                {
                    (void_t) gos_shellDriverTransmitString("%s could not be deleted.\r\n", &params[index]);
                }
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("Task could not be found.\r\n");
            }
        }
        else if (strcmp(params, "suspend_tid") == 0)
        {
            taskId = (gos_tid_t)strtol(&params[++index], NULL, 16);

            if (gos_taskSuspend(taskId) == GOS_SUCCESS)
            {
                (void_t) gos_shellDriverTransmitString("0x%X task has been suspended.\r\n", taskId);
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("0x%X task could not be suspended.\r\n", taskId);
            }
        }
        else if (strcmp(params, "suspend") == 0)
        {
            if (gos_taskGetId(&params[++index], &taskId) == GOS_SUCCESS)
            {
                if (gos_taskSuspend(taskId) == GOS_SUCCESS)
                {
                    (void_t) gos_shellDriverTransmitString("%s has been suspended.\r\n", &params[index]);
                }
                else
                {
                    (void_t) gos_shellDriverTransmitString("%s could not be suspended.\r\n", &params[index]);
                }
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("Task could not be found.\r\n");
            }
        }
        else if (strcmp(params, "resume_tid") == 0)
        {
            taskId = (gos_tid_t)strtol(&params[++index], NULL, 16);

            if (gos_taskResume(taskId) == GOS_SUCCESS)
            {
                (void_t) gos_shellDriverTransmitString("0x%X task has been resumed.\r\n", taskId);
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("0x%X task could not be resumed.\r\n", taskId);
            }
        }
        else if (strcmp(params, "resume") == 0)
        {
            if (gos_taskGetId(&params[++index], &taskId) == GOS_SUCCESS)
            {
                if (gos_taskResume(taskId) == GOS_SUCCESS)
                {
                    (void_t) gos_shellDriverTransmitString("%s has been resumed.\r\n", &params[index]);
                }
                else
                {
                    (void_t) gos_shellDriverTransmitString("%s could not be resumed.\r\n", &params[index]);
                }
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("Task could not be found.\r\n");
            }
        }
        else if (strcmp(params, "unblock_tid") == 0)
        {
            taskId = (gos_tid_t)strtol(&params[++index], NULL, 16);

            if (gos_taskUnblock(taskId) == GOS_SUCCESS)
            {
                (void_t) gos_shellDriverTransmitString("0x%X task has been unblocked.\r\n", taskId);
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("0x%X task could not be unblocked.\r\n", taskId);
            }
        }
        else if (strcmp(params, "unblock") == 0)
        {
            if (gos_taskGetId(&params[++index], &taskId) == GOS_SUCCESS)
            {
                if (gos_taskUnblock(taskId) == GOS_SUCCESS)
                {
                    (void_t) gos_shellDriverTransmitString("%s has been unblocked.\r\n", &params[index]);
                }
                else
                {
                    (void_t) gos_shellDriverTransmitString("%s could not be unblocked.\r\n", &params[index]);
                }
            }
            else
            {
                (void_t) gos_shellDriverTransmitString("Task could not be found.\r\n");
            }
        }
        else if (strcmp(params, "cpu") == 0)
        {
            (void_t) gos_shellDriverTransmitString("CPU usage: %u.%02u%%\r\n", (gos_kernelGetCpuUsage() / 100), (gos_kernelGetCpuUsage() % 100));
        }
        else if (strcmp(params, "runtime") == 0)
        {
            (void_t) gos_runTimeGet(&totalRunTime);

            (void_t) gos_shellDriverTransmitString(
                    "System runtime: %d days %d hours %d minutes %d seconds\r\n",
                    totalRunTime.days,
                    totalRunTime.hours,
                    totalRunTime.minutes,
                    totalRunTime.seconds
                    );
        }
        else
        {
            // Nothing to do.
        }
    }
}
