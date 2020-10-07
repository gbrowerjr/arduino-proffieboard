/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include "Arduino.h"
#include "stm32l4_wiring_private.h"
#include "stm32l4_iap.h"
#include "dosfs_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t __etextbkp;

const __attribute__((section(".iap_prefix"))) stm32l4_iap_prefix_t stm32l4_iap_prefix = {
    .bcdDevice = USB_DID,
    .idProduct = USB_PID,
    .idVendor  = USB_VID,
};

const __attribute__((section(".iap_info"))) stm32l4_iap_info_t stm32l4_iap_info = {
#if defined(STM32L432xx)
    .signature = "STM32L432xx",
#endif
#if defined(STM32L433xx)
    .signature = "STM32L433xx",
#endif
#if defined(STM32L476xx)
    .signature = "STM32L476xx",
#endif
#if defined(STM32L496xx)
    .signature = "STM32L496xx",
#endif
    .length = sizeof(stm32l4_iap_info_t),
    .features = 0,
    .address = 0x08000800,
    .size = (uint32_t)((uint8_t*)&__etextbkp - 0x08000800)
};

stm32l4_adc_t stm32l4_adc;
stm32l4_exti_t stm32l4_exti;

#if 1
void __attribute__((naked)) HardFault_Handler (void)
{
  asm volatile(
    " tst lr,#4       \n"
    " ite eq          \n"
    " mrseq r0,msp    \n"
    " mrsne r0,psp    \n"
    " mov r1,lr       \n"
    " ldr r2,=HardFault_Handler_C\n"
    " bx r2"
    : /* Outputs */
    : /* Inputs */
    : /* Clobbers */
    );
}
#else
void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");
void HardFault_Handler (void) {
   asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
   stack_pointer = HARDFAULT_PSP;
    while (1)
    {
#if defined(USBCON)
	USBD_Poll();
#endif
    }
}
#endif
  
void __attribute__((used)) HardFault_Handler_C(uint32_t *frame, uint32_t lr)
{
  volatile uint32_t pc  __attribute__((unused)) = frame[6];
    while (1)
    {
#if defined(USBCON)
	USBD_Poll();
#endif
    }
}
  
void BusFault_Handler(void)
{
    while (1)
    {
#if defined(USBCON)
	USBD_Poll();
#endif
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
#if defined(USBCON)
	USBD_Poll();
#endif
    }
}

void init( void )
{
    stm32l4_system_initialize(_SYSTEM_CORE_CLOCK_, _SYSTEM_CORE_CLOCK_/2, _SYSTEM_CORE_CLOCK_/2, STM32L4_CONFIG_LSECLK, STM32L4_CONFIG_HSECLK, STM32L4_CONFIG_SYSOPT);

    armv7m_svcall_initialize();
    armv7m_pendsv_initialize();
    armv7m_systick_initialize(STM32L4_SYSTICK_IRQ_PRIORITY);
    armv7m_timer_initialize();

    stm32l4_rtc_configure(STM32L4_RTC_IRQ_PRIORITY);

    stm32l4_exti_create(&stm32l4_exti, STM32L4_EXTI_IRQ_PRIORITY);
    stm32l4_exti_enable(&stm32l4_exti);

#if (DOSFS_SDCARD == 1)

#ifdef PIN_SPI_SD_POWER
    // TODO: Power SD card on/off as needed.
    stm32l4_gpio_pin_configure(PIN_SPI_SD_POWER, (GPIO_PUPD_NONE | GPIO_OSPEED_HIGH | GPIO_OTYPE_PUSHPULL | GPIO_MODE_OUTPUT));
    stm32l4_gpio_pin_write(PIN_SPI_SD_POWER, 0);
#endif

#if defined(STM32L476xx) || defined(STM32L496xx)
    extern const stm32l4_spi_pins_t g_SPI3Pins;
#else    
    extern const stm32l4_spi_pins_t g_SPIPins;
#endif    

    stm32l4_sdspi_initialize_with_pins(
#if defined(STM32L476xx) || defined(STM32L496xx)
      SPI_INSTANCE_SPI3,
      g_SPI3Pins.mosi,
      g_SPI3Pins.miso,
      g_SPI3Pins.sck,
#ifdef PIN_SPI_SD_ENABLE    
      PIN_SPI_SD_ENABLE
#else    
      GPIO_PIN_PD2
#endif // PIN_SPI_SD_ENABLE    
#else
      SPI_INSTANCE_SPI1,
      g_SPIPins.mosi,
      g_SPIPins.miso,
      g_SPIPins.sck,
#ifdef PIN_SPI_SD_ENABLE    
      PIN_SPI_SD_ENABLE
#else    
      GPIO_PIN_PA8
#endif // PIN_SPI_SD_ENABLE    
#endif
    );
    
#elif (DOSFS_SDCARD == 2)
    stm32l4_sdmmc_initialize(0);
#elif (DOSFS_SDCARD == 3)
    stm32l4_sdmmc_initialize(STM32L4_SDMMC_OPTION_HIGH_SPEED);
#endif
#if (DOSFS_SFLASH >= 1)
    dosfs_sflash_init();
#endif

    /* This is here to work around a linker issue in avr/fdevopen.c */
    asm(".global stm32l4_stdio_put");
    asm(".global stm32l4_stdio_get");
}

#ifdef __cplusplus
}
#endif
