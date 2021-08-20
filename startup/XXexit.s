/**************************************************
 *
 * __exit -- halts the system, non-debug version.
 *
 * Copyright 2006 IAR Systems. All rights reserved.
 *
 * $Revision: 95271 $
 *
 **************************************************/


        MODULE  ?__exit
        PUBLIC  __exit

        AAPCS INTERWORK, VFP_COMPATIBLE, RWPI_COMPATIBLE, ROPI_ANY_COMPATIBLE				
		
        SECTION .text:CODE:NOROOT(2)

        THUMB
__exit
          NOCALL __exit

        MOVS    r1,#0x20
        LSLS    r1,r1,#12
        ADDS    r1,r1,#0x26    ; was: LDR  r1,=0x20026
        MOVS    r0,#0x18
        BKPT    0xAB
        B       __exit

        LTORG

        END
