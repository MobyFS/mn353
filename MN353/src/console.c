
#include <MDR32Fx.h>
#include <types.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>

#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_port.h"
#include "serial.h"

static u8 dbg_print_mode = 0; // 0x00  -  штатная работа;
static MDR_UART_TypeDef *CON_UART;

static SERIAL_LINE console;

int rxirq_count;

//------------------------------------------------------------------------------
// Обработчик прерывания "приёмник полон"
//------------------------------------------------------------------------------
static inline void irq_rx( u8 byte )
{
  rxirq_count++; // Это чтоб посмотреть протребление в режиме sleep
  
    // Сохранить байт в циклическом буфере, из которого можно считать
    // с помощью функции getch()
    u8 e = (console.rx.e + 1) & (RX_SIZE-1);
    if (e == console.rx.b) 
        return;    // Переполнение входного буфера
    console.rx.buf[console.rx.e] = byte;
    console.rx.e = e;
}

//------------------------------------------------------------------------------
// Обработчик прерывания "передатчик пуст"
//------------------------------------------------------------------------------
static inline int irq_tx()
{
    u8 byte;
    u16 b = console.tx.beg;
    if ( b == console.tx.end ) return -1;
    byte = console.tx.buf[b];
    console.tx.beg = (b + 1)&(TX_SIZE-1);
    return byte;
}

void CON_UART_HandlerWork(void)
{
    u32 SR;
    while(1) {
        SR = CON_UART->MIS;
        
        if (SR == 0) 
            // Всё обработано
            break;
        if ( SR & UART_MIS_RXMIS)
            // Обработать принятый байт
            irq_rx(CON_UART->DR);
        if ( SR & UART_MIS_TXMIS ) {
            int byte;
            byte = irq_tx(); // Есть чего передавать? 
            if ( byte < 0 ) 
                // Нет - запретить прерывания по передаче
                UART_ITConfig(CON_UART,UART_IT_TX,DISABLE);
            else 
                // Да - передать
                CON_UART->DR = (u8)byte;
        }
    }
}
/*
void UART2_IRQHandler(void)
{
  CON_UART_HandlerWork();
}
*/
void UART1_IRQHandler(void)
{
  CON_UART_HandlerWork();
}


//------------------------------------------------------------------------------
// Извлечь байт из буфера клавиатуры.
//------------------------------------------------------------------------------
int getch()
{
    u8 byte;

    if (console.rx.b == console.rx.e) {
	//Dispatch(1);
	return -1;
    }
    byte = console.rx.buf[console.rx.b];
    console.rx.b = (console.rx.b + 1) & (RX_SIZE-1);
    return byte;
}

//------------------------------------------------------------------------------
// Очистить буфер клавиатуры.
//------------------------------------------------------------------------------
void console_clear_con()
{
    __disable_interrupt();
    console.rx.b = 0;
    console.rx.e = 0;
    __enable_interrupt();
}

//------------------------------------------------------------------------------
// Получить текущую частоту CPU_CLK
//------------------------------------------------------------------------------
u32 RST_CLK_GetCpuClock(void)
{
  u32 cpu_c1_freq, cpu_c2_freq, cpu_c3_freq;
  u32 pll_mul;
  u32 temp;


    // Determine CPU_C1 frequency
    cpu_c1_freq = (u32)8000000;
    if (MDR_RST_CLK->CPU_CLOCK & 1) {
      cpu_c1_freq /= 2;
    }

    // Determine CPU_C2 frequency 
    cpu_c2_freq = cpu_c1_freq;
    
    if  (MDR_RST_CLK->CPU_CLOCK & 4) {
      // Determine CPU PLL output frequency
      pll_mul = ((MDR_RST_CLK->PLL_CONTROL >> 8) & (u32)0x0F) + 1;
      cpu_c2_freq *= pll_mul;
    }

    // Select CPU_CLK from HSI, CPU_C3, LSE, LSI cases 
    switch ((MDR_RST_CLK->CPU_CLOCK >> 8) & (u32)0x03) {
    case 0 : // HSI
        temp = (u32)8000000;
        break;
    case 1 : // CPU_C3
        // Determine CPU_C3 frequency 
        if ((MDR_RST_CLK->CPU_CLOCK >> 4 & (u32)0x08) == 0x00) {
            cpu_c3_freq = cpu_c2_freq;
        }
        else {
            cpu_c3_freq = cpu_c2_freq/
                          (1 << ((MDR_RST_CLK->CPU_CLOCK >> 4 & (u32)0x07) + 1));
        }
        temp = cpu_c3_freq;
        break;
    case 2 : // LSE 
        temp = (u32)32768;
        break;
    default : // LSI 
        temp = (u32)40000;
        break;
    }
    return temp;
}

//------------------------------------------------------------------------------
// Инициализация консоли
//------------------------------------------------------------------------------
void console_init(MDR_UART_TypeDef *UARTx)
{    
    //u32 tmpreg;
    UART_InitTypeDef UART_InitStruct;
    PORT_InitTypeDef PORT_InitStruct;
     
    CON_UART = UARTx;
    memset( &console, 0, sizeof(console) );	 // Инициализация структуры
    
    // Разрешить тактирование UART
    if (UARTx == MDR_UART1)
        RST_CLK_PCLKcmd(RST_CLK_PCLK_UART1, ENABLE);
    else
        RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);
    UART_BRGInit(UARTx, UART_HCLKdiv1);      // Установит частоту HCLK
    
    // Настроить UART
    UART_InitStruct.UART_BaudRate   = CONSOLE_SPEED;
    UART_InitStruct.UART_WordLength = UART_WordLength8b;
    UART_InitStruct.UART_StopBits   = UART_StopBits1;
    UART_InitStruct.UART_Parity     = UART_Parity_No;
    UART_InitStruct.UART_FIFOMode   = UART_FIFO_OFF;
    UART_InitStruct.UART_HardwareFlowControl = 
                  UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;    
    UART_Init(UARTx,&UART_InitStruct);        

    // Подключить UART к ножкам
    // UART1 штатная консоль для МН353 взамен МН352, на LDM-K1986BE92QI можно 
    // перемычками вывести с XS9 на XS7 PB[5,6]
    if (UARTx == MDR_UART1) {
        PORT_InitStruct.PORT_Pin   = PORT_Pin_5 | PORT_Pin_6;
        PORT_InitStruct.PORT_FUNC  = PORT_FUNC_ALTER;
    }
    else {
        PORT_InitStruct.PORT_Pin   = PORT_Pin_0 | PORT_Pin_1;
        PORT_InitStruct.PORT_FUNC  = PORT_FUNC_OVERRID;
    }
    PORT_InitStruct.PORT_SPEED = PORT_SPEED_SLOW;
    PORT_InitStruct.PORT_OE    = PORT_OE_IN;
    PORT_InitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
    PORT_InitStruct.PORT_GFEN  = PORT_GFEN_ON;
    PORT_InitStruct.PORT_PD    = PORT_PD_DRIVER;
    PORT_InitStruct.PORT_PD_SHM = PORT_PD_SHM_OFF;
    PORT_InitStruct.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
    PORT_InitStruct.PORT_PULL_UP = PORT_PULL_UP_ON;
    
    if (UARTx == MDR_UART1)
        PORT_Init(MDR_PORTB, &PORT_InitStruct);        
    else
        PORT_Init(MDR_PORTF, &PORT_InitStruct);        

    UART_Cmd(UARTx, ENABLE);                   // Запустить
    UART_ITConfig(UARTx,UART_IT_RX,ENABLE);    // Разрешить прерывания по приему
    // Размаскировать NVIC 
    if (UARTx == MDR_UART1) {
        NVIC_DisableIRQ(UART2_IRQn);
        NVIC_EnableIRQ(UART1_IRQn);
    }
    else {
        NVIC_DisableIRQ(UART1_IRQn);
        NVIC_EnableIRQ(UART2_IRQn);
    }
}


//------------------------------------------------------------------------------
// Запись байта в циклический буфер передатчика
//------------------------------------------------------------------------------
void putk(int byte)
{
    if (dbg_print_mode == 0) {
	u32 t;
        u8 bt;
	
	if (byte == 0) return;
	
	do	
	    t = (console.tx.end + 1) & (TX_SIZE-1);
	while (t == console.tx.beg);			// Ждать свободного места
		
        console.tx.buf[ console.tx.end ] = (u8)byte;	// Поместить байт в буфер
        console.tx.end = (console.tx.end + 1) & (TX_SIZE-1);
	  
        if ((CON_UART->IMSC & UART_IMSC_TXIM) == 0) {  
            u16 b = console.tx.beg;
            bt = console.tx.buf[b];
            console.tx.beg = (b + 1)&(TX_SIZE-1);
            CON_UART->DR = bt;
            CON_UART->IMSC |= UART_IMSC_TXIM;      	// Разрешить прерывания	
        }
    }
    else {			
        // Работаем по опросу (не по прерыванию)
	if (byte == 0) {	
            // Дождаться окончания передачи и выйти
            while ((CON_UART->FR & UART_FR_TXFE) == 0);
	    return;
	}
        // Дождаться опустошения буфера и записать в него байт
        while (CON_UART->FR & UART_FR_TXFF);
        CON_UART->DR = byte;
        if (byte == '\n') putk('\r');
    }
}

//------------------------------------------------------------------------------
// Вывод данных с преобразованием формата
//------------------------------------------------------------------------------
int printf(const char *fmt, ...)
{
    int c;
    enum { LEFT, RIGHT } adjust;
    enum { LONG, INT } intsize;
    int fill;
    int width, max, len, base;
    static const char X2C_tab[]= "0123456789ABCDEF";
    static const char x2c_tab[]= "0123456789abcdef";
    const char *x2c;
    char *p;
    long i;
    unsigned long u;
    char temp[8 * sizeof(long) / 3 + 2];
    va_list argp;

    va_start(argp, fmt);

    while ((c= *fmt++) != 0) {
	if (c != '%') {
	    // Ordinary character. 
	    putk(c);
	    if (c == '\n') putk('\r');
	    continue;
	}
	// Format specifier of the form:
	//	%[adjust][fill][width][.max]keys
	//
	c= *fmt++;

	adjust= RIGHT;
	if (c == '-') {
	    adjust= LEFT;
	    c= *fmt++;
	}
	
	fill= ' ';
	if (c == '0') {
	    fill= '0';
	    c= *fmt++;
	}

	width= 0;
	if (c == '*') {
	    // Width is specified as an argument, e.g. %*d. 
	    width= va_arg(argp, int);
	    c= *fmt++;
	} else
	    if (isdigit(c)) {
		// A number tells the width, e.g. %10d. 
		do {
		    width= width * 10 + (c - '0');
		} while (isdigit(c= *fmt++));
	    }

	max= INT_MAX;
	if (c == '.') {
	    // Max field length coming up. 
	    if ((c= *fmt++) == '*') {
		max= va_arg(argp, int);
		c= *fmt++;
	    } else
		if (isdigit(c)) {
		    max= 0;
		    do {
			max= max * 10 + (c - '0');
		    } while (isdigit(c= *fmt++));
		}
	}

	// Set a few flags to the default. 
	x2c= x2c_tab;
	i= 0;
	base= 10;
	intsize= INT;
	if (c == 'l' || c == 'L') {
	    // "Long" key, e.g. %ld. 
	    intsize= LONG;
	    c= *fmt++;
	}
	if (c == 0) break;

	switch (c) {
	   		// Decimal. 
	   case 'd':
			i= intsize == LONG ? va_arg(argp, long)
						: va_arg(argp, int);
			u= i < 0 ? -i : i;
			goto int2ascii;

	   		// Octal. 
	   case 'o':
			base= 010;
			goto getint;

	   		// Pointer, interpret as %X or %lX. 
	   case 'p':
			if (sizeof(char *) > sizeof(int)) intsize= LONG;

	   		// Hexadecimal.  %X prints upper case A-F, not %lx. 
	   case 'X':
			x2c= X2C_tab;
	   case 'x':
			base= 0x10;
			goto getint;

			// Unsigned decimal. 
	   case 'u':
		getint:
			u= intsize == LONG ? va_arg(argp, unsigned long)
						: va_arg(argp, unsigned int);
		int2ascii:
			p= temp + sizeof(temp)-1;
			*p= 0;
			do {
			    *--p= x2c[(ptrdiff_t) (u % base)];
			} while ((u /= base) > 0);
			goto string_length;

			// A character. 
	   case 'c':
			p= temp;
			*p= va_arg(argp, int);
			len= 1;
			goto string_print;

			// Simply a percent. 
	   case '%':
			p= temp;
			*p= '%';
			len= 1;
			goto string_print;

			// A string.  The other cases will join in here. 
	   case 's':
			p= va_arg(argp, char *);

		string_length:
			for (len= 0; p[len] != 0 && len < max; len++) {}

		string_print:
			width -= len;
			if (i < 0) width--;
			if (fill == '0' && i < 0) putk('-');
			if (adjust == RIGHT) {
			    while (width > 0) { putk(fill); width--; }
			}
			if (fill == ' ' && i < 0) putk('-');
			while (len > 0) { putk((unsigned char) *p++); len--; }
			while (width > 0) { putk(fill); width--; }
			break;

			// Unrecognized format key, echo it back. 
	   default:
			putk('%');
			putk(c);
	}
    }

    // Mark the end with a null (should be something else, like -1). 
    putk(0);
    va_end(argp);
    //while( console.tx.beg != console.tx.end );
    return 0;
}

