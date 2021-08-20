#ifndef __SERIAL_H
#define __SERIAL_H

#define TX_SIZE 256
#define RX_SIZE 256
#define CONSOLE_SPEED    115200

typedef struct {
    u16 beg, end;
    u8 buf[TX_SIZE];
} TXDATA;

typedef struct {
    u8 b, e;
    u8 buf[RX_SIZE];
    int getch_mode;
} RXDATA;

typedef struct {
    TXDATA tx;
    RXDATA rx;
} SERIAL_LINE;

extern void serial_init(MDR_UART_TypeDef *UARTx);
extern void serial_close(MDR_UART_TypeDef* UARTx);
extern int serial_read(void* buf, int len_read);
extern int serial_write(const void* buf, int write_len);
extern void serial_send_byte(u8_t byte);




#endif /* __SERIAL_H */
