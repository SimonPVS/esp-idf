// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once 

#include <stdbool.h>
#include "hal/interrupt_controller_types.h"
#include "hal/interrupt_controller_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gets target platform interrupt descriptor table
 *
 * @return Address of interrupt descriptor table
 */
__attribute__((pure))  const int_desc_t *interrupt_controller_hal_desc_table(void);

/**
 * @brief Gets the interrupt type given an interrupt number.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @return interrupt type
 */
__attribute__((pure))  int_type_t interrupt_controller_hal_desc_type(int interrupt_number);

/**
 * @brief Gets the interrupt level given an interrupt number.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @return interrupt level bitmask
 */
__attribute__((pure))  int interrupt_controller_hal_desc_level(int interrupt_number);

/**
 * @brief Gets the cpu flags given the interrupt number and target cpu.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @param cpu_number CPU number between 0 and SOC_CPU_CORES_NUM - 1
 * @return flags for that interrupt number 
 */
__attribute__((pure))  uint32_t interrupt_controller_hal_desc_flags(int interrupt_number, int cpu_number);

/**
 * @brief Gets the interrupt type given an interrupt number.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @return interrupt type
 */
static inline int_type_t interrupt_controller_hal_get_type(int interrupt_number)
{
    return interrupt_controller_hal_desc_type(interrupt_number);
}    

/**
 * @brief Gets the interrupt level given an interrupt number.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @return interrupt level bitmask
 */
static inline int interrupt_controller_hal_get_level(int interrupt_number)
{
    return interrupt_controller_hal_desc_level(interrupt_number);
}

/**
 * @brief Gets the cpu flags given the interrupt number and target cpu.
 * 
 * @param interrupt_number Interrupt number 0 to 31
 * @param cpu_number CPU number between 0 and SOC_CPU_CORES_NUM - 1
 * @return flags for that interrupt number 
 */
static inline uint32_t interrupt_controller_hal_get_cpu_desc_flags(int interrupt_number, int cpu_number)
{
    return interrupt_controller_hal_desc_flags(interrupt_number, cpu_number);
}

/**
 * @brief enable interrupts specified by the mask
 * 
 * @param mask bitmask of interrupts that needs to be enabled 
 */
static inline void interrupt_controller_hal_enable_interrupts(uint32_t mask) 
{
    intr_cntrl_ll_enable_interrupts(mask);
}

/**
 * @brief disable interrupts specified by the mask
 * 
 * @param mask bitmask of interrupts that needs to be disabled 
 */
static inline void interrupt_controller_hal_disable_interrupts(uint32_t mask)
{
    intr_cntrl_ll_disable_interrupts(mask);
}

/**
 * @brief checks if given interrupt number has a valid handler
 * 
 * @param intr interrupt number ranged from 0 to 31
 * @param cpu cpu number ranged betweeen 0 to SOC_CPU_CORES_NUM - 1
 * @return true for valid handler, false otherwise 
 */
static inline bool interrupt_controller_hal_has_handler(int intr, int cpu)
{
    return intr_cntrl_ll_has_handler(intr, cpu);
}    

/**
 * @brief sets interrupt handler and optional argument of a given interrupt number 
 * 
 * @param intr interrupt number ranged from 0 to 31
 * @param handler handler invoked when an interrupt occurs
 * @param arg optional argument to pass to the handler
 */
static inline void interrupt_controller_hal_set_int_handler(uint8_t intr, interrupt_handler_t handler, void *arg) 
{
    intr_cntrl_ll_set_int_handler(intr, handler, arg);
}

/**
 * @brief Gets argument passed to handler of a given interrupt number 
 * 
 * @param intr interrupt number ranged from 0 to 31
 * 
 * @return argument used by handler of passed interrupt number
 */
static inline void * interrupt_controller_hal_get_int_handler_arg(uint8_t intr) 
{
    return intr_cntrl_ll_get_int_handler_arg(intr);
}

/**
 * @brief Disables interrupts that are not located in iram
 * 
 * @param newmask mask of interrupts needs to be disabled
 * @return oldmask where to store old interrupts state
 */
static inline uint32_t interrupt_controller_hal_disable_int_mask(uint32_t newmask)
{
    return intr_cntrl_ll_disable_int_mask(newmask); 
}

/**
 * @brief Enables interrupts that are not located in iram
 * 
 * @param newmask mask of interrupts needs to be disabled
 */
static inline void interrupt_controller_hal_enable_int_mask(uint32_t newmask) 
{
    intr_cntrl_ll_enable_int_mask(newmask);
}

#ifdef __cplusplus
}
#endif