char *GetConsoleName(void) 
{
     if (CON_UART == MDR_UART1) return "MDR_UART1";
     else return "MDR_UART2";
}

void console_set_poll_mode()
{
  dbg_print_mode = 0xF;
  // запретить прерывания по передаче
  UART_ITConfig(CON_UART,UART_IT_TX,DISABLE);
}

extern void hard_reset();
void dbg_put(char c)
{
  //u8_t byte = c+0x30;
  dbg_print_mode = 0xF;
  // запретить прерывания по передаче
  UART_ITConfig(CON_UART,UART_IT_TX,DISABLE);
  //CON_UART->IMSC &= ~UART_IT;

  switch(c) {
  case 3: printf("\r\nHard Fault\r\n"); break;
  case 4: printf("\r\nMemory Management Fault\r\n"); break;
  case 5: printf("\r\nBus Fault\r\n"); break;
  case 6: printf("\r\nUsage Fault\r\n"); break;
  default: printf("\r\nUnknown Fault\r\n"); break;
  }
  /*
  // Дождаться опустошения буфера и записать в него байт
  while (CON_UART->FR & UART_FR_TXFF);
  CON_UART->DR = byte; 
  while (CON_UART->FR & UART_FR_TXFF);
  CON_UART->DR = byte;
  while (CON_UART->FR & UART_FR_TXFF);
  */
  hard_reset();
}


int getc()
{
    int c;
    while ((c=getch()) < 0);
    return c;
}

char * __gets(char *s, int n) 
{
    register int ch;
    register char *ptr;
    int nn = n;

    ptr = s;
a:    
    while (nn > 0 ) {
        ch = getc();
        if (ch == 127) {        // Backspace
            if (nn != n) {
               nn++;
               ptr--;
               putk(ch);
            }
            continue;
        }
        putk(ch);
        if (ch == '\r')
            putk('\n');
	*ptr++ = ch;
        nn--;
	if ((ch == '\n') || (ch == '\r'))
	    break;
    }
    if ((ch != '\n') && (ch != '\r')) {
        // Дошли до края, ждём Enter или Backspace
        for (;;) {
            ch = getc();
            if ((ch == '\n') || (ch == '\r')) {
                putk('\n');
                putk('\r');
                break;
            }
            if (ch == 127) {        // Backspace
                 nn++;
                 ptr--;
                 putk(ch);
                 goto a;
            }
        }
    }
    *ptr = '\0';
    return s;
}





