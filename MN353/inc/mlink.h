#ifndef _LINK_H
#define _LINK_H

#include <FreeRTOS.h>
#include <otimer.h>
#include <olist.h>

#define MB 0x55
#define MH 0xAD
#define MK 0x92
#define MP 0x6A

typedef enum
{	// состояние звеньевого соединения с аппаратом 
	link_st_disable, // соединение отключено
	link_st_no_bi,   // Нет физического соединения
	link_st_down,    // Нет звеньевого соединения (устанавливаем)
	link_st_up       // есть звеньевое соединение
}link_st_t;


typedef enum
{
	link_st_tx_wait_c_ack, // ожидание ответа на команду-квитанцию
	link_st_tx_wait_d_ack, // ожидание ответа на команду-данные
	link_st_tx_wait_ping   // ожидание наступления момента передачи команды-квитанции
}link_st_tx_t;

// Состояние передатчика "команда-данные"
typedef enum
{
	link_d_st_free,      // нет сообщения для передачи
	link_d_st_wait_chan, // ожидание освобождения канала
	link_d_st_wait_ack,  // передача сообщения (ожидание квитанции)
	link_d_st_tx_ok,     // сообщение передано
	link_d_st_lost       // сообщение потеряно
}link_d_st_t;

typedef struct
{
	u16_t len;     // размер в байтах поля данных (без учёта КС и маркеров)
	u32_t magic;
	list_t list;
	u8_t addr : 4, // звеньевой адрес
		N : 1, // циклический номер
		C : 1, // бит команды (1) / ответа (0)
		B : 1, // бит занятости
		S : 1; // бит суффикса
	u8_t body[1];  // первый байт данных сообщения
} link_msg_t;

// Запрос на передачу сообщения в аппарат в единой очереди запросов.
typedef struct
{
	link_msg_t* msg;        // Сообщение, которое необходимо передать
	list_t list;            // Для включения в очередь (на основе списка)
	link_st_tx_t tx_st;     // Состояние передатчика после передачи сообщения
	struct link_t* link;    // Звеньевое соединение, к которому относится передаваемое сообщение
} link_tx_rq;

typedef struct link_t
{
	u32_t magic;
	// состояние звеньевого соединения с аппаратом
	link_st_t st;

	u8_t ca369; // !0 - режим "СА369"

	u8_t addr; // адрес

	struct
	{
		u8_t N : 1, // циклический номер на передачу
		     B : 1; // признак занятости корреспондента
	
		link_msg_t* current;  // команда-данные, подлежащая передачи
		link_st_tx_t st;      // состояние передатчика
		u8_t attempts;        // число выполненных попыток передачи
		link_d_st_t d_st;     // состояние передатчика "команда-данные"
	}tx;

	o_timer_t timer_ack;
	o_timer_t timer_ping;

	link_tx_rq tx_rq;
} link_t;



// Максимально допустимая длина кадра с учётом служебной области,
// данных и контрольной суммы
#define LINK_MAX_FRAME_SIZE (1+2048+2+16)

// Минимально допустимая длина кадра с учётом служебной области и
// контрольной суммы
#define LINK_MIN_FRAME_SIZE (1+0+2)

//! Структура для выделения кадра на приёме из потока байт.
typedef struct
{	//! состояние приёмного автомата
	enum
	{
		rx_st_idle,  // приём отключен
		rx_st_mb,    // ожидание певого байта маркера начала
		rx_st_mh,    // ожидание второго байта маркера начала
		rx_st_data,  // ожидание байта данных кадра
		rx_st_mk     // ожидание второго байтка маркера конца
	}st;

	u16_t len; // число полученных байт сообщения (число байт в буфере)

	u8_t buffer[LINK_MAX_FRAME_SIZE];
	
	////u8_t ca369;   // !0 - получен кадр с маркером начала соответствующему режиму СА369

	// Статистика ошибок
	u32_t ovf;    // число переполнений входного буфера
	u32_t trun;   // число ошибок: недопустимо короткий кадр
	u32_t mark;   // число ошибок выделения маркеров
	u32_t cs;     // число ошибок в контрольной сумме

}link_rx_machin_t;

extern void mlink_init(link_t* link, u8_t addr);
extern void link_start(link_t* link);
extern void link_stop(link_t* link);
extern void link_timeout(o_timer_t t, u8_t count, void** params);
extern void link_ph_layer_up(link_t* link);
extern void link_ph_layer_down(link_t* link);
extern int  link_put_msg(link_t* link, link_msg_t* msg);
extern int  mlink_rx_bytes(link_rx_machin_t* m, const u8_t* buffer, u32_t size, list_t* msg_list);
extern void link_rx_machin_init(link_rx_machin_t* m);
extern void link_rx_msg(link_t* link, link_msg_t* msg);
extern void link_check_for_tx(link_t* link);

extern link_msg_t* link_msg_new(u16_t size);
extern void link_msg_del(link_msg_t* msg);
extern link_msg_t* link_msg_clone(const link_msg_t* msg);

//void link_send(link_t* link, link_msg_t* msg);
void link_send(link_msg_t* msg);

// Функции, на которые ссылается модуль link.c
extern void link_callback_timeout(void* p1, void* p2);
extern void link_callback_st(link_t* link);
extern void link_callback_d_st(link_t* link, link_d_st_t st);
extern void link_callback_B(link_t* link, u8_t B);
extern void link_callback_rx(link_t* link, link_msg_t* msg);
extern void link_callback_tx_bytes(const u8_t* buffer, u32_t size);
extern void link_callback_check_for_tx(link_t* link);
#endif // _LINK_H

