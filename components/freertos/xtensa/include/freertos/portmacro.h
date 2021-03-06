/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>	/* required for XSHAL_CLIB */
#include <xtensa/xtruntime.h>
#include "esp_private/crosscore_int.h"
#include "esp_timer.h"              /* required for FreeRTOS run time stats */
#include "soc/spinlock.h"
#include <esp_heap_caps.h>
#include "esp_rom_sys.h"
#include "sdkconfig.h"
#include "freertos/xtensa_api.h"

#ifdef CONFIG_LEGACY_INCLUDE_COMMON_HEADERS
#include "soc/soc_memory_layout.h"
#endif

/*-----------------------------------------------------------
 * Port specific definitions.
 *
 * The settings in this file configure FreeRTOS correctly for the
 * given hardware and compiler.
 *
 * These settings should not be altered.
 *-----------------------------------------------------------
 */

/* Type definitions. */

#define portCHAR		int8_t
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		int32_t
#define portSHORT		int16_t
#define portSTACK_TYPE	uint8_t
#define portBASE_TYPE	int

typedef portSTACK_TYPE			StackType_t;
typedef portBASE_TYPE			BaseType_t;
typedef unsigned portBASE_TYPE	UBaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif
/*-----------------------------------------------------------*/

// portbenchmark
#include "portbenchmark.h"
#include "sdkconfig.h"
#include "esp_attr.h"

static inline uint32_t xPortGetCoreID(void);

// Critical section management. NW-TODO: replace XTOS_SET_INTLEVEL with more efficient version, if any?
// These cannot be nested. They should be used with a lot of care and cannot be called from interrupt level.
//
// Only applies to one CPU. See notes above & below for reasons not to use these.
#define portDISABLE_INTERRUPTS()      do { XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL); portbenchmarkINTERRUPT_DISABLE(); } while (0)
#define portENABLE_INTERRUPTS()       do { portbenchmarkINTERRUPT_RESTORE(0); XTOS_SET_INTLEVEL(0); } while (0)

// Cleaner solution allows nested interrupts disabling and restoring via local registers or stack.
// They can be called from interrupts too.
// WARNING: Only applies to current CPU. See notes above.
static inline unsigned portENTER_CRITICAL_NESTED(void) {
	unsigned state = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
	portbenchmarkINTERRUPT_DISABLE();
	return state;
}
#define portEXIT_CRITICAL_NESTED(state)   do { portbenchmarkINTERRUPT_RESTORE(state); XTOS_RESTORE_JUST_INTLEVEL(state); } while (0)

/*
Modifications to portENTER_CRITICAL.

For an introduction, see "Critical Sections & Disabling Interrupts" in docs/api-guides/freertos-smp.rst

The original portENTER_CRITICAL only disabled the ISRs. This is enough for single-CPU operation: by
disabling the interrupts, there is no task switch so no other tasks can meddle in the data, and because
interrupts are disabled, ISRs can't corrupt data structures either.

For multiprocessing, things get a bit more hairy. First of all, disabling the interrupts doesn't stop
the tasks or ISRs on the other processors meddling with our CPU. For tasks, this is solved by adding
a spinlock to the portENTER_CRITICAL macro. A task running on the other CPU accessing the same data will
spinlock in the portENTER_CRITICAL code until the first CPU is done.

For ISRs, we now also need muxes: while portENTER_CRITICAL disabling interrupts will stop ISRs on the same
CPU from meddling with the data, it does not stop interrupts on the other cores from interfering with the
data. For this, we also use a spinlock in the routines called by the ISR, but these spinlocks
do not disable the interrupts (because they already are).

This all assumes that interrupts are either entirely disabled or enabled. Interrupt priority levels
will break this scheme.

Remark: For the ESP32, portENTER_CRITICAL and portENTER_CRITICAL_ISR both alias vTaskEnterCritical, meaning
that either function can be called both from ISR as well as task context. This is not standard FreeRTOS
behaviour; please keep this in mind if you need any compatibility with other FreeRTOS implementations.
*/

/* "mux" data structure (spinlock) */
typedef struct {
    spinlock_t spinlock;
} portMUX_TYPE;

#define portMUX_FREE_VAL		SPINLOCK_FREE
#define portMUX_NO_TIMEOUT      SPINLOCK_WAIT_FOREVER  /* When passed for 'timeout_cycles', spin forever if necessary */
#define portMUX_TRY_LOCK        SPINLOCK_NO_WAIT       /* Try to acquire the spinlock a single time only */
#define portMUX_INITIALIZER_UNLOCKED  {.spinlock=SPINLOCK_INITIALIZER} 

#define portASSERT_IF_IN_ISR()        vPortAssertIfInISR()
void vPortAssertIfInISR(void);

#define portCRITICAL_NESTING_IN_TCB 0

static inline void __attribute__((always_inline)) vPortCPUInitializeMutex(portMUX_TYPE *mux) 
{
    spinlock_initialize(&mux->spinlock);
}

static inline void __attribute__((always_inline)) vPortCPUAcquireMutex(portMUX_TYPE *mux) 
{
    spinlock_acquire(&mux->spinlock, portMUX_NO_TIMEOUT);
}

