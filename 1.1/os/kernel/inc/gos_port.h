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
//! @file       gos_port.h
//! @author     Ahmed Gazar
//! @date       2023-07-12
//! @version    1.0
//!
//! @brief      GOS port header.
//! @details    This header contains the platform-specific ported definitions of the OS.
//*************************************************************************************************
// History
// ------------------------------------------------------------------------------------------------
// Version    Date          Author          Description
// ------------------------------------------------------------------------------------------------
// 1.0        2023-07-12    Ahmed Gazar     Initial version created.
//*************************************************************************************************
#ifndef GOS_PORT_H
#define GOS_PORT_H
/*
 * Includes
 */
#include "gos_kernel.h"
#include "gos_config.h"

/*
 * Macros
 */
/*
 * ARM_CORTEX_M4 definitions
 */
#if (CFG_TARGET_CPU == ARM_CORTEX_M4)

/**
 * Processor reset function.
 */
#define gos_ported_procReset() (                                                                                 \
{                                                                                                                \
    GOS_ASM ("dsb 0xF":::"memory");                                                                              \
    *(u32_t*)(0xE000ED0CUL) = (u32_t)((0x5FAUL << 16U) | (*(u32_t*)(0xE000ED0CUL) & (7UL << 8U)) | (1UL << 2U)); \
    GOS_ASM ("dsb 0xF":::"memory");                                                                              \
}                                                                                                                \
)

/**
 * Reschedule function.
 */
#define gos_ported_reschedule(privilege) (                        \
{                                                                 \
    if (privilege == GOS_PRIVILEGED)                              \
    {                                                             \
        /* Trigger PendSV directly. */                            \
        ICSR |= (1 << 28);                                        \
    }                                                             \
    else                                                          \
    {                                                             \
        /* Call Supervisor exception to get Privileged access. */ \
        GOS_ASM("SVC #255");                                      \
    }                                                             \
}                                                                 \
)

/**
 * Pend SV handler function name.
 */
#define gos_ported_pendSVHandler GOS_NAKED PendSV_Handler

/**
 * Context-switch function.
 */
#define gos_ported_doContextSwitch() (                                  \
{                                                                       \
    /* Save LR back to main, must do this firstly. */                   \
    GOS_ASM("PUSH {LR}");                                               \
                                                                        \
    /* Save the context of current task. */                             \
    /* Get current PSP. */                                              \
    GOS_ASM("MRS R0, PSP");                                             \
    /* Save R4 to R11 to PSP Frame Stack. */                            \
    GOS_ASM("STMDB R0!, {R4-R11}"); /* R0 is updated after decrement */ \
    /* Save current value of PSP. */                                    \
    GOS_ASM("BL gos_kernelSaveCurrentPsp"); /* R0 is first argument */  \
                                                                        \
    /* Do scheduling. */                                                \
    /* Select next task. */                                             \
    GOS_ASM("BL gos_kernelSelectNextTask");                             \
                                                                        \
    /* Retrieve the context of next task. */                            \
    /* Get its past PSP value. */                                       \
    GOS_ASM("BL gos_kernelGetCurrentPsp"); /* return PSP is in R0 */    \
    /* Retrieve R4-R11 from PSP Fram Stack. */                          \
    GOS_ASM("LDMIA R0!, {R4-R11}"); /* R0 is updated after increment */ \
    /* Update PSP. */                                                   \
    GOS_ASM("MSR PSP, R0");                                             \
                                                                        \
    /* Exit. */                                                         \
    GOS_ASM("POP {LR}");                                                \
    GOS_ASM("BX LR");                                                   \
}                                                                       \
)

/**
 * SVC handler function name.
 */
#define gos_ported_svcHandler GOS_NAKED SVC_Handler

/**
 * SVC handler function.
 */
#define gos_ported_handleSVC() (                \
{                                               \
    /* Check LR to know which stack is used. */ \
    GOS_ASM("TST LR, 4");                       \
    /* 2 next instructions are conditional. */  \
    GOS_ASM("ITE EQ");                          \
    /* Save MSP if bit 2 is 0. */               \
    GOS_ASM("MRSEQ R0, MSP");                   \
    /* Save PSP if bit 2 is 1. */               \
    GOS_ASM("MRSNE R0, PSP");                   \
                                                \
    /* Check if reset is requried. */           \
    if (resetRequired == GOS_TRUE)              \
    {                                           \
        resetRequired = GOS_FALSE;              \
        gos_kernelProcessorReset();             \
    }                                           \
                                                \
    /* Pass R0 as the argument. */              \
    GOS_ASM("B gos_kernelSVC_HandlerMain");     \
}                                               \
)

/**
 * SVC handler main function name.
 */
#define gos_ported_svcHandlerMain gos_kernelSVC_HandlerMain

/**
 * SVC handler main function.
 */
#define gos_ported_handleSVCMain(sp) (                     \
{                                                          \
    /* Get the address of the instruction saved in PC. */  \
    u8_t* pInstruction = (u8_t*)(sp[6]);                   \
                                                           \
    /* Go back 2 bytes (16-bit opcode). */                 \
    pInstruction -= 2;                                     \
                                                           \
    /* Get the opcode, in little-endian. */                \
    u8_t svcNum = *pInstruction;                           \
                                                           \
    switch (svcNum)                                        \
    {                                                      \
        case 0xFF:                                         \
            /* Trigger PendSV. */                          \
            ICSR |= (1 << 28);                             \
            break;                                         \
        default: break;                                    \
    }                                                      \
}                                                          \
)

/**
 * System tick handler function name.
 */
#define gos_ported_sysTickInterrupt SysTick_Handler

/**
 * Kernel start initialization function.
 */
#define gos_ported_kernelStartInit() (                            \
{                                                                 \
    /* Prepare PSP of the first task. */                          \
    GOS_ASM("BL gos_kernelGetCurrentPsp"); /* return PSP in R0 */ \
    GOS_ASM("MSR PSP, R0");  /* set PSP */                        \
                                                                  \
    /* Change to use PSP. */                                      \
    GOS_ASM("MRS R0, CONTROL");                                   \
    GOS_ASM("ORR R0, R0, #2"); /* set bit[1] SPSEL */             \
    GOS_ASM("MSR CONTROL, R0");                                   \
                                                                  \
    /* Move to unprivileged level. */                             \
    GOS_ASM("MRS R0, CONTROL");                                   \
    GOS_ASM("ORR R0, R0, #1"); /* Set bit[0] nPRIV */             \
    GOS_ASM("MSR CONTROL, R0");                                   \
    /* Right after here, access is limited. */                    \
}                                                                 \
)

/**
 * Fault handler enable function.
 */
#define gos_ported_enableFaultHandlers() (  \
{                                           \
    SHCSR |= (1 << 16); /* Memory Fault */  \
    SHCSR |= (1 << 17); /* Bus Fault    */  \
    SHCSR |= (1 << 18); /* Usage Fault  */  \
}                                           \
)

#endif
#endif
