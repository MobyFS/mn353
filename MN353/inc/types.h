/*============================================================================================
 *
 *  (C) 2013, LDM-SYSTEMS
 *
 *  ���������������� ������ ��� ���������� ����� LDM-K1986BE92QI
 *
 *  ������ �� ��������������� "��� ����", �.�. ������������� ��� ������, ���������� ���������
 *  ������������� ���������� �� ���������� ��� ����������� Milandr K1986BE92QI. �������� LDM-SYSTEMS
 *  �� ����� ������� ��������������� �� ��������� ����������� ������������� �������, ���
 *  �������������� ������������� �� ��� ������, ��.
 *
 *--------------------------------------------------------------------------------------------
 *
 *  ���� type.h: ������ ������� ����� � �������
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


// ������� �����������
#define RST_T_U_ABORT      0x01   // ����������: undifined abort
#define RST_T_D_ABORT      0x02   // ����������: data abort
#define RST_T_P_ABORT      0x03   // ����������: prefetch abort
#define RST_T_SWI          0x04   // SWI
#define RST_T_IRQ          0x05   // ���������� ���������� �� ���������
#define RST_T_TEST         0x06   // �������� ����������
#define RST_T_DISP         0x07   // ���������
#define RST_T_SYS          0x08   // ������ ������� (��������� ������������)
#define RST_T_RAND         0x09   // ������ ���
#define RST_T_TIME         0x0A   // ������
#define RST_T_HOST         0x0B   // ����� � ����������� ��������
#define RST_T_PSSV         0x0C   // ���������� �������� ����������
#define RST_T_SOFT         0x0D   // ����������� ������



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
 * ����� ����� types.h
 *=============================================================================*/

