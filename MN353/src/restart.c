
#include <MDR32Fx.h>
#include <mn353.h>
#include <types.h>

extern void console_set_poll_mode(void);
extern int printf(const char *fmt, ...);

const struct {
	int type;
	const char* desc;
	} rst_map[] = 
	{
		{ RST_T_U_ABORT,	"Undefined instruction"	},
		{ RST_T_D_ABORT,	"Data abort"		},
		{ RST_T_P_ABORT,	"Prefetch abort"	},
		{ RST_T_SWI,		"Unsupported SWI"	},
		{ RST_T_IRQ,		"Default IRQ"		},
		{ RST_T_TEST,		"User soft restart"	},
		{ RST_T_DISP,		"Dispatch"		},
		{ RST_T_SYS,    	"System failure"	},
		{ RST_T_RAND,		"RSG failure"		},
		{ RST_T_TIME,		"Timer"			},
		{ RST_T_HOST,		"Host"			},
		{ RST_T_PSSV,		"Power Supply"		},
                { RST_T_SOFT,           "Software error"        }

	};


void hard_reset(void)
{
    SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEYSTAT_Pos) |
                  (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |                 
                  SCB_AIRCR_SYSRESETREQ_Msk);   // Keep priority group unchanged             
    __DSB();    // Ensure completion of memory access
    while(1);   // wait until reset     
}



void restart(int type, char *param, unsigned long line)
{
    int i;
    
    console_set_poll_mode();
      
    for (i=0; i<sizeof(rst_map)/sizeof(rst_map[0]); i++) {
        if (rst_map[i].type == type) {
            if (param == 0)
                printf("\nRESTART: %s: %08X\n", rst_map[i].desc, line);
            else
                printf("\nRESTART: %s, %s: %d\n", rst_map[i].desc, param, line);
            break;
        }
    }
    if (i == (sizeof(rst_map)/sizeof(rst_map[0])))
         printf("\nRESTART: %d, L: %d, P: %X\n", type, line, param);	 
  
    hard_reset();
    /*
    SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEYSTAT_Pos) |
                  (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |                 
                  SCB_AIRCR_SYSRESETREQ_Msk);   // Keep priority group unchanged             
    __DSB();    // Ensure completion of memory access
    while(1);   // wait until reset   
    */
}