static inline bool __attribute__((always_inline)) vPortCPUAcquireMutexTimeout(portMUX_TYPE *mux, int timeout) 
{
    return (spinlock_acquire(&mux->spinlock, timeout));
}

static inline void __attribute__((always_inline)) vPortCPUReleaseMutex(portMUX_TYPE *mux)
{
    spinlock_release(&mux->spinlock);
}

void vPortEnterCritical(portMUX_TYPE *mux);
void vPortExitCritical(portMUX_TYPE *mux);

/*
 * Returns true if the current core is in ISR context; low prio ISR, med prio ISR or timer tick ISR. High prio ISRs
 * aren't detected here, but they normally cannot call C code, so that should not be an issue anyway.
 */
BaseType_t xPortInIsrContext(void);

static inline void __attribute__((always_inline)) vPortEnterCriticalCompliance(portMUX_TYPE *mux)
{
    if(!xPortInIsrContext()) {                                                                  
        vPortEnterCritical(mux);                                                                
    } else {
        esp_rom_printf("%s:%d (%s)- port*_CRITICAL called from ISR context!\n",
                       __FILE__, __LINE__, __FUNCTION__);
        abort();
    }
}

static inline void __attribute__((always_inline)) vPortExitCriticalCompliance(portMUX_TYPE *mux)
{
    if(!xPortInIsrContext()) {                                                                  
        vPortExitCritical(mux);                                                                 
    } else {
        esp_rom_printf("%s:%d (%s)- port*_CRITICAL called from ISR context!\n",
                       __FILE__, __LINE__, __FUNCTION__);
        abort();
    }
}

#ifdef CONFIG_FREERTOS_CHECK_PORT_CRITICAL_COMPLIANCE
/* Calling port*_CRITICAL from ISR context would cause an assert failure.
 * If the parent function is called from both ISR and Non-ISR context then call port*_CRITICAL_SAFE
 */
#define portENTER_CRITICAL(mux)  vPortEnterCriticalCompliance(mux)   
#define portEXIT_CRITICAL(mux)   vPortExitCriticalCompliance(mux)     
#else
#define portENTER_CRITICAL(mux)        vPortEnterCritical(mux)
#define portEXIT_CRITICAL(mux)         vPortExitCritical(mux)
#endif

#define portENTER_CRITICAL_ISR(mux)    vPortEnterCritical(mux)
#define portEXIT_CRITICAL_ISR(mux)     vPortExitCritical(mux)

static inline void __attribute__((always_inline)) vPortEnterCriticalSafe(portMUX_TYPE *mux)
{
    if (xPortInIsrContext()) {                    
        portENTER_CRITICAL_ISR(mux);              
    } else {                                      
        portENTER_CRITICAL(mux);                  
    }                                             
}

static inline void __attribute__((always_inline)) vPortExitCriticalSafe(portMUX_TYPE *mux)
{
    if (xPortInIsrContext()) {                    
        portEXIT_CRITICAL_ISR(mux);               
    } else {                                      
        portEXIT_CRITICAL(mux);                   
    }                                            
}

#define portENTER_CRITICAL_SAFE(mux)  vPortEnterCriticalSafe(mux)
#define portEXIT_CRITICAL_SAFE(mux)  vPortExitCriticalSafe(mux)
/*
 * Wrapper for the Xtensa compare-and-set instruction. This subroutine will atomically compare
 * *addr to 'compare'. If *addr == compare, *addr is set to *set. *set is updated with the previous
 * value of *addr (either 'compare' or some other value.)
 *
 * Warning: From the ISA docs: in some (unspecified) cases, the s32c1i instruction may return the
 * *bitwise inverse* of the old mem if the mem wasn't written. This doesn't seem to happen on the
 * ESP32 (portMUX assertions would fail).
 */
static inline void __attribute__((always_inline)) uxPortCompareSet(volatile uint32_t *addr, uint32_t compare, uint32_t *set) {
    compare_and_set_native(addr, compare, set);
}

// These FreeRTOS versions are similar to the nested versions above
#define portSET_INTERRUPT_MASK_FROM_ISR()            portENTER_CRITICAL_NESTED()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(state)     portEXIT_CRITICAL_NESTED(state)

//Because the ROM routines don't necessarily handle a stack in external RAM correctly, we force
//the stack memory to always be internal.
#define portTcbMemoryCaps (MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)
#define portStackMemoryCaps (MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)

#define pvPortMallocTcbMem(size) heap_caps_malloc(size, portTcbMemoryCaps)
#define pvPortMallocStackMem(size)  heap_caps_malloc(size, portStackMemoryCaps)

/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			4
#define portNOP()					XT_NOP()
/*-----------------------------------------------------------*/

/* Fine resolution time */
#define portGET_RUN_TIME_COUNTER_VALUE()  xthal_get_ccount()
//ccount or esp_timer are initialized elsewhere
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()

#ifdef CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER
/* Coarse resolution time (us) */
#define portALT_GET_RUN_TIME_COUNTER_VALUE(x)    x = (uint32_t)esp_timer_get_time()
#endif

