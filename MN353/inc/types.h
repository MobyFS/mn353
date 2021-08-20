/*============================================================================================
 *
 *  (C) 2013, LDM-SYSTEMS
 *
 *  Демонстрационный проект для отладочной платы LDM-K1986BE92QI
 *
 *  Данное ПО предоставляется "КАК ЕСТЬ", т.е. исключительно как пример, призванный облегчить
 *  пользователям разработку их приложений для процессоров Milandr K1986BE92QI. Компания LDM-SYSTEMS
 *  не несет никакой ответственности за возможные последствия использования данного, или
 *  разработанного пользователем на его основе, ПО.
 *
 *--------------------------------------------------------------------------------------------
 *
 *  Файл type.h: алиасы базовых типов и макросы
 *
 *=============================================================================*/

#ifndef __TYPES_H
#define __TYPES_H

typedef signed long s32;
typedef signed short s16;
typedef signed char s8;

typedef signed long const sc32;
typedef signed short const sc16;
typedef signed char const sc8;

typedef volatile signed long vs32;
typedef volatile signed short vs16;
typedef volatile signed char vs8;

typedef volatile signed long const vsc32;
typedef volatile signed short const vsc16;
typedef volatile signed char const vsc8;

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef unsigned long u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;

typedef unsigned long const uc32;
typedef unsigned short const uc16;
typedef unsigned char const uc8;

typedef volatile unsigned long vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char vu8;

typedef volatile unsigned long const vuc32;
typedef volatile unsigned short const vuc16;
typedef volatile unsigned char const vuc8;

typedef enum {FALSE = 0, TRUE = !FALSE} bool;


// Причины перезапуска
#define RST_T_U_ABORT      0x01   // Исключение: undifined abort
#define RST_T_D_ABORT      0x02   // Исключение: data abort
#define RST_T_P_ABORT      0x03   // Исключение: prefetch abort
#define RST_T_SWI          0x04   // SWI
#define RST_T_IRQ          0x05   // Обработчик прерываний по умолчанию
#define RST_T_TEST         0x06   // Тестовый перезапуск
#define RST_T_DISP         0x07   // Диспетчер
#define RST_T_SYS          0x08   // Ошибка системы (остановка планировщика)
#define RST_T_RAND         0x09   // Ошибка ГСЧ
#define RST_T_TIME         0x0A   // Таймер
#define RST_T_HOST         0x0B   // Связь с управляющим изделием
#define RST_T_PSSV         0x0C   // Супервизор питающих напряжений
#define RST_T_SOFT         0x0D   // Программная ошибка



#define   timer_t    int      //[TMR]  Used for timer ID returned by timer_create().

#define LINK_MAGIC   0xE128746B
#define MSG_MAGIC    0x85937593

//#define DBG_PUT

#ifdef DBG_PUT
#define putdbg(a) putk(a)
#else
#define putdbg(a) 
#endif



#endif /* __TYPES_H */

/*=============================================================================
 * Конец файла types.h
 *=============================================================================*/

