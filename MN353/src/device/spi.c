
// spi

#include <MDR32Fx.h>
#include <mn353.h>
#include <types.h>
#include <stdio.h>
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_port.h"

extern u32 RST_CLK_GetCpuClock(void);


#define SPI_TX_SIZE 16
#define SPI_RX_SIZE 16
#define SPI_SPEED 2000000

typedef struct {
    u16 beg, end;
    u16 buf[SPI_TX_SIZE];
} SPITXDATA;

typedef struct {
    u8 b, e;
    u16 buf[SPI_RX_SIZE];
    int getch_mode;
} SPIRXDATA;

typedef struct {
    SPITXDATA tx;
    SPIRXDATA rx;
} SPI_LINE;

static SPI_LINE spi;

//------------------------------------------------------------------------------
// ���������� ���������� �� ���������� ������ FIFO ���������
//------------------------------------------------------------------------------
static inline void irq_rx(u16 byte)
{
    // ��������� ���� � ����������� ������, �� �������� ����� �������
    // � ������� ������� spi_getch()
    u8 e = (spi.rx.e + 1) & (SPI_RX_SIZE-1);
    if (e == spi.rx.b) 
        return; 			// ������������ �������� ������
    spi.rx.buf[spi.rx.e] = byte;
    spi.rx.e = e;
}

//------------------------------------------------------------------------------
// ���������� ���������� �� ���������� ������ FIFO �����������
//------------------------------------------------------------------------------
static inline int irq_tx()
{
    u16 byte;
    u16 b = spi.tx.beg;
    if (b == spi.tx.end) 
        return -1;
    byte = spi.tx.buf[b];
    spi.tx.beg = (b + 1)&(SPI_TX_SIZE-1);
    return ((int)byte)&0xFF;
}


void SPI_HandlerWork(void)
{
    u32 SR;
    while(1) {
        SR = MDR_SSP1->MIS;
        
        if (SR == 0) break; // �� ����������
            
        if ( SR & SSP_MIS_RXMIS) {
            // ���������� �������� ����
            while (MDR_SSP1->SR & SSP_SR_RNE) {
                irq_rx(MDR_SSP1->DR);
            }
        }
        if ( SR & SSP_MIS_TXMIS ) {
            int byte;
            byte = irq_tx(); // ���� ���� ����������? 
            if ( byte < 0 ) 
                // ��� - ��������� ���������� �� ��������
                SSP_ITConfig(MDR_SSP1,SSP_IT_TX,DISABLE);
            else 
                // �� - ��������
                MDR_SSP1->DR = (u16)byte;
        }
        if (SR & SSP_MIS_RTMIS) {
            // ���-�� �������� � FIFO ���������
            while (MDR_SSP1->SR & SSP_SR_RNE) {
                irq_rx(MDR_SSP1->DR);
            }          
        }
    }  
}

void SSP1_IRQHandler(void)
{
      SPI_HandlerWork();  
}