void vPortYield( void );
void vPortEvaluateYieldFromISR(int argc, ...);
void _frxt_setup_switch( void );
/**
 * Macro to count number of arguments of a __VA_ARGS__ used to support portYIELD_FROM_ISR with, 
 * or without arguments.
 */ 
#define portGET_ARGUMENT_COUNT(...) portGET_ARGUMENT_COUNT_INNER(0, ##__VA_ARGS__,1,0)
#define portGET_ARGUMENT_COUNT_INNER(zero, one, count, ...) count

_Static_assert(portGET_ARGUMENT_COUNT() == 0, "portGET_ARGUMENT_COUNT() result does not match for 0 arguments");
_Static_assert(portGET_ARGUMENT_COUNT(1) == 1, "portGET_ARGUMENT_COUNT() result does not match for 1 argument");

#define portYIELD()	vPortYield()

/**
 * @note    The macro below could be used when passing a single argument, or without any argument, 
 *          it was developed to support both usages of portYIELD inside of an ISR. Any other usage form
 *          might result in undesired behaviour
 */
#define portYIELD_FROM_ISR(...) vPortEvaluateYieldFromISR(portGET_ARGUMENT_COUNT(__VA_ARGS__), ##__VA_ARGS__)

/* Yielding within an API call (when interrupts are off), means the yield should be delayed
   until interrupts are re-enabled.

   To do this, we use the "cross-core" interrupt as a trigger to yield on this core when interrupts are re-enabled.This
   is the same interrupt & code path which is used to trigger a yield between CPUs, although in this case the yield is
   happening on the same CPU.
*/
#define portYIELD_WITHIN_API() esp_crosscore_int_send_yield(xPortGetCoreID())

/*-----------------------------------------------------------*/

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

// When coprocessors are defined, we to maintain a pointer to coprocessors area.
// We currently use a hack: redefine field xMPU_SETTINGS in TCB block as a structure that can hold:
// MPU wrappers, coprocessor area pointer, trace code structure, and more if needed.
// The field is normally used for memory protection. FreeRTOS should create another general purpose field.
typedef struct {
	#if XCHAL_CP_NUM > 0
	volatile StackType_t* coproc_area; // Pointer to coprocessor save area; MUST BE FIRST
	#endif

	#if portUSING_MPU_WRAPPERS
	// Define here mpu_settings, which is port dependent
	int mpu_setting; // Just a dummy example here; MPU not ported to Xtensa yet
	#endif

	#if configUSE_TRACE_FACILITY_2
	struct {
		// Cf. porttraceStamp()
		int taskstamp;        /* Stamp from inside task to see where we are */
		int taskstampcount;   /* A counter usually incremented when we restart the task's loop */
	} porttrace;
	#endif
} xMPU_SETTINGS;

// Main hack to use MPU_wrappers even when no MPU is defined (warning: mpu_setting should not be accessed; otherwise move this above xMPU_SETTINGS)
#if (XCHAL_CP_NUM > 0 || configUSE_TRACE_FACILITY_2) && !portUSING_MPU_WRAPPERS   // If MPU wrappers not used, we still need to allocate coproc area
	#undef portUSING_MPU_WRAPPERS
	#define portUSING_MPU_WRAPPERS 1   // Enable it to allocate coproc area
	#define MPU_WRAPPERS_H             // Override mpu_wrapper.h to disable unwanted code
	#define PRIVILEGED_FUNCTION
	#define PRIVILEGED_DATA
#endif

extern void esp_vApplicationIdleHook( void );
extern void esp_vApplicationTickHook( void );

#ifndef CONFIG_FREERTOS_LEGACY_HOOKS
#define vApplicationIdleHook    esp_vApplicationIdleHook
#define vApplicationTickHook    esp_vApplicationTickHook
#endif /* !CONFIG_FREERTOS_LEGACY_HOOKS */

void _xt_coproc_release(volatile void * coproc_sa_base);
void vApplicationSleep( TickType_t xExpectedIdleTime );
void vPortSetStackWatchpoint( void* pxStackStart );

#define portSUPPRESS_TICKS_AND_SLEEP( idleTime ) vApplicationSleep( idleTime )

/*-----------------------------------------------------------*/

/* Architecture specific optimisations. */
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1

/* Check the configuration. */
#if( configMAX_PRIORITIES > 32 )
    #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set to 1 when configMAX_PRIORITIES is less than or equal to 32.  It is very rare that a system requires more than 10 to 15 different priorities as tasks that share a priority will time slice.
#endif

/* Store/clear the ready priorities in a bit map. */
#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )

/*-----------------------------------------------------------*/

#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31 - __builtin_clz( ( uxReadyPriorities ) ) )

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

/*-----------------------------------------------------------*/

// porttrace
#if configUSE_TRACE_FACILITY_2
#include "porttrace.h"
#endif

// configASSERT_2 if requested
#if configASSERT_2
#include <stdio.h>
void exit(int);
#define configASSERT( x )   if (!(x)) { porttracePrint(-1); printf("\nAssertion failed in %s:%d\n", __FILE__, __LINE__); exit(-1); }
#endif

#endif // __ASSEMBLER__

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */

