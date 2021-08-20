///////////////////////////////////////////////////////////////////////////////
//
// IAR ANSI C/C++ Compiler V7.70.1.11437/W32 for ARM      17/Mar/2021  17:48:06
// Copyright 1999-2016 IAR Systems AB.
//
//    Cpu mode     =  thumb
//    Endian       =  little
//    Source file  =  E:\Zh\Cortex\soft\mn353_mdr\MN353\src\console.c
//    Command line =  
//        E:\Zh\Cortex\soft\mn353_mdr\MN353\src\console.c -D USE_MDR1986VE9x
//        -lCN E:\Zh\Cortex\soft\mn353_mdr\Debug\List -lb
//        E:\Zh\Cortex\soft\mn353_mdr\Debug\List -o
//        E:\Zh\Cortex\soft\mn353_mdr\Debug\Obj --no_cse --no_unroll
//        --no_inline --no_code_motion --no_tbaa --no_clustering
//        --no_scheduling --debug --endian=little --cpu=Cortex-M3
//        --no_path_in_file_macros -e --fpu=None --dlib_config "C:\Program
//        Files (x86)\IAR Systems\Embedded Workbench
//        7.5\arm\INC\c\DLib_Config_Normal.h" -I
//        E:\Zh\Cortex\soft\mn353_mdr\FreeRTOS\src\include\ -I
//        E:\Zh\Cortex\soft\mn353_mdr\MDR_Library\inc\ -I
//        E:\Zh\Cortex\soft\mn353_mdr\startup\ -I
//        E:\Zh\Cortex\soft\mn353_mdr\mn353\inc\ -Ol --use_c++_inline
//    Locale       =  Russian_Russia.1251
//    List file    =  E:\Zh\Cortex\soft\mn353_mdr\Debug\List\console.s
//
///////////////////////////////////////////////////////////////////////////////

        #define SHT_PROGBITS 0x1

        EXTERN PORT_Init
        EXTERN RST_CLK_PCLKcmd
        EXTERN UART_BRGInit
        EXTERN UART_Cmd
        EXTERN UART_ITConfig
        EXTERN UART_Init
        EXTERN __aeabi_memset
        EXTERN hard_reset
        EXTERN isdigit

        PUBLIC CON_UART_HandlerWork
        PUBLIC GetConsoleName
        PUBLIC RST_CLK_GetCpuClock
        PUBLIC UART1_IRQHandler
        PUBLIC __gets
        PUBLIC console_clear_con
        PUBLIC console_init
        PUBLIC console_set_poll_mode
        PUBLIC dbg_put
        PUBLIC getc
        PUBLIC getch
        PUBLIC printf
        PUBLIC putk
        PUBLIC rxirq_count


        SECTION `.text`:CODE:NOROOT(1)
        THUMB
// static __interwork __softfp void NVIC_EnableIRQ(IRQn_Type)
NVIC_EnableIRQ:
        MOVS     R1,#+1
        ANDS     R2,R0,#0x1F
        LSLS     R1,R1,R2
        LDR.W    R2,??DataTable13  ;; 0xe000e100
        SXTB     R0,R0            ;; SignExt  R0,R0,#+24,#+24
        LSRS     R0,R0,#+5
        STR      R1,[R2, R0, LSL #+2]
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
// static __interwork __softfp void NVIC_DisableIRQ(IRQn_Type)
NVIC_DisableIRQ:
        MOVS     R1,#+1
        ANDS     R2,R0,#0x1F
        LSLS     R1,R1,R2
        LDR.W    R2,??DataTable13_1  ;; 0xe000e180
        SXTB     R0,R0            ;; SignExt  R0,R0,#+24,#+24
        LSRS     R0,R0,#+5
        STR      R1,[R2, R0, LSL #+2]
        BX       LR               ;; return

        SECTION `.bss`:DATA:REORDER:NOROOT(0)
        DATA
dbg_print_mode:
        DS8 1

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
CON_UART:
        DS8 4

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
console:
        DS8 524

        SECTION `.bss`:DATA:REORDER:NOROOT(2)
        DATA
rxirq_count:
        DS8 4

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
irq_rx:
        LDR.W    R1,??DataTable13_2
        LDR      R1,[R1, #+0]
        ADDS     R1,R1,#+1
        LDR.W    R2,??DataTable13_2
        STR      R1,[R2, #+0]
        LDR.W    R1,??DataTable13_3
        LDRB     R1,[R1, #+261]
        ADDS     R1,R1,#+1
        MOVS     R2,R1
        LDR.W    R3,??DataTable13_3
        LDRB     R3,[R3, #+260]
        UXTB     R2,R2            ;; ZeroExt  R2,R2,#+24,#+24
        CMP      R2,R3
        BEQ.N    ??irq_rx_0
??irq_rx_1:
        LDR.W    R2,??DataTable13_3
        LDR.W    R3,??DataTable13_3
        LDRB     R3,[R3, #+261]
        ADD      R2,R2,R3
        STRB     R0,[R2, #+262]
        LDR.W    R0,??DataTable13_3
        STRB     R1,[R0, #+261]
??irq_rx_0:
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
irq_tx:
        LDR.W    R0,??DataTable13_3
        LDRH     R1,[R0, #+0]
        MOVS     R0,R1
        LDR.W    R2,??DataTable13_3
        LDRH     R2,[R2, #+2]
        UXTH     R0,R0            ;; ZeroExt  R0,R0,#+16,#+16
        CMP      R0,R2
        BNE.N    ??irq_tx_0
        MOVS     R0,#-1
        B.N      ??irq_tx_1
??irq_tx_0:
        LDR.W    R0,??DataTable13_3
        UXTH     R1,R1            ;; ZeroExt  R1,R1,#+16,#+16
        ADD      R0,R0,R1
        LDRB     R0,[R0, #+4]
        ADDS     R1,R1,#+1
        AND      R1,R1,#0xFF
        LDR.W    R2,??DataTable13_3
        STRH     R1,[R2, #+0]
        UXTB     R0,R0            ;; ZeroExt  R0,R0,#+24,#+24
??irq_tx_1:
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
CON_UART_HandlerWork:
        PUSH     {R4,LR}
        B.N      ??CON_UART_HandlerWork_0
??CON_UART_HandlerWork_1:
        MOVS     R2,#+0
        MOVS     R1,#+32
        LDR.W    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        BL       UART_ITConfig
??CON_UART_HandlerWork_0:
        LDR.W    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR      R4,[R0, #+64]
        CMP      R4,#+0
        BNE.N    ??CON_UART_HandlerWork_2
        POP      {R4,PC}          ;; return
??CON_UART_HandlerWork_2:
        LSLS     R0,R4,#+27
        BPL.N    ??CON_UART_HandlerWork_3
        LDR.W    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR      R0,[R0, #+0]
        UXTB     R0,R0            ;; ZeroExt  R0,R0,#+24,#+24
        BL       irq_rx
??CON_UART_HandlerWork_3:
        LSLS     R0,R4,#+26
        BPL.N    ??CON_UART_HandlerWork_0
        BL       irq_tx
        CMP      R0,#+0
        BMI.N    ??CON_UART_HandlerWork_1
        UXTB     R0,R0            ;; ZeroExt  R0,R0,#+24,#+24
        LDR.W    R1,??DataTable13_4
        LDR      R1,[R1, #+0]
        STR      R0,[R1, #+0]
        B.N      ??CON_UART_HandlerWork_0

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
UART1_IRQHandler:
        PUSH     {R7,LR}
        BL       CON_UART_HandlerWork
        POP      {R0,PC}          ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
getch:
        LDR.W    R0,??DataTable13_3
        LDRB     R0,[R0, #+260]
        LDR.W    R1,??DataTable13_3
        LDRB     R1,[R1, #+261]
        CMP      R0,R1
        BNE.N    ??getch_0
        MOVS     R0,#-1
        B.N      ??getch_1
??getch_0:
        LDR.W    R0,??DataTable13_3
        LDR.W    R1,??DataTable13_3
        LDRB     R1,[R1, #+260]
        ADD      R0,R0,R1
        LDRB     R0,[R0, #+262]
        LDR.W    R1,??DataTable13_3
        LDRB     R1,[R1, #+260]
        ADDS     R1,R1,#+1
        LDR.W    R2,??DataTable13_3
        STRB     R1,[R2, #+260]
        UXTB     R0,R0            ;; ZeroExt  R0,R0,#+24,#+24
??getch_1:
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
console_clear_con:
        CPSID    I
        MOVS     R0,#+0
        LDR.W    R1,??DataTable13_3
        STRB     R0,[R1, #+260]
        MOVS     R0,#+0
        LDR.W    R1,??DataTable13_3
        STRB     R0,[R1, #+261]
        CPSIE    I
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
RST_CLK_GetCpuClock:
        LDR.W    R0,??DataTable13_5  ;; 0x7a1200
        LDR.W    R1,??DataTable13_6  ;; 0x4002000c
        LDR      R1,[R1, #+0]
        LSLS     R1,R1,#+31
        BPL.N    ??RST_CLK_GetCpuClock_0
        LSRS     R0,R0,#+1
??RST_CLK_GetCpuClock_0:
        LDR.W    R1,??DataTable13_6  ;; 0x4002000c
        LDR      R1,[R1, #+0]
        LSLS     R1,R1,#+29
        BPL.N    ??RST_CLK_GetCpuClock_1
        LDR.W    R1,??DataTable13_7  ;; 0x40020004
        LDR      R1,[R1, #+0]
        UBFX     R1,R1,#+8,#+4
        ADDS     R1,R1,#+1
        MULS     R0,R1,R0
??RST_CLK_GetCpuClock_1:
        LDR.W    R1,??DataTable13_6  ;; 0x4002000c
        LDR      R1,[R1, #+0]
        LSRS     R1,R1,#+8
        ANDS     R1,R1,#0x3
        CMP      R1,#+0
        BEQ.N    ??RST_CLK_GetCpuClock_2
        CMP      R1,#+2
        BEQ.N    ??RST_CLK_GetCpuClock_3
        BCC.N    ??RST_CLK_GetCpuClock_4
        B.N      ??RST_CLK_GetCpuClock_5
??RST_CLK_GetCpuClock_2:
        LDR.W    R0,??DataTable13_5  ;; 0x7a1200
        B.N      ??RST_CLK_GetCpuClock_6
??RST_CLK_GetCpuClock_4:
        LDR.W    R1,??DataTable13_6  ;; 0x4002000c
        LDR      R1,[R1, #+0]
        LSRS     R1,R1,#+4
        LSLS     R1,R1,#+28
        BPL.N    ??RST_CLK_GetCpuClock_7
??RST_CLK_GetCpuClock_8:
        MOVS     R1,#+1
        LDR.W    R2,??DataTable13_6  ;; 0x4002000c
        LDR      R2,[R2, #+0]
        UBFX     R2,R2,#+4,#+3
        ADDS     R2,R2,#+1
        LSLS     R1,R1,R2
        UDIV     R0,R0,R1
??RST_CLK_GetCpuClock_7:
        B.N      ??RST_CLK_GetCpuClock_6
??RST_CLK_GetCpuClock_3:
        MOV      R0,#+32768
        B.N      ??RST_CLK_GetCpuClock_6
??RST_CLK_GetCpuClock_5:
        MOVW     R0,#+40000
??RST_CLK_GetCpuClock_6:
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
console_init:
        PUSH     {R4,R5,LR}
        SUB      SP,SP,#+28
        MOVS     R4,R0
        LDR.W    R0,??DataTable13_4
        STR      R4,[R0, #+0]
        MOV      R1,#+524
        MOVS     R2,#+0
        LDR.W    R5,??DataTable13_3
        MOVS     R0,R5
        BL       __aeabi_memset
        LDR.W    R0,??DataTable13_8  ;; 0x40030000
        CMP      R4,R0
        BNE.N    ??console_init_0
        MOVS     R1,#+1
        MOVS     R0,#+64
        BL       RST_CLK_PCLKcmd
        B.N      ??console_init_1
??console_init_0:
        MOVS     R1,#+1
        MOVS     R0,#+128
        BL       RST_CLK_PCLKcmd
??console_init_1:
        MOVS     R1,#+0
        MOVS     R0,R4
        BL       UART_BRGInit
        MOVS     R0,#+115200
        STR      R0,[SP, #+12]
        MOVS     R0,#+96
        STRH     R0,[SP, #+16]
        MOVS     R0,#+0
        STRH     R0,[SP, #+18]
        MOVS     R0,#+0
        STRH     R0,[SP, #+20]
        MOVS     R0,#+0
        STRH     R0,[SP, #+22]
        MOV      R0,#+768
        STRH     R0,[SP, #+24]
        ADD      R1,SP,#+12
        MOVS     R0,R4
        BL       UART_Init
        LDR.W    R0,??DataTable13_8  ;; 0x40030000
        CMP      R4,R0
        BNE.N    ??console_init_2
        MOVS     R0,#+96
        STRH     R0,[SP, #+0]
        MOVS     R0,#+2
        STRB     R0,[SP, #+8]
        B.N      ??console_init_3
??console_init_2:
        MOVS     R0,#+3
        STRH     R0,[SP, #+0]
        MOVS     R0,#+3
        STRB     R0,[SP, #+8]
??console_init_3:
        MOVS     R0,#+1
        STRB     R0,[SP, #+9]
        MOVS     R0,#+0
        STRB     R0,[SP, #+2]
        MOVS     R0,#+1
        STRB     R0,[SP, #+10]
        MOVS     R0,#+1
        STRB     R0,[SP, #+7]
        MOVS     R0,#+0
        STRB     R0,[SP, #+6]
        MOVS     R0,#+0
        STRB     R0,[SP, #+5]
        MOVS     R0,#+0
        STRB     R0,[SP, #+4]
        MOVS     R0,#+1
        STRB     R0,[SP, #+3]
        LDR.W    R0,??DataTable13_8  ;; 0x40030000
        CMP      R4,R0
        BNE.N    ??console_init_4
        MOV      R1,SP
        LDR.W    R0,??DataTable13_9  ;; 0x400b0000
        BL       PORT_Init
        B.N      ??console_init_5
??console_init_4:
        MOV      R1,SP
        LDR.W    R0,??DataTable13_10  ;; 0x400e8000
        BL       PORT_Init
??console_init_5:
        MOVS     R1,#+1
        MOVS     R0,R4
        BL       UART_Cmd
        MOVS     R2,#+1
        MOVS     R1,#+16
        MOVS     R0,R4
        BL       UART_ITConfig
        LDR.W    R0,??DataTable13_8  ;; 0x40030000
        CMP      R4,R0
        BNE.N    ??console_init_6
        MOVS     R0,#+7
        BL       NVIC_DisableIRQ
        MOVS     R0,#+6
        BL       NVIC_EnableIRQ
        B.N      ??console_init_7
??console_init_6:
        MOVS     R0,#+6
        BL       NVIC_DisableIRQ
        MOVS     R0,#+7
        BL       NVIC_EnableIRQ
??console_init_7:
        ADD      SP,SP,#+28
        POP      {R4,R5,PC}       ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
putk:
        PUSH     {R7,LR}
        LDR.W    R1,??DataTable13_11
        LDRB     R1,[R1, #+0]
        CMP      R1,#+0
        BNE.N    ??putk_0
        CMP      R0,#+0
        BEQ.N    ??putk_1
??putk_2:
        LDR.W    R1,??DataTable13_3
        LDRH     R1,[R1, #+2]
        ADDS     R1,R1,#+1
        AND      R1,R1,#0xFF
        LDR.W    R2,??DataTable13_3
        LDRH     R2,[R2, #+0]
        CMP      R1,R2
        BEQ.N    ??putk_2
        LDR.W    R1,??DataTable13_3
        LDR.W    R2,??DataTable13_3
        LDRH     R2,[R2, #+2]
        ADD      R1,R1,R2
        STRB     R0,[R1, #+4]
        LDR.W    R0,??DataTable13_3
        LDRH     R0,[R0, #+2]
        ADDS     R0,R0,#+1
        AND      R0,R0,#0xFF
        LDR.W    R1,??DataTable13_3
        STRH     R0,[R1, #+2]
        LDR.W    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR      R0,[R0, #+56]
        LSLS     R0,R0,#+26
        BMI.N    ??putk_3
        LDR.N    R0,??DataTable13_3
        LDRH     R1,[R0, #+0]
        LDR.N    R0,??DataTable13_3
        UXTH     R1,R1            ;; ZeroExt  R1,R1,#+16,#+16
        ADD      R0,R0,R1
        LDRB     R0,[R0, #+4]
        ADDS     R1,R1,#+1
        AND      R1,R1,#0xFF
        LDR.N    R2,??DataTable13_3
        STRH     R1,[R2, #+0]
        UXTB     R0,R0            ;; ZeroExt  R0,R0,#+24,#+24
        LDR.N    R1,??DataTable13_4
        LDR      R1,[R1, #+0]
        STR      R0,[R1, #+0]
        LDR.N    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR      R0,[R0, #+56]
        ORRS     R0,R0,#0x20
        LDR.N    R1,??DataTable13_4
        LDR      R1,[R1, #+0]
        STR      R0,[R1, #+56]
        B.N      ??putk_3
??putk_0:
        CMP      R0,#+0
        BNE.N    ??putk_4
??putk_5:
        LDR.N    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR      R0,[R0, #+24]
        LSLS     R0,R0,#+24
        BPL.N    ??putk_5
        B.N      ??putk_1
??putk_4:
        LDR.N    R1,??DataTable13_4
        LDR      R1,[R1, #+0]
        LDR      R1,[R1, #+24]
        LSLS     R1,R1,#+26
        BMI.N    ??putk_4
        LDR.N    R1,??DataTable13_4
        LDR      R1,[R1, #+0]
        STR      R0,[R1, #+0]
        CMP      R0,#+10
        BNE.N    ??putk_3
        MOVS     R0,#+13
        BL       putk
??putk_3:
??putk_1:
        POP      {R0,PC}          ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
printf:
        PUSH     {R1-R3}
        PUSH     {R0-R11,LR}
        MOVS     R5,R0
        ADD      R8,SP,#+52
        B.N      ??printf_0
??printf_1:
        MOVS     R0,R4
        BL       putk
        CMP      R4,#+10
        BNE.N    ??printf_2
        MOVS     R0,#+13
        BL       putk
??printf_2:
??printf_0:
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
        CMP      R4,#+0
        BEQ.N    ??printf_3
        CMP      R4,#+37
        BNE.N    ??printf_1
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
        MOVS     R6,#+1
        CMP      R4,#+45
        BNE.N    ??printf_4
        MOVS     R6,#+0
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
??printf_4:
        MOVS     R0,#+32
        STR      R0,[SP, #+0]
        CMP      R4,#+48
        BNE.N    ??printf_5
        MOVS     R0,#+48
        STR      R0,[SP, #+0]
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
??printf_5:
        MOVS     R7,#+0
        CMP      R4,#+42
        BNE.N    ??printf_6
        LDR      R7,[R8, #+0]
        ADDS     R8,R8,#+4
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
        B.N      ??printf_7
??printf_6:
        MOVS     R0,R4
        BL       isdigit
        CMP      R0,#+0
        BEQ.N    ??printf_7
??printf_8:
        MOVS     R0,#+10
        MLA      R0,R0,R7,R4
        SUBS     R7,R0,#+48
        LDRB     R0,[R5, #+0]
        ADDS     R5,R5,#+1
        MOVS     R4,R0
        BL       isdigit
        CMP      R0,#+0
        BNE.N    ??printf_8
??printf_7:
        MVNS     R10,#-2147483648
        CMP      R4,#+46
        BNE.N    ??printf_9
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
        CMP      R4,#+42
        BNE.N    ??printf_10
        LDR      R10,[R8, #+0]
        ADDS     R8,R8,#+4
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
        B.N      ??printf_9
??printf_10:
        MOVS     R0,R4
        BL       isdigit
        CMP      R0,#+0
        BEQ.N    ??printf_9
        MOVS     R10,#+0
??printf_11:
        MOVS     R0,#+10
        MLA      R0,R0,R10,R4
        SUBS     R10,R0,#+48
        LDRB     R0,[R5, #+0]
        ADDS     R5,R5,#+1
        MOVS     R4,R0
        BL       isdigit
        CMP      R0,#+0
        BNE.N    ??printf_11
??printf_9:
        LDR.N    R1,??DataTable13_12
        MOVS     R9,#+0
        MOVS     R0,#+10
        MOVS     R2,#+1
        CMP      R4,#+108
        BEQ.N    ??printf_12
        CMP      R4,#+76
        BNE.N    ??printf_13
??printf_12:
        MOVS     R2,#+0
        LDRB     R4,[R5, #+0]
        ADDS     R5,R5,#+1
??printf_13:
        CMP      R4,#+0
        BNE.N    ??printf_14
??printf_3:
        MOVS     R0,#+0
        BL       putk
        MOVS     R0,#+0
        ADD      SP,SP,#+16
        POP      {R4-R11}
        LDR      PC,[SP], #+16    ;; return
??printf_14:
        MOVS     R3,R4
        CMP      R3,#+37
        BEQ.N    ??printf_15
        CMP      R3,#+88
        BEQ.N    ??printf_16
        CMP      R3,#+99
        BEQ.N    ??printf_17
        CMP      R3,#+100
        BEQ.N    ??printf_18
        CMP      R3,#+111
        BEQ.N    ??printf_19
        CMP      R3,#+112
        BEQ.N    ??printf_20
        CMP      R3,#+115
        BEQ.N    ??printf_21
        CMP      R3,#+117
        BEQ.N    ??printf_22
        CMP      R3,#+120
        BEQ.N    ??printf_23
        B.N      ??printf_24
??printf_18:
        UXTB     R2,R2            ;; ZeroExt  R2,R2,#+24,#+24
        CMP      R2,#+0
        BNE.N    ??printf_25
        LDR      R9,[R8, #+0]
        ADDS     R8,R8,#+4
        B.N      ??printf_26
??printf_25:
        LDR      R9,[R8, #+0]
        ADDS     R8,R8,#+4
??printf_26:
        CMP      R9,#+0
        BPL.N    ??printf_27
        RSBS     R2,R9,#+0
        B.N      ??printf_28
??printf_27:
        MOV      R2,R9
??printf_28:
        B.N      ??printf_29
??printf_19:
        MOVS     R0,#+8
        B.N      ??printf_22
??printf_20:
??printf_16:
        LDR.N    R1,??DataTable13_13
??printf_23:
        MOVS     R0,#+16
??printf_22:
        UXTB     R2,R2            ;; ZeroExt  R2,R2,#+24,#+24
        CMP      R2,#+0
        BNE.N    ??printf_30
        LDR      R2,[R8, #+0]
        ADDS     R8,R8,#+4
        B.N      ??printf_29
??printf_30:
        LDR      R2,[R8, #+0]
        ADDS     R8,R8,#+4
??printf_29:
        ADD      R4,SP,#+15
        MOVS     R3,#+0
        STRB     R3,[R4, #+0]
??printf_31:
        SUBS     R4,R4,#+1
        UDIV     R3,R2,R0
        MLS      R3,R0,R3,R2
        LDRB     R3,[R1, R3]
        STRB     R3,[R4, #+0]
        UDIV     R2,R2,R0
        CMP      R2,#+0
        BNE.N    ??printf_31
        B.N      ??printf_32
??printf_17:
        ADD      R4,SP,#+4
        LDR      R0,[R8, #+0]
        ADDS     R8,R8,#+4
        STRB     R0,[R4, #+0]
        MOVS     R11,#+1
        B.N      ??printf_33
??printf_15:
        ADD      R4,SP,#+4
        MOVS     R0,#+37
        STRB     R0,[R4, #+0]
        MOVS     R11,#+1
        B.N      ??printf_33
??printf_21:
        LDR      R4,[R8, #+0]
        ADDS     R8,R8,#+4
??printf_32:
        MOVS     R11,#+0
        B.N      ??printf_34
??printf_35:
        ADDS     R11,R11,#+1
??printf_34:
        LDRB     R0,[R4, R11]
        CMP      R0,#+0
        BEQ.N    ??printf_33
        CMP      R11,R10
        BLT.N    ??printf_35
??printf_33:
        SUBS     R7,R7,R11
        CMP      R9,#+0
        BPL.N    ??printf_36
        SUBS     R7,R7,#+1
??printf_36:
        LDR      R0,[SP, #+0]
        CMP      R0,#+48
        BNE.N    ??printf_37
        CMP      R9,#+0
        BPL.N    ??printf_37
        MOVS     R0,#+45
        BL       putk
??printf_37:
        UXTB     R6,R6            ;; ZeroExt  R6,R6,#+24,#+24
        CMP      R6,#+1
        BNE.N    ??printf_38
        B.N      ??printf_39
??printf_40:
        LDR      R0,[SP, #+0]
        BL       putk
        SUBS     R7,R7,#+1
??printf_39:
        CMP      R7,#+1
        BGE.N    ??printf_40
??printf_38:
        LDR      R0,[SP, #+0]
        CMP      R0,#+32
        BNE.N    ??printf_41
        CMP      R9,#+0
        BPL.N    ??printf_41
        MOVS     R0,#+45
        BL       putk
        B.N      ??printf_41
??printf_42:
        LDRB     R0,[R4, #+0]
        BL       putk
        ADDS     R4,R4,#+1
        SUBS     R11,R11,#+1
??printf_41:
        CMP      R11,#+1
        BGE.N    ??printf_42
??printf_43:
        CMP      R7,#+1
        BLT.N    ??printf_44
        LDR      R0,[SP, #+0]
        BL       putk
        SUBS     R7,R7,#+1
        B.N      ??printf_43
??printf_44:
        B.N      ??printf_0
??printf_24:
        MOVS     R0,#+37
        BL       putk
        MOVS     R0,R4
        BL       putk
        B.N      ??printf_0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_0:
        DC8 "MDR_UART1"
        DC8 0, 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_1:
        DC8 "MDR_UART2"
        DC8 0, 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_2:
        DC8 "\015\012Hard Fault\015\012"
        DC8 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_3:
        DC8 "\015\012Memory Management Fault\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_4:
        DC8 "\015\012Bus Fault\015\012"
        DC8 0, 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_5:
        DC8 "\015\012Usage Fault\015\012"

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
?_6:
        DC8 "\015\012Unknown Fault\015\012"
        DC8 0, 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`printf::X2C_tab`:
        DC8 "0123456789ABCDEF"
        DC8 0, 0, 0

        SECTION `.rodata`:CONST:REORDER:NOROOT(2)
        DATA
`printf::x2c_tab`:
        DC8 "0123456789abcdef"
        DC8 0, 0, 0

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
GetConsoleName:
        LDR.N    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        LDR.N    R1,??DataTable13_8  ;; 0x40030000
        CMP      R0,R1
        BNE.N    ??GetConsoleName_0
        LDR.N    R0,??DataTable13_14
        B.N      ??GetConsoleName_1
??GetConsoleName_0:
        LDR.N    R0,??DataTable13_15
??GetConsoleName_1:
        BX       LR               ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
console_set_poll_mode:
        PUSH     {R7,LR}
        MOVS     R0,#+15
        LDR.N    R1,??DataTable13_11
        STRB     R0,[R1, #+0]
        MOVS     R2,#+0
        MOVS     R1,#+32
        LDR.N    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        BL       UART_ITConfig
        POP      {R0,PC}          ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
dbg_put:
        PUSH     {R4,LR}
        MOVS     R4,R0
        MOVS     R0,#+15
        LDR.N    R1,??DataTable13_11
        STRB     R0,[R1, #+0]
        MOVS     R2,#+0
        MOVS     R1,#+32
        LDR.N    R0,??DataTable13_4
        LDR      R0,[R0, #+0]
        BL       UART_ITConfig
        UXTB     R4,R4            ;; ZeroExt  R4,R4,#+24,#+24
        CMP      R4,#+3
        BEQ.N    ??dbg_put_0
        BCC.N    ??dbg_put_1
        CMP      R4,#+5
        BEQ.N    ??dbg_put_2
        BCC.N    ??dbg_put_3
        CMP      R4,#+6
        BEQ.N    ??dbg_put_4
        B.N      ??dbg_put_1
??dbg_put_0:
        LDR.N    R0,??DataTable13_16
        BL       printf
        B.N      ??dbg_put_5
??dbg_put_3:
        LDR.N    R0,??DataTable13_17
        BL       printf
        B.N      ??dbg_put_5
??dbg_put_2:
        LDR.N    R0,??DataTable13_18
        BL       printf
        B.N      ??dbg_put_5
??dbg_put_4:
        LDR.N    R0,??DataTable13_19
        BL       printf
        B.N      ??dbg_put_5
??dbg_put_1:
        LDR.N    R0,??DataTable13_20
        BL       printf
??dbg_put_5:
        BL       hard_reset
        POP      {R4,PC}          ;; return

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13:
        DC32     0xe000e100

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_1:
        DC32     0xe000e180

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_2:
        DC32     rxirq_count

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_3:
        DC32     console

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_4:
        DC32     CON_UART

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_5:
        DC32     0x7a1200

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_6:
        DC32     0x4002000c

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_7:
        DC32     0x40020004

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_8:
        DC32     0x40030000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_9:
        DC32     0x400b0000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_10:
        DC32     0x400e8000

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_11:
        DC32     dbg_print_mode

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_12:
        DC32     `printf::x2c_tab`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_13:
        DC32     `printf::X2C_tab`

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_14:
        DC32     ?_0

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_15:
        DC32     ?_1

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_16:
        DC32     ?_2

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_17:
        DC32     ?_3

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_18:
        DC32     ?_4

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_19:
        DC32     ?_5

        SECTION `.text`:CODE:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
??DataTable13_20:
        DC32     ?_6

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
getc:
        PUSH     {R7,LR}
??getc_0:
        BL       getch
        CMP      R0,#+0
        BMI.N    ??getc_0
        POP      {R1,PC}          ;; return

        SECTION `.text`:CODE:NOROOT(1)
        THUMB
__gets:
        PUSH     {R4-R8,LR}
        MOVS     R4,R0
        MOVS     R5,R1
        MOV      R8,R5
        MOVS     R7,R4
        B.N      ??__gets_0
??__gets_1:
        CMP      R6,#+127
        BNE.N    ??__gets_2
        ADDS     R8,R8,#+1
        SUBS     R7,R7,#+1
        MOVS     R0,R6
        BL       putk
??__gets_0:
        CMP      R8,#+1
        BLT.N    ??__gets_3
        BL       getc
        MOVS     R6,R0
        CMP      R6,#+127
        BNE.N    ??__gets_4
        CMP      R8,R5
        BEQ.N    ??__gets_5
        ADDS     R8,R8,#+1
        SUBS     R7,R7,#+1
        MOVS     R0,R6
        BL       putk
??__gets_5:
        B.N      ??__gets_0
??__gets_4:
        MOVS     R0,R6
        BL       putk
        CMP      R6,#+13
        BNE.N    ??__gets_6
        MOVS     R0,#+10
        BL       putk
??__gets_6:
        STRB     R6,[R7, #+0]
        ADDS     R7,R7,#+1
        SUBS     R8,R8,#+1
        CMP      R6,#+10
        BEQ.N    ??__gets_7
        CMP      R6,#+13
        BNE.N    ??__gets_0
??__gets_7:
??__gets_3:
        CMP      R6,#+10
        BEQ.N    ??__gets_8
        CMP      R6,#+13
        BEQ.N    ??__gets_8
??__gets_2:
        BL       getc
        MOVS     R6,R0
        CMP      R6,#+10
        BEQ.N    ??__gets_9
        CMP      R6,#+13
        BNE.N    ??__gets_1
??__gets_9:
        MOVS     R0,#+10
        BL       putk
        MOVS     R0,#+13
        BL       putk
??__gets_8:
        MOVS     R0,#+0
        STRB     R0,[R7, #+0]
        MOVS     R0,R4
        POP      {R4-R8,PC}       ;; return

        SECTION `.iar_vfe_header`:DATA:NOALLOC:NOROOT(2)
        SECTION_TYPE SHT_PROGBITS, 0
        DATA
        DC32 0

        SECTION __DLIB_PERTHREAD:DATA:REORDER:NOROOT(0)
        SECTION_TYPE SHT_PROGBITS, 0

        SECTION __DLIB_PERTHREAD_init:DATA:REORDER:NOROOT(0)
        SECTION_TYPE SHT_PROGBITS, 0

        END
// 
//   533 bytes in section .bss
//   160 bytes in section .rodata
// 1 830 bytes in section .text
// 
// 1 830 bytes of CODE  memory
//   160 bytes of CONST memory
//   533 bytes of DATA  memory
//
//Errors: none
//Warnings: none
