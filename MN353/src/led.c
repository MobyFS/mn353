#include <MDR32Fx.h>
#include "MDR32F9Qx_port.h"
#include <FreeRTOS.h>
#include <task.h>


/* Смещения в регистре и маски для конкретных светодиодов */
typedef enum {
    LED0_OFS    = 0,
    LED1_OFS    = 1,
    LED2_OFS    = 2,
    LED3_OFS    = 3,
    //LED4_OFS    = 14,
    LED0_MASK   = (1 << LED0_OFS),
    LED1_MASK   = (1 << LED1_OFS),
    LED2_MASK   = (1 << LED2_OFS),
    LED3_MASK   = (1 << LED3_OFS),
    //LED4_MASK   = (1 << LED4_OFS),
    LEDS_MASK   = (LED0_MASK | LED1_MASK | LED2_MASK | LED3_MASK)// | LED4_MASK)
} LEDS_Masks;

/* ROL */
#define __SHLC(val, cnt) ((val << (cnt & 31)) | (val >> ((32 - cnt) & 31)))

uint32_t CurrentLights;

void InitPortLED(void) {
    MDR_PORTB->FUNC &= ~((0xFF << (LED0_OFS << 1)));   /* Port */
    MDR_PORTB->ANALOG |= LEDS_MASK;                    /* Digital */
    MDR_PORTB->PWR |= (0xAA << (LED0_OFS << 1));       /* Slow */
    MDR_PORTB->RXTX &= ~LEDS_MASK;
    MDR_PORTB->OE |= LEDS_MASK;
}

void ShiftLights(void) {
    uint32_t ovf;
    MDR_PORTB->RXTX = (MDR_PORTB->RXTX & ~LEDS_MASK) | (CurrentLights & LEDS_MASK);
    ovf = (CurrentLights & (1UL << 31)) != 0;
    CurrentLights <<= 1;
    CurrentLights |= ovf;
}


void LightsOn(void *stub) 
{
  InitPortLED();
  /* Запускаем "спецэффект" */
  //CurrentLights = __SHLC(0xFDF05380, LED0_OFS);
  CurrentLights = __SHLC(0x01010101, LED0_OFS);
  for (;;) {
      vTaskDelay(200);
      ShiftLights();
  }
}



