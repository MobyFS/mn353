/**************************************************
 *
 * Code for handling system exit, thumb 2 version.
 *
 * Copyright 2006 IAR Systems. All rights reserved.
 *
 * $Revision: 95271 $
 *
 **************************************************/

;-----------------------------------------------------------------------------
; This file contains the exit code used by the ICCARM C compiler.
;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbols ?cstartup etc.
;
; All code in the modules will be placed in the ICODE segment,
;
; Cortex-M version
;
; $Revision: 95271 $
;
;-----------------------------------------------------------------------------

;
; Naming covention of labels in this file:
;
;  ?xxx   - External labels only accessed from assembler.
;  __xxx  - External labels accessed from or defined in C.
;  xxx    - Labels local to one module (note: this file contains
;           several modules).
;  main   - The starting point of the user program.
;



;---------------------------------------------------------------
; ?_EXIT
; main may return an exit code in R0, or _exit may be called with
; the exit code in R0.
; If the exit code is needed for som reason, R0 should be stored
; in e.g. one of the registers R4-R7, so that the value is
; preserved when calling __call_dtors and _Close_all.
;---------------------------------------------------------------

        MODULE  ?cexit

        AAPCS INTERWORK, VFP_COMPATIBLE, RWPI_COMPATIBLE, ROPI_ANY_COMPATIBLE				
		
        PUBLIC  _exit

        SECTION .text:CODE:NOROOT(2)
        REQUIRE ?jump_to_exit

        THUMB
_exit:
        MOV     R7,R0
        ;; Fall through to the next fragment



; ----------------------------------------
; The standard library REQUIRE:s the label __cstart_call_dtors, if
; needed.
;

        SECTION .text:CODE:NOROOT(1)
        PUBLIC  __cexit_call_dtors
        EXTWEAK  __call_dtors

        THUMB

__cexit_call_dtors:
          FUNCALL _exit, __call_dtors
        BL      __call_dtors

        ;; Fall through.


; ----------------------------------------
; The standard library REQUIRE:s the label __cstart_closeall, if
; needed.
;

        SECTION .text:CODE:NOROOT(1)
        PUBLIC  __cexit_closeall
        EXTWEAK  _Close_all

        THUMB

__cexit_closeall:
          FUNCALL _exit, _Close_all
        BL      _Close_all

        ;; Fall through.


; ----------------------------------------
; The standard library REQUIRE:s the label __cstart_clearlocks, if
; needed.
;

        SECTION .text:CODE:NOROOT(1)
        PUBLIC  __cexit_clearlocks
        EXTWEAK  __iar_clearlocks

        THUMB

__cexit_clearlocks:
          FUNCALL _exit, __iar_clearlocks
        BL      __iar_clearlocks

        ;; Fall through.


        EXTERN  __exit

        SECTION .text:CODE:NOROOT(1)

        THUMB
?jump_to_exit:
        MOV     R0,R7
          FUNCALL _exit, __exit
        BL      __exit
        B       ?jump_to_exit

        END
