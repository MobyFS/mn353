#ifndef __MN353_H
#define __MN353_H

// Проект для отладочной платы LDM-R1986BE92QI
#define LDM_K1986BE92QI
// Подключить поддержку LCD
#define LCD_ENABLE

#define CPU_CLK_Value 32000000
#if CPU_CLK_Value % 8000000 != 0
    #error "Invalid CPU_CLK_Value"
#endif

#endif /* __MN353_H */
