
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

#define SERIAL_SPEED 19200

static MDR_UART_TypeDef *UART;

static SERIAL_LINE serial;

//------------------------------------------------------------------------------
// Обработчик прерывания "приёмник полон"
//------------------------------------------------------------------------------
static inline void irq_rx( u8 byte )
{  
    // Сохранить байт в циклическом буфере, из которого можно считать
    // с помощью функции getch()
    u8 e = (serial.rx.e + 1) & (RX_SIZE-1);
    if (e == serial.rx.b) 
        return;    // Переполнение входного буфера
    serial.rx.buf[serial.rx.e] = byte;
    serial.rx.e = e;
}

//------------------------------------------------------------------------------
// Обработчик прерывания "передатчик пуст"
//------------------------------------------------------------------------------
static inline int irq_tx()
{
    u8 byte;
    u16 b = serial.tx.beg;
    if ( b == serial.tx.end ) return -1;
    byte = serial.tx.buf[b];
    serial.tx.beg = (b + 1)&(TX_SIZE-1);
    return byte;
}

void UART_HandlerWork(void)
{
    u32 SR;
    while(1) {
        SR = UART->MIS;
        
        if (SR == 0) 
            // Всё обработано
            break;
        if ( SR & UART_MIS_RXMIS)
            // Обработать принятый байт
            irq_rx(UART->DR);
        if ( SR & UART_MIS_TXMIS ) {
            int byte;
            byte = irq_tx(); // Есть чего передавать? 
            if ( byte < 0 ) 
                // Нет - запретить прерывания по передаче
                UART_ITConfig(UART,UART_IT_TX,DISABLE);
            else 
                // Да - передать
                UART->DR = (u8)byte;
        }
    }
}

void UART2_IRQHandler(void)
{
    UART_HandlerWork();
}
/*
void UART1_IRQHandler(void)
{
    UART_HandlerWork();
}
*/

//------------------------------------------------------------------------------
// Инициализация последовательного интерфейса
//------------------------------------------------------------------------------
void serial_init(MDR_UART_TypeDef *UARTx)
{    
    UART_InitTypeDef UART_InitStruct;
    PORT_InitTypeDef PORT_InitStruct;
     
    UART = UARTx;
    memset(&serial,0,sizeof(serial));    // Инициализация структуры
    
    // Разрешить тактирование UART
    if (UARTx == MDR_UART1)
        RST_CLK_PCLKcmd(RST_CLK_PCLK_UART1, ENABLE);
    else
        RST_CLK_PCLKcmd(RST_CLK_PCLK_UART2, ENABLE);
    UART_BRGInit(UARTx, UART_HCLKdiv1);      // Установит частоту HCLK
    
    // Настроить UART
    UART_InitStruct.UART_BaudRate   = SERIAL_SPEED;
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
        //PORT_InitStruct.PORT_FUNC  = PORT_FUNC_OVERRID;
        PORT_InitStruct.PORT_FUNC  = PORT_FUNC_ALTER; 
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
        //PORT_Init(MDR_PORTF, &PORT_InitStruct); 
        PORT_Init(MDR_PORTD, &PORT_InitStruct);

    UART_Cmd(UARTx, ENABLE);                   // Запустить
    UART_ITConfig(UARTx,UART_IT_RX,ENABLE);    // Разрешить прерывания по приему
    // Размаскировать NVIC 
    if (UARTx == MDR_UART1) {
        //NVIC_DisableIRQ(UART2_IRQn);
        NVIC_EnableIRQ(UART1_IRQn);
    }
    else {
        //NVIC_DisableIRQ(UART1_IRQn);
        NVIC_EnableIRQ(UART2_IRQn);
    }
}


void serial_close(MDR_UART_TypeDef* UARTx)
{
  /* Check the parameters */
  assert_param(IS_UART_ALL_PERIPH(UARTx));

  /* Clear UART CR */
  UARTx->CR = 0;
  UARTx->LCR_H = 0;
  UARTx->RSR_ECR = 0;
  UARTx->FR = UART_FLAG_TXFE | UART_FLAG_RXFE;
  UARTx->ILPR = 0;
  UARTx->IBRD = 0;
  UARTx->FBRD = 0;
  UARTx->IFLS = UART_IT_FIFO_LVL_8words;
  UARTx->IMSC = 0;
  UARTx->DMACR = 0;
  /* Set UART CR[RXE] and UART CR[TXE] bits */
  //UARTx->CR = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;
}


void serial_send_byte(u8_t byte)
{
    u32 t;
    u8 bt;

    do      
        t = (serial.tx.end + 1) & (TX_SIZE-1);
    while (t == serial.tx.beg);                     // Ждать свободного места
            
    serial.tx.buf[ serial.tx.end ] = (u8)byte;      // Поместить байт в буфер
    serial.tx.end = (serial.tx.end + 1) & (TX_SIZE-1);
      
    if ((UART->IMSC & UART_IMSC_TXIM) == 0) {  
        u16 b = serial.tx.beg;
        bt = serial.tx.buf[b];
        serial.tx.beg = (b + 1)&(TX_SIZE-1);
        UART->DR = bt;
        UART->IMSC |= UART_IMSC_TXIM;                // Разрешить прерывания 
    }
}

void serial_send_block(u8_t *buf,int size)
{
    int i;
    for (i=0; i<size; i++) 
        serial_send_byte(buf[i]);
}

int serial_write(const void* buf, int write_len)
{
    serial_send_block((u8_t *)buf,write_len);
    return write_len;
  
#if 0
    AT91PS_USART US;
    SERIAL* m_serial;
    SERIAL_TX_BUF *txbuf;
    u32_t size, t;
    u32_t t1,os;
    const u32_t const_len = write_len;
    const u8_t *in_buf = (u8_t*)buf;
    
    ASSERT(ser_n < SERIAL_COUNT);
    
    US = usart_configs[ser_n].US;
    m_serial = &serials[ser_n];
    
    while (write_len)
    {
        t = (m_serial->txend + 1) & (BUF_TX_CHAIN_LEN-1);
        if ( t == m_serial->txbeg )
        {
                m_serial->tx_ovf++;
                break; // нет свободного буфера для передачи
        }
        txbuf = &m_serial->txbuf[m_serial->txend];

        if ( write_len <=  BUF_TX_LEN ) size = write_len;
        else size = BUF_TX_LEN;         
        memcpy( txbuf->data, in_buf, size );
        txbuf->beg = size;
        t1 = m_serial->txend;
        
        os = _OSDisable();
        m_serial->txend = t;            
        
        if (( US->US_TCR == 0 ) && (t1 == m_serial->txbeg))
        {
            US->US_TPR = (AT91_REG)txbuf->data;
            US->US_TCR = size;
            US->US_IER = AT91C_US_TXBUFE;                   
            US->US_PTCR = AT91C_PDC_TXTEN;
        }
        _OSEnable(os);
        
        write_len -= size;
        in_buf += size;
    }
    
    return const_len - write_len;
#endif
}


int get_byte()
{
    u8 byte;

    if (serial.rx.b == serial.rx.e) {
        //Dispatch(1);
        return -1;
    }
    byte = serial.rx.buf[serial.rx.b];
    serial.rx.b = (serial.rx.b + 1) & (RX_SIZE-1);
    return byte;
}


int serial_read(void* buf, int len_read)
{
    u8_t* out_buf = (u8_t*)buf;
    int c,len = 0;
    
    while (((c=get_byte()) >= 0) && (len < len_read)) {
        out_buf[len] = (u8_t)c;
        len++;
    }
    return len;
}


