
// Подкрутка otimer под таймеры FreeRTOS, чтобы сохранить текст звена

#include <FreeRTOS.h>
#include <types.h>
#include "otimer.h"
#include <stdarg.h>
#include <string.h>
#include <restart.h>

#define OTIMER_MAGIC 0x13774321

typedef struct
{
	u32_t magic;
        TimerHandle_t handle;
	timer_t id;
	o_timer_func_t func;
	u8_t count;
	void* prm[1];
}o_timer_cb_t;


// Статистика создания/удаления(остановки) таймеров
struct
{
	u32_t start;  // числ запущенных таймеров (создано структур TM_ITEM)
	u32_t stop;   // числ остановленных таймеров (удалено структур TM_ITEM)
	u32_t signal; // число сработанных таймеров
}timer_stat;


static void sigev_notify_function( TimerHandle_t pxTimer )
{
    // Optionally do something if the pxTimer parameter is NULL.
    configASSERT( pxTimer );
 
    // Which timer expired?
    o_timer_cb_t* t = (o_timer_cb_t*)pvTimerGetTimerID(pxTimer);
    
    t->func(t, t->count, &t->prm[0]);
}

// ОПИСАНИЕ: запустить таймер.
// АРГУМЕНТЫ:
// @ticks - число тиков до срабатывания таймера.
// ВОЗВРАЩАЕТ: хэндл таймера (после использования должна быть вызвана
// функция timer_stop).
o_timer_t otimer_start(tick_t ticks, o_timer_func_t func, u8_t count, ...)
{
	int rs;
        TimerHandle_t xTimers;
	o_timer_cb_t* t;
	va_list ap;

	va_start(ap, count);

	timer_stat.start++;

	t = tm_malloc(sizeof(o_timer_cb_t) + (count?count-1:0)*sizeof(void*));
	if (t == NULL) FATAL_ERROR();

	t->magic = OTIMER_MAGIC;

	t->func = func;

	// Сохраняем число аргументов
	t->count = count;
	for (rs=0; rs<count; rs++) 
            t->prm[rs] = va_arg(ap, void*);

	// Создаём таймер
        xTimers = xTimerCreate(
                   "TM",                 // Просто текст, не используется ядром.
                   ticks,                // Период таймера в тиках
                   pdFALSE,              // Перезагрузки не надо
                   (void *) t,           // Указатель на o_timer_cb_t.
                   sigev_notify_function // callback для всех один
                              );
        if (xTimers == NULL) {
            // Не получилось создать таймер
            putdbg('F');
            FATAL_ERROR();
        }
        else {
            t->handle = xTimers;
	    // Запускаем таймер
            putdbg('t');
            if (xTimerStart( xTimers, 0) != pdPASS ) {
                putdbg('f');
                // Не получилось перевести таймер в активное состояние
                FATAL_ERROR();
            }
        }
	va_end(ap);
        putdbg('+');
	return t;
}

// ОПИСАНИЕ: остановка таймер и удаление таймера. Каждый созданный таймер
// с помощью функции timer_start должен быть удалён с помощью данной функции.
// АРГУМЕНТЫ:
// @tm - останавливаемый таймер. Таймер может быть уже сработал или нет.
void otimer_stop(o_timer_t tm)
{
	o_timer_cb_t* const t = (o_timer_cb_t*)tm;

	if (tm == NULL) return;

	configASSERT(t->magic == OTIMER_MAGIC);

	timer_stat.stop++;
        
        if (xTimerDelete(t->handle,0) == pdFAIL)
            FATAL_ERROR();

	memset(t, 0, sizeof(o_timer_cb_t));

	tm_free(t);
        putdbg('-');
}


