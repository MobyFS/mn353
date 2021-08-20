
//#include <1986BE91.h>
#include <MDR32Fx.h>
#include <mn353.h>
#include <stdio.h>
#include <types.h>
#include <FreeRTOS.h>
#include <task.h>
#include <restart.h>
#include <app.h>
#include <serial.h>

extern void putk(char); 
extern void console_set_poll_mode(void);
extern u32 RST_CLK_GetCpuClock(void);
extern char *GetConsoleName(void);
extern int getch(void);
extern void spi_init(void);
extern void spi_test();
extern void show_link_err();


#pragma section="CSTACK"

extern void console_init();
extern void ShowLCD();

void show_info()
{
    u32 cpuclk;
  
    cpuclk = RST_CLK_GetCpuClock();
    printf("\n"
         "LDM-K1986BE92QI Evaluation Board (MH353 prototype)\r\n"
         "CPU ID:  %08X (1986BE9x)\r\n"
         "CPU_CLK: %u Hz (%u.%u MHz)\r\n",
         SCB->CPUID,
         cpuclk,
         cpuclk/1000000,
         cpuclk - ((cpuclk/1000000)*1000000)
         );
    printf("STACK:   %08x-%08x size %u\r\n",
         __section_begin("CSTACK"),
         __section_end("CSTACK"),
         __section_size("CSTACK"));
    printf("CONSOLE: %s\r\n\n>",GetConsoleName());
}

#ifdef  LCD_ENABLE
extern void LCD_CLS();
extern void banner(int);
#endif
extern int rxirq_count;
extern void LightsOn(void *stub);

/*
static void Pause(void) {
    u32 i;
    for (i = 15000; i > 0; i--) __NOP();
}
*/
TaskHandle_t ledHandle;
u8_t props[14];

static void thread_main(void *stub)
{
#ifdef  LCD_ENABLE     
    int im = 0;
#endif
    int i;
    char inp[9];
    TaskHandle_t xHandle = NULL;

    xTaskCreate(LightsOn, "LEDS", 
                configMINIMAL_STACK_SIZE, NULL, 
                tskIDLE_PRIORITY, &ledHandle);
    
    
    show_info();
    
    for (;;) {
        __gets(inp,8);
        switch (inp[0]) {
            case '\r':
            case '\n': putk('>'); break;

            case 'h':
            case 'H':
               printf("h - help\r\n"
                      "i - show info\r\n"
#ifdef  LCD_ENABLE     
                      "l - start LCD show\r\n"
                      "d - stop LCD show\r\n"
#endif
                      "r - restart\r\n"
                      "s - spi test\r\n>"
                      "1 - serial init\r\n>"
                      "2 - serial send\r\n>"
                      "3 - mlink up\r\n>"
                      "4 - mlink down\r\n>"
                      "5 - start session\r\n>"
                      "6 - loopback\r\n>"
                      "7 - get properties\r\n>"
                      "8 - end session\r\n>"
                      "9 - mlink stat\r\n>"
                      );
               break;
            case 'R':
            case 'r':  restart(RST_T_TEST,__FILE__,__LINE__);

            case 'S':
            case 's':  spi_test(); putk('>'); break;

            case 'I':
            case 'i':  show_info(); break;
#ifdef  LCD_ENABLE            
            case 'L':
            case 'l':  // Конфликтует с SPI
                       printf("LCD is running (SPI blocked)\r\n>");
                       xTaskCreate(ShowLCD,"LCD",configMINIMAL_STACK_SIZE, 
                                   NULL, tskIDLE_PRIORITY, &xHandle );                  
                       break;
            case 'D':
            case 'd':  
                       if (xHandle != NULL) {
                           printf("LCD removed\r\n>");
                           vTaskDelete( xHandle );
                           xHandle = NULL;
                           LCD_CLS();
                           // Переинициализировать из-за конфликта
                           spi_init();
                       }
                       else putk('>');
                       break;
            case 'n':
                       im++;
                       banner(im & 0x3);
                       spi_init();
                       putk('>');
                       break;
            case 'w': 
                       
                       //__disable_interrupt();
                       //puts("Waitng for Rx...\r\n");
                       vTaskSuspend( ledHandle );
                       //vTaskSuspendAll();
                       vTaskDelay(1000);
                       rxirq_count = 0;
                       for (;;) {                          
                           vPortSuppressTicksAndSleep(100000);
                           if (rxirq_count)
                               break;
                       }
                       im = 0;
                       //*__WFI(); // Wait for Interrupt
                       vTaskResume( ledHandle );
                       //xTaskResumeAll();
                       putk('>');
                       break;
            case 'z':  
                       vTaskEndScheduler();
                       __DSB();    // Ensure completion of memory access
                      // vTaskDelete(0);
                       break;
                       
           case 'e': vTaskDelete( NULL ); break;

           case '1':
                     serial_init(MDR_UART2);
                     break;

           case '2':
                     for (i=0; i<32; i++)
                         serial_send_byte((u8_t)(i+0x30));
                     break;
        case '3':
                        printf("Start data link\r\n>");
                        app_init();
                        app_start();
                        break;
        case '4':
                        printf("Stop data link\r\n>");
                        app_stop();
                        break;
                        
        case '5': 
                        printf("НАЧАТЬ ОБМЕН\r\n");
                        if (sync_cmd(0)) printf("FAILD\n\r>");
                        else printf("OK\n\r>");
                        break;
        case '6':
                        printf("ШЛЕЙФ\r\n");
                        if (loop_cmd(0)) printf("FAILD\n\r>");
                        else printf("OK\n\r>");
                        break;
        case '7':
                        printf("ЗАПРОС РЕКВИЗИТОВ\r\n");
                        if (props_cmd(0,props)) printf("FAILD\n\r>");
                        else {
                            printf("OK\n\r");
                            for (i=0; i<14; i++)
                                printf("%02X ",props[i]);
                            printf("\n\r>");
                        }
                        break;
        case '8':
                        printf("ЗАКОНЧИТЬ ОБМЕН\r\n");
                        if (end_cmd(0)) printf("FAILD\n\r>");
                        else printf("OK\n\r>");
                        break;
                        
        case '9':
                        show_link_err();
                        printf("\n\r>");
                        break;
#endif
        default:   putk('>');
        }
    }  
}


#include <stdio.h>
//#include <gunzip.h>
int main() 
{
// unsigned long size;
//  fputs("start",stdout);
    SCB->VTOR = 0x8000000;        // Смещение таблицы векторов
#ifdef  LCD_ENABLE            
    banner(0);
#endif  
    console_init(MDR_UART1);
  
    spi_init();
  
//gunzip(0,&size,0,&size);    
    
    __enable_interrupt();
  
    xTaskCreate(thread_main, "CONSOLE", 
                configMINIMAL_STACK_SIZE, NULL, 
                tskIDLE_PRIORITY, NULL);
  
//    xTaskCreate(LightsOn, "LEDS", 
//                configMINIMAL_STACK_SIZE, NULL, 
//                tskIDLE_PRIORITY, &ledHandle);

    vTaskStartScheduler();
    restart(RST_T_SYS,__FILE__,__LINE__);
}