void spi_init(void)
{
    SSP_InitTypeDef SPI_InitStruct;
    PORT_InitTypeDef PORT_InitStruct;
    
    // ��������� ������������ SSP1
    RST_CLK_PCLKcmd(RST_CLK_PCLK_SSP1, ENABLE);
    SSP_BRGInit(MDR_SSP1, SSP_HCLKdiv1);      // ��������� ������� SSPCLK=HCLK
    
    // ��������� ��������� �������� ������ SSP
    SPI_InitStruct.SSP_Mode = SSP_ModeMaster;
    SPI_InitStruct.SSP_WordLength = SSP_WordLength8b;
    SPI_InitStruct.SSP_SPO = SSP_SPO_Low;
    SPI_InitStruct.SSP_SPH = SSP_SPH_1Edge;
    SPI_InitStruct.SSP_FRF = SSP_FRF_SPI_Motorola;
    // ������ �������� ������
    SPI_InitStruct.SSP_CPSDVSR = 2;
    SPI_InitStruct.SSP_SCR = (RST_CLK_GetCpuClock()/2/SPI_SPEED)-1;  
    SPI_InitStruct.SSP_HardwareFlowControl = 0;
    
    SSP_Init(MDR_SSP1, &SPI_InitStruct);
    
    // ���������� SSP1 � ������
#ifndef LDM_K1986BE92QI    
    // � ������ ��353 ��� ������ ���� PB[12-15] 
    PORT_InitStruct.PORT_Pin   = PORT_Pin_12 | PORT_Pin_13 | PORT_Pin_14 | PORT_Pin_15;
#else    
    // ��� LDM-K1986BE92QI ������ ���� PF[0-3], ���� ����������� � UART2 �� XS7,
    // �� ����� ����������� ������� �� XS7 PB[5,6] ��� �������.
    PORT_InitStruct.PORT_Pin   = PORT_Pin_0 | PORT_Pin_1 | PORT_Pin_2 | PORT_Pin_3;
#endif
    PORT_InitStruct.PORT_FUNC  = PORT_FUNC_ALTER;
    PORT_InitStruct.PORT_SPEED = PORT_SPEED_FAST;
    PORT_InitStruct.PORT_OE    = PORT_OE_IN;
    PORT_InitStruct.PORT_MODE  = PORT_MODE_DIGITAL;
    PORT_InitStruct.PORT_GFEN  = PORT_GFEN_ON;
    PORT_InitStruct.PORT_PD    = PORT_PD_DRIVER;
    PORT_InitStruct.PORT_PD_SHM = PORT_PD_SHM_OFF;
    PORT_InitStruct.PORT_PULL_DOWN = PORT_PULL_DOWN_OFF;
    PORT_InitStruct.PORT_PULL_UP = PORT_PULL_UP_ON;
    
#ifndef LDM_K1986BE92QI    
    PORT_Init(MDR_PORTB, &PORT_InitStruct);   // ��353
#else    
    PORT_Init(MDR_PORTF, &PORT_InitStruct);   // LDM-K1986BE92QI
#endif
    SSP_Cmd(MDR_SSP1, ENABLE);                // ��������� SSP1
    // ��������� ���������� �� ������ � �������������� NVIC
    SSP_ITConfig(MDR_SSP1, SSP_IT_RX | SSP_IT_RT, ENABLE);    
    NVIC_EnableIRQ(SSP1_IRQn);
}

//------------------------------------------------------------------------------
// ������ ����� � ����������� ����� �����������
//------------------------------------------------------------------------------
void spi_send(int byte)
{
    u32 t;
    u16 bt;
    
    // ����� ���������� �����	
    do {	
        t = (spi.tx.end + 1) & (SPI_TX_SIZE-1);
    } while (t == spi.tx.beg);			
    
    // ��������� ���� � �����	
    spi.tx.buf[spi.tx.end] = (u8)byte;	
    spi.tx.end = (spi.tx.end + 1) & (SPI_TX_SIZE-1);
	
    // ��������� ��������, ���� ��� �� ��������
    if ((MDR_SSP1->IMSC & SSP_IMSC_TXIM) == 0) {  
        u16 b = spi.tx.beg;
        bt = spi.tx.buf[b];
        spi.tx.beg = (b + 1)&(SPI_TX_SIZE-1);
        MDR_SSP1->DR = bt;
        SSP_ITConfig(MDR_SSP1, SSP_IT_TX, ENABLE);  // ��������� ����������	
    }
}

//------------------------------------------------------------------------------
// ������� ���� �� ������
//------------------------------------------------------------------------------
int spi_getch()
{
    u8 byte;

    if (spi.rx.b == spi.rx.e) {
	//Dispatch(1);
	return -1;
    }
    byte = spi.rx.buf[spi.rx.b];
    spi.rx.b = (spi.rx.b + 1) & (SPI_RX_SIZE-1);
    return byte;
}

void spi_test()
{
    int i,j,b,e;
    
    printf("\r\nTest SPI at speed %d.%u MHz... ",SPI_SPEED/1000000,SPI_SPEED%1000000);
    e = 0;
    for (j=0; j<32; j++) {
        for (i=0; i<8; i++)
            spi_send(j*8+i);
        for (i=0; i<8; i++) {
            while ((b=spi_getch()) < 0);
            if (b != (j*8+i))
            e++;
        }
    }
    printf("done.\r\nErrors: %d\r\n",e);
}

