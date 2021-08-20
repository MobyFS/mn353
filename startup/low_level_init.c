/**************************************************
 *
 * This module contains the function `__low_level_init', a function
 * that is called before the `main' function of the program.  Normally
 * low-level initializations - such as setting the prefered interrupt
 * level or setting the watchdog - can be performed here.
 *
 * Note that this function is called before the data segments are
 * initialized, this means that this function cannot rely on the
 * values of global or static variables.
 *
 * When this function returns zero, the startup code will inhibit the
 * initialization of the data segments. The result is faster startup,
 * the drawback is that neither global nor static data will be
 * initialized.
 *
 * Copyright 1999-2004 IAR Systems. All rights reserved.
 *
 * $Revision: 21623 $
 *
 **************************************************/
//#include <1986BE91.h>
#include <MDR32Fx.h>
#include <MDR32F9Qx_rst_clk.h>
#include <mn353.h>

void Frequency_CLK_PLL(unsigned pll_on, unsigned int pll_mul)
{
  
   // Работа от HSE
#if 1  
   // pll_mul - Коэффициент умножения для CPU PLL
   // pll_on - Бит включения PLL
   MDR_RST_CLK->HS_CONTROL = RST_CLK_HSE_ON;       // Включаем внешний резонатор HSE
   while (MDR_RST_CLK->CLOCK_STATUS == 0) __NOP(); // Ожидаем запуска HSE и PLL
   // HCLK=CPU_C3, CPU_C3=CPU_C2, CPU_C2=CPU_C1, CPU_C1=HSE
   MDR_RST_CLK->CPU_CLOCK = 0x102;                 // Включаем CPU_CLK

   if ((pll_on == 1) && (pll_mul)) {
       MDR_RST_CLK->PLL_CONTROL = (pll_on<<2)|((pll_mul-1)<<8); // Вкл PLL
       while (MDR_RST_CLK->CLOCK_STATUS < 0x6) __NOP(); // Ожидаем запуска HSE и PLL
       // HCLK=CPU_C3, CPU_C3=CPU_C2, CPU_C2=PLL, CPU_C1=HSE
       MDR_RST_CLK->CPU_CLOCK = 0x106;                  // Включаем CPU_CLK
   }
   RST_CLK_HSIcmd(DISABLE);
#else   
   // Работа от HSI

   if ((pll_on == 1) && (pll_mul)) {
       MDR_RST_CLK->PLL_CONTROL = (pll_on<<2)|((pll_mul-1)<<8); // Вкл PLL
       while (MDR_RST_CLK->CLOCK_STATUS != 0x2) __NOP(); // Ожидаем запуска PLL
       MDR_RST_CLK->CPU_CLOCK = 0x104;                    // Включаем CPU_CLK
   }
#endif  
}


#ifdef __cplusplus
extern "C" {
#endif
 
#pragma language=extended

__interwork int __low_level_init(void);

__interwork int __low_level_init(void)
{
  /*==================================*/
  /*  Initialize hardware.            */
  /*==================================*/
  
  //Frequency_CLK_PLL(0, 0);           // Вкл. генератор CLK
  Frequency_CLK_PLL(1, CPU_CLK_Value/HSE_Value);
  MDR_RST_CLK->PER_CLOCK = 0x23F001D0;//FFFFFFFF;   // Включаем CLK на порты
  /*
  MDR_RST_CLK->PER_CLOCK = RST_CLK_PCLK_PORTA |
                       RST_CLK_PCLK_PORTB |
                       RST_CLK_PCLK_PORTC |
                       RST_CLK_PCLK_PORTD |
                       RST_CLK_PCLK_PORTE |
                       RST_CLK_PCLK_PORTF;
  */
  
  /*==================================*/
  /* Choose if segment initialization */
  /* should be done or not.           */
  /* Return: 0 to omit seg_init       */
  /*         1 to run seg_init        */
  /*==================================*/
  return 1;
}

#pragma language=default

#ifdef __cplusplus
}
#endif
