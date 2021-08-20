#ifndef _OTMER_H
#define _OTMER_H

#include "timers.h"

#define tm_malloc(s) pvPortMalloc(s)
#define tm_free(m)   vPortFree(m)

typedef int tick_t;
typedef void* o_timer_t;

typedef void (*o_timer_func_t)(o_timer_t, u8_t count, void** params);

extern o_timer_t otimer_start(tick_t ticks, o_timer_func_t func, u8_t count, ...);
extern void otimer_stop(o_timer_t tm);

#endif // _OTMER_H
