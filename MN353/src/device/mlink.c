
// Звено ведущего компонента

#include <types.h>
#include <app.h>
#include "mlink.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <restart.h>
#include <assert.h>

//#define assert ASSERT

// Идентификаторы таймеров
#define TIMER_ACK  1   // таймер ожидания ответа
#define TIMER_PING 2   // таймер ожидания момента передачи команды-квитанции

// таймаут ожидания ответа в мс (4.4 сек по протоколу)
#define TIMEOUT_ACK   (4400/portTICK_PERIOD_MS) 
// таймаут передачи команды-квитацнии в мс (по протоколу не реже 1 в 2 сек)
#define TIMEOUT_PING  (1500/portTICK_PERIOD_MS) 

// Максимальное число попыток передачи сообщения
#define MAX_ATTEMTPS 3

#define timer_start(timer, link, timer_id, timeout_value) \
        do { \
            if (timer != NULL) otimer_stop(timer);  \
            timer = otimer_start(timeout_value, link_timeout, 2, link, timer_id); \
        }while(0) 

#define timer_stop(timer) do { \
        if (timer)   \
        { \
            otimer_stop(timer);  \
            timer = NULL; \
        } \
}while(0)


#ifdef CHECK_MALLOC
void link_mem_init();
void* link_malloc(size_t size);
void link_free(void* mem);
#else
# define link_malloc(s) pvPortMalloc(s)
# define link_free(m)   vPortFree(m)
#endif // CHECK_MALLOC

#if 0

#define log(...) ukld_log("LINK", __VA_ARGS__)
#define dbg(...) ukld_dbg("LINK", __VA_ARGS__)
#define dbg_printf(...) ukld_dbg_printf(__VA_ARGS__)

#else

#define log(...)
#define dbg(...)
#define dbg_printf(...)

#endif

//////////////////////////////// прототипы /////////////////////////////////////
static void link_tx_c(link_t* link);
static void link_cancel_rq_tx(link_t* link);
static void link_timeout(o_timer_t t, u8_t count, void** params);

static struct
{
    struct
    {
        list_t q;       // очередь запросов на передачу
        enum {
            g_tx_st_idle,
            g_tx_st_busy
        } st;           // состояние передатчика
        link_t* link;
    }tx;
} g_link =
{
    .tx = {
        .q  = {&g_link.tx.q, &g_link.tx.q},
        .st = g_tx_st_idle
    }
};


//! Инициализировать структуру link_t.
//!@param link Указатель на структуру, которую необходимо инициализировать.
//!@addr Адрес.
void mlink_init(link_t* link, u8_t addr)
{
    memset(link, 0, sizeof(link_t));
    link->magic = LINK_MAGIC;
    link->st    = link_st_disable;
    link->addr  = addr;     
    link->tx.current = NULL;
    link->tx.d_st = link_d_st_free;
    link->tx_rq.msg = NULL;
    list_init(&link->tx_rq.list);
    link->tx_rq.tx_st = link_st_tx_wait_c_ack;
    link->tx_rq.link = link;
}

//! Перевести соединение в новое состояние, с оповещением
//! верхнего уровня о смене состояния.
//!@param link Соединение, которое необходимо перевести в новое состояние.
//!@param st Новое состояние соединения.
static void link_st(link_t* link, link_st_t st)
{
    if (link->st != st) {
        // Состояние соединения изменилось, оповещаем верхний уровень.
        link->st = st;
        link_callback_st(link);
    }
}

//! Установить новое состояние передатчика информационных сообщений и
//! оповестить верхний уровень о смене состояния.
static void link_d_st(link_t* link, link_d_st_t st)
{
    if (link->tx.d_st != st) {
        // Состояние соединения изменилось, оповещаем верхний уровень.
        link->tx.d_st = st;
        link_callback_d_st(link, st);
    }
}

//! Установить признак занятости местного аппарата.
static void link_B(link_t* link, u8_t B)
{
    if (link->tx.B != B) {
        link->tx.B = B;
        link_callback_B(link, B);
    }
}

//! Перейти к процедуре установления соединения.
void link_start(link_t* link)
{
    assert(link->magic == LINK_MAGIC);

    link_st(link, link_st_no_bi);
}

//! Прекратить работу по каналу.
void link_stop(link_t* link)
{
    assert(link->magic == LINK_MAGIC);

    if (link->st == link_st_disable) return;

    // останавливаем все таймеры
    timer_stop(link->timer_ack);
    timer_stop(link->timer_ping);
    
    // Удаляем сообщения в буфере на передачу
    if (link->tx.current) {
        assert(link->tx.current->magic == MSG_MAGIC);
        list_remove(&link->tx.current->list);
        link_msg_del(link->tx.current);
        link->tx.current = NULL;
        link->tx.d_st = link_d_st_free;
    }

    // Если в общей очереди передачи находится запрос на передачу от
    // нашего соединения, то удаляем запрос из очереди.
    link_cancel_rq_tx(link);
    if (g_link.tx.link == link) {
        // Общий передатчик занят передачей нашего сообщения, отменяем передачу - 
        // ставим признак "передатчик свободен" и переходим к передаче сообщения
        // от следующего канала.
        g_link.tx.st = g_tx_st_idle;
        g_link.tx.link = NULL;
        link_callback_check_for_tx(link);
    }
    
    // Устанавливаем состояние канала "отключен"
    link_st(link, link_st_disable);
}

//! Оповестить модуль поддержики звеньевого соединения о
//! появлении связи на физическом уровне.
void link_ph_layer_up(link_t* link)
{
    assert(link->magic == LINK_MAGIC);
    // Соединение на физическом уровне может появиться только, если
    // его раньше не было.

    if (link->st != link_st_no_bi) return;
    
    // Соединение на физическом уровне у нас есть, теперь можем попытаться
    // установить соединение на звеньевом уровне.
    link->tx.N = 0;
    link->tx.B = 1; // удалённый корреспондент занят
    link->tx.st = link_st_tx_wait_ping;

    // Передаём команду квитанцию
    link_tx_c(link);

    // Переводим соединение в состояние "установления соединения"
    link_st(link, link_st_down);
}

//! Известить модуль поддержки звеньевого соединения о потере физического соединения.
void link_ph_layer_down(link_t* link)
{
    assert(link->magic == LINK_MAGIC);
    
    if (link->st == link_st_disable || link->st == link_st_no_bi) return;

    timer_stop(link->timer_ack);
    timer_stop(link->timer_ping);

    // Удаляем сообщения в буфере на передачу
    if (link->tx.current) {
        assert(link->tx.current->magic == MSG_MAGIC);
        list_remove(&link->tx.current->list);
        link_msg_del(link->tx.current);
        link->tx.current = NULL;
        link->tx.d_st = link_d_st_free;
    }

    link_cancel_rq_tx(link);

    link_st(link, link_st_no_bi);
}

//! Проверить КС (КС - последние 2 байта).
//!@return Возвращает 0 - успех, иначе ошибка КС.
/*static*/ int link_check_ks(const void* data, u16_t len)
{
    int i, c0 = 0, c1 = 0;
    const u8_t* const ptr = (u8_t*)data;
    for (i=0; i<len; i++)
    {
            c0 = (c0 + ptr[i]) % 255;
            c1 = (c1 + c0) % 255;
    }
    return ((c0 == 0) && (c1 == 0)) ? 0 : 0x0F;
}

//! Инициализировать струкруту, содержащую конечный автомат, необходимый
//! для выделения кадра из байтового потока физической линии.
//!@param m Указатель на структуру, которую необходимо инициализировать.
void link_rx_machin_init(link_rx_machin_t* m)
{
    memset(m, 0, sizeof(link_rx_machin_t));
    m->st = rx_st_mb;
}

//! Обработать байты данных, полученные из физической линни связи.
//!@param m      Служебная структура, содержащая данные конечного автомата, 
//!              используемого для выделения кадра из потока байт.
//!@param buffer Полученные из физической линии байты данных.
//!@param size   Число байт в массиве buffer.
//!@param[in,out] msg_list Список, в который необходимо поместить полученные
//!                        кадры (сообщения).
//!@return Число полученных сообщений и помещённых в список msg_list.
int mlink_rx_bytes(link_rx_machin_t* m, const u8_t* buffer, u32_t size, list_t* msg_list)
{
    u32_t i;
    int msg_count = 0;
    
    // Формат кадра:
    // <B><H><SLUG><данные><KS0><KS1><B><K>
    // Вместо байта <H> для кадров квитанций может быть байт <P>

    for (i=0; i<size; i++)
    {
        const u8_t byte = buffer[i];

        switch (m->st)
        {
        case rx_st_idle:
                return msg_count;

        case rx_st_mb:
                if (byte != MB) continue;
                m->st = rx_st_mh;
                continue;

        case rx_st_mh:
                if (byte == MB) continue;
                if ((byte == MH) || (byte == MP)) {
    begin:          // получен маркер начала
                    m->st = rx_st_data;
                    m->len = 0;
                }
                else
                    m->st = rx_st_mb;
                continue;

        case rx_st_data:
                if (byte == MB) {
                    // первый байт маркера конца, начало следующего пакета, 
                    // либо байтстафинг
                    m->st = rx_st_mk;
                    continue;
                }
        put_byte:
                if (m->len == LINK_MAX_FRAME_SIZE) {
                    // полученный кадр слишком велик
                    m->ovf++;
                    m->st = rx_st_mb;
                    continue;
                }
                m->buffer[m->len++] = byte;
                continue;

        case rx_st_mk:
                if (byte == MB) {
                    // байт стафинг
                    m->st = rx_st_data;
                    goto put_byte;
                }
                if ((byte == MH) || (byte == MP)) {
                    // получили маркер начала, отбрасываем текущий кадр
                    goto begin;
                }
                
                m->st = rx_st_mb;

                if (byte == MK) {
                    // получили маркер конца
                    link_msg_t* msg;
                    //log("<ВК>\n");
                    // Обработка полученного сообщения
                    m->st = rx_st_mb;
                    // Проверка кадра на минимальный размер 
                    if (m->len < LINK_MIN_FRAME_SIZE) {
                        // Недопустимо короткий кадр
                        m->trun++;
                        continue;
                    }
                    // Проверка контрольной суммы
                    if (link_check_ks(m->buffer, m->len)) {
                        // Ошибка в КС
                        m->cs++;
                        for (int k=0; k<m->len; k++) printf("%02X ",m->buffer[k]);
                        printf("\r\n");
                        continue;
                    }
                    // Длина и контрольная сумма кадра корректны, строим из него
                    // сообщение звеньевого уровня.
                    msg = link_msg_new(m->len - (1 + 2)); // служебная область + КС
                    // Тело сообщения
                    memcpy(msg->body, &m->buffer[1], msg->len);
                    
                    // Поля из служебной области
                    msg->addr = m->buffer[0] & 0xF;
                    msg->S = (m->buffer[0] & (1 << 4)) ? 1 : 0;
                    msg->B = (m->buffer[0] & (1 << 5)) ? 1 : 0;
                    msg->C = (m->buffer[0] & (1 << 6)) ? 1 : 0;
                    msg->N = (m->buffer[0] & (1 << 7)) ? 1 : 0;
                    
                    dbg("RX MSG, A = %d, S = %d, B = %d, C = %d, N = %d, len = %d\n",
                            msg->addr, msg->S, msg->B, msg->C, msg->N, msg->len);
                    
                    if (msg->len != 0) {
                        int j;
                        dbg_printf("RXD = ");
                        for (j=0; j<msg->len; j++) {
                            dbg_printf("%02X ", msg->body[j]);
                        }
                        dbg_printf("\n");
                    }
                    // Добавляем сообщение в список принятых сообщений
                    list_add(msg_list, &msg->list);
                    // Инкрементируем число добавленных сообщений в список
                    msg_count++;
                }
                else
                    m->mark++; // ошибка выделения маркера конца
                continue;
        }
    }

    return msg_count;
}

u16_t fletcher_put(u16_t ks, u8_t byte)
{
    union
    {
        u16_t w;
        struct
        {
           u8_t c0;
           u8_t c1;
        }b;
    }u, tmp;

    u.w = ks;

    tmp.w = u.b.c0 + byte;
    
    if (tmp.b.c1) tmp.b.c0++;
    
    if (tmp.b.c0 == 0xFF)
            u.b.c0 = 0;
    else
            u.b.c0 = tmp.b.c0;
    
    tmp.w = u.b.c1 + u.b.c0;
            
    if (tmp.b.c1) tmp.b.c0++;

    if (tmp.b.c0 == 0xFF) 
            u.b.c1 = 0;
    else
            u.b.c1 = tmp.b.c0;

    return u.w;
}


u16_t fletcher_make(u16_t ks)
{
    u16_t w;
    u8_t result_c0, result_c1;
    u8_t c0, c1;

    c0 = (u8_t)ks;
    c1 = (u8_t)(ks >> 8);

    // ks0 = c0 - c1
    if (c1 > c0) w = c0 + 0xFF;
    else w = c0;
    
    w -= c1; // w := c0 - c1

    if (w == 0xFF) w = 0;

    result_c0 = (u8_t)w;

    
    // ks1 = c1 - 2*c0
    w = c0 << 1; //2*c0
    
    if (w >= 0xFF) w -= 0xFF;
    
    result_c1 = c1;

    if (c1 < w) result_c1 += 0xFF;

    result_c1 -= w; // result_c1 := c1 - 2*c0

    if (result_c1 == 0xFF) result_c1 = 0;

    return (result_c1 << 8) | result_c0;
}

//! Передать сообщение в канал.
//!@msg Сообщение, подлежащее передаче.
void link_send(link_msg_t* msg)
{
    u8_t* ptr;
    u8_t byte;
    u16_t size, i;
    u16_t ks = 0;

    dbg("TX MSG, A = %d, S = %d, B = %d, C = %d, N = %d, len = %d\n",
            msg->addr, msg->S, msg->B, msg->C, msg->N, msg->len);

    if (msg->len != 0) {
        int j;
        dbg_printf("TXD = ");
        for (j=0; j<msg->len; j++) {
                dbg_printf("%02X ", msg->body[j]);
        }
        dbg_printf("\n");
    }

    ptr = link_malloc(1 + 2 + (1+1+msg->len+2)*2 + 2);
    if (ptr == NULL) FATAL_ERROR();

    ptr[0] = MK; // Символ К
    ptr[1] = MB; // Маркер начала

    if (msg->len == 0)
        ptr[2] = MP; // кадр-квитанция
    else
        ptr[2] = MH; // кадр-данные

    // Служебная область
    byte = msg->addr & 0xF;
    if (msg->len && msg->S) byte |= (1 << 4);
    if (msg->B) byte |= (1 << 5);
    if (msg->C) byte |= (1 << 6);
    if (msg->N) byte |= (1 << 7);
    ptr[3] = byte;
    size = 4;
    if (byte == MB) ptr[size++] = MB;
    ks = fletcher_put(ks, byte);

    // Данные
    for (i=0; i<msg->len; i++) {
        byte = msg->body[i];
        ptr[size++] = byte;
        if (byte == MB) ptr[size++] = MB;
        ks = fletcher_put(ks, byte);
    }
    // Контрольная сумма
    ks = fletcher_put(ks, 0);
    ks = fletcher_put(ks, 0);
    ks = fletcher_make(ks);

    byte = (u8_t)ks;
    ptr[size++] = byte;
    if (byte == MB) ptr[size++] = MB;
    byte = (u8_t)(ks >> 8);
    ptr[size++] = byte;
    if (byte == MB) ptr[size++] = MB;

    // маркер конца
    ptr[size++] = MB;
    ptr[size++] = MK;

    // Передача байтов кадра в физическую линию
    link_callback_tx_bytes(ptr, size);

    link_free(ptr);
}

// ОПИСАНИЕ: поставить сообщение в единую очередь сообщений на передачу в аппарат.
// АРГУМЕНТЫ:
// @link - звеньевое соединение, к которому относиться сообщение;
// @msg - сообщение подлежащее передачи;
// @tx_st - состояние передатчика, после передачи данного сообщение (признак
// того что передаём: команду-квитанцию или команду-данные);
static void link_want_tx(link_t* link, link_msg_t* msg, link_st_tx_t tx_st)
{
    // Проверяем, что структура tx_rq не используется.
    assert(list_is_empty(&link->tx_rq.list));

    link->tx_rq.msg = msg;
    link->tx_rq.tx_st = tx_st;

    if (msg->addr == 0)
            list_add_head(&g_link.tx.q, &link->tx_rq.list);
    else
            list_add(&g_link.tx.q, &link->tx_rq.list);
    
    // Проверяем можно ли передать сообщение из очереди.
    link_check_for_tx(link);
}

// ОПИСАНИЕ: отменить запрос на передачу.
// АРГУМЕНТЫ:
// @link звеньевое соединение, запрос на передачу сообщение через которое необходимо
// отменить.
static void link_cancel_rq_tx(link_t* link)
{       
    if (list_is_empty(&link->tx_rq.list)) 
            return; // От данного соединения в очереди запросов на передачу запроса нет.

    // Удаляем запрос на передачу
    list_remove(&link->tx_rq.list);

    // Удаляем не переданное сообщение (только команду-квитанцию, команда-данные находится
    // в очереди и будет удалена при очищении очереди или при успешной передачи сообщения)
    if (link->tx_rq.tx_st == link_st_tx_wait_c_ack && link->tx_rq.msg)
            link_msg_del(link->tx_rq.msg);
    link->tx_rq.msg = NULL;
}


// ОПИСАНИЕ: проверить можно ли передать сообщение из единой очереди сообщений
// на передачу.
void link_check_for_tx(link_t* link)
{
    link_tx_rq* rq;

    if (g_link.tx.st != g_tx_st_idle) { 
        // передатчик занят
        putdbg('*'); 
        return; 
    } 

    if (list_is_empty(&g_link.tx.q)) { 
        // нет запросов на передачу
        putdbg('.'); 
        return; 
    } 

    // Извлекаем из очереди запросов на передачу, первый из них, тоесть тот,
    // который поступил раньше всех.
    rq = list_entry(g_link.tx.q.next, link_tx_rq);
    list_remove(&rq->list);

    // Передача сообщения
    link_send(rq->msg);
    putdbg('S');
    // Запус таймера ожидания квитанции
    timer_start(rq->link->timer_ack, rq->link, TIMER_ACK, TIMEOUT_ACK);
    // Переводим передатчик в состояние "занят" (ожидания ответа)
    g_link.tx.st = g_tx_st_busy;
    g_link.tx.link = rq->link;   // сохраняем указатель на соединение, передачей
                                 // сообщения которого мы заняты в настоящий момент

    // Удаляем переданное сообщение, если оно больше не нужно.
    if (rq->tx_st == link_st_tx_wait_c_ack) 
        link_msg_del(rq->msg);
    else {
        // Передали в канал команду-данные - новое состояние сообщения: 
        // ожидание квитанции от местного аппарата.
        link_d_st(rq->link, link_d_st_wait_ack);
    }
    // Удаляем ссылку на сообщение (на всякий случай).
    rq->msg = NULL;
}

//! Выполнить очередную попытку передачи первого сообщения из очереди
//! сообщений на передачу.
static void link_tx_d(link_t* link)
{
    link_msg_t* msg;

    // Получаем указатель на первое сообщение в очереди.
    assert(link->tx.current && link->tx.current->magic == MSG_MAGIC);

    msg = link->tx.current;
    msg->N = link->tx.N;
    msg->C = 1;
    msg->B = 0; // я никогда не занят
    msg->addr = link->addr;

    link->tx.st = link_st_tx_wait_d_ack;

    link_want_tx(link, msg, link_st_tx_wait_d_ack);
}

//! Передать команду-квитанцию.
static void link_tx_c(link_t* link)
{
    link_msg_t* cmd = link_msg_new(0);

if (cmd == 0)
FATAL_ERROR();
        
    cmd->N = link->tx.N;
    cmd->C = 1;
    cmd->B = 0;
    cmd->S = 0;
    cmd->addr = link->addr;

    link->tx.st = link_st_tx_wait_c_ack;

    link_want_tx(link, cmd, link_st_tx_wait_c_ack);
}


//! Получено сообщение, обработать.
//!@param[in] link Канал, по которому получено сообщение.
//!@param[in] msg Полученное сообщение.
void link_rx_msg(link_t* link, link_msg_t* ack)
{
    assert(link->magic = LINK_MAGIC);
    assert(ack->magic == MSG_MAGIC);

    if (link->st == link_st_disable || link->st == link_st_no_bi)
    {   // Работа с аппаратом прекращена, либо отсутствует физическое соединение.
        link_msg_del(ack);
        return;
    }

    link_B(link, ack->B);

    if (link->tx.st == link_st_tx_wait_ping)
    {   // Ответ до команды.
        log("Ответ до команды link->addr: %d, ack->addr: %d\n", link->addr, ack->addr);
        link_msg_del(ack);
        return;
    }

    // Была передана команда, ожидаем ответ.

    if (ack->N != link->tx.N) {
        // Циклический номер полученного ответа не соответствует циклическому
        // номер переданной квитацнии.
        log("Ошибка в N сообщения.\n");
        link_msg_del(ack);
        return;         
    }

    // Возможна ситуация когда истёк таймаут ожидания квитанции на очередную попытку
    // передачи команды, поставлена заявка на передачу очередной попыки и после этого
    // пришла квитанция на предыдущую попытку передачи команды. Поэтому после получения
    // квитанции всегда отменяем текущую передачу по данной link, так как выполнять очередную
    // попытку уже нет необходимости.
    
    link_cancel_rq_tx(link);       // Отменяем запрос на передачу.
    if (g_link.tx.link == link) {
        g_link.tx.st = g_tx_st_idle;
        g_link.tx.link = NULL;
        link_callback_check_for_tx(link);
    }

    link->tx.N ^= 1;
    
    // Останавливаем таймер на ожидание квитанции
    timer_stop(link->timer_ack);

    if (link->st == link_st_down) {
        // Находимся в состоянии установления звеньевого соединения.
        // Переходим в состояние "соединение установлено".
        link_st(link, link_st_up);
    }

    if (link->tx.st == link_st_tx_wait_d_ack) {
        // Была передана команда-данные.
        link_msg_t* msg;
        // В очереди обязано быть по крайней мере одно сообщение.
        assert(link->tx.current);
        // Получаем указатель на первое сообщение в очереди.
        msg = link->tx.current;
        assert(msg->magic == MSG_MAGIC);
        // Удаляем сообщение из очереди.
        list_remove(&msg->list);
        // Удаляем сообщение из ОЗУ.
        link_msg_del(msg);
        link->tx.current = NULL;
        // Переводим состояние передатчика "передано"
        // (будет извещён верхний уровень об успешной передачи сообщения)
        link_d_st(link, link_d_st_tx_ok);
        // Переводим состояние передатчика в "свободен"
        link_d_st(link, link_d_st_free);
    }

    // Проверяем есть ли непереданные сообщения в очереди на передачу, если есть
    // то передаём, иначе запускаем таймер на передачу команды-квитанциии.
    if ((link->tx.current == NULL) || link->tx.B) {
        // Нет сообщения для передачи или приёмный буфер "местного" занят, 
        // запускаем таймер, по истечению которго передадим команду-квитанцию.
        timer_start(link->timer_ping, link, TIMER_PING, TIMEOUT_PING);
        link->tx.st = link_st_tx_wait_ping;             
    }
    else {
        // Есть сообщение для передачи, передаём его.
        link->tx.attempts = 0; // Обнуляем счётчик числа попыток. 
        link_tx_d(link);       // Передаём.
    }

    // Если полученный ответ является ответом-данными, 
    // то передаём его на верхний уровень.
    if (ack->len != 0) 
        link_callback_rx(link, ack);
    else 
        link_msg_del(ack);
}

//------------------------------------------------------------------------------
// Обработчик таймаута.
//------------------------------------------------------------------------------
static void link_timeout(o_timer_t t, u8_t count, void** params)
{
    link_t* link = (link_t*)params[0];
    u8_t timer_id = (u8_t)(u32_t)params[1];
    
    switch (timer_id)
    {
    case TIMER_ACK: 
        putdbg('A');
        g_link.tx.st = g_tx_st_idle;
        g_link.tx.link = NULL;
        link_callback_check_for_tx(link);
        if (link->timer_ack == NULL) break;
        timer_stop(link->timer_ack);

        // Истёк тамаут ожидания квитанции
        if (link->st == link_st_disable || link->st == link_st_no_bi) 
            // работа с аппаратом прекращена или нет физ. канала
            break; 
        
        dbg("TIMEOUT, addr: %d\n", link->addr);
        
        if (link->st == link_st_down) {
            // Выполняется установка соединения.
            assert(link->tx.st == link_st_tx_wait_c_ack);
            // Очереданая попытка передачи команды квитацнии
            link_tx_c(link);
            break;
        }
        // Соединение установлено 
        if (link->tx.st == link_st_tx_wait_c_ack) {
            // Передавалась КОМАНДА-КВИТАНЦИЯ
            link->tx.attempts++;
            if (link->tx.attempts == MAX_ATTEMTPS) {
                // Достигнуто максимальное число попыток передачи команды-квитанции.
                // Переходим в режим установления соединения.
                link->tx.N = 0;
                link->tx.B = 1;   // удалённый корреспондент занят
                // Удаляем сообщения в буфере на передачу
                if (link->tx.current) {
                    assert(link->tx.current->magic == MSG_MAGIC);
                    list_remove(&link->tx.current->list);
                    link_msg_del(link->tx.current);
                    link->tx.current = NULL;
                    link_d_st(link, link_d_st_lost);
                    link->tx.d_st = link_d_st_free;
                }
                link_st(link, link_st_down);
            }
            // Передача команды-квитанции
            link_tx_c(link);
            break;
        }
        else if (link->tx.st == link_st_tx_wait_d_ack) {
            // Передавалась КОМАНДА-ДАННЫЕ
            link->tx.attempts++;
            if (link->tx.attempts == MAX_ATTEMTPS) {
                // Достигнуто максимальное число попыток передачи команды-данные.
                // Удаляем сообщения в буфере на передачу
                if (link->tx.current) {
                    assert(link->tx.current->magic == MSG_MAGIC);
                    list_remove(&link->tx.current->list);
                    link_msg_del(link->tx.current);
                    link->tx.current = NULL;
                    link_d_st(link, link_d_st_lost);
                    link->tx.d_st = link_d_st_free;
                }
                // Переходим в режим установления соединения.
                link_st(link, link_st_down);
                // Передача команды-квитанции
                link_tx_c(link);
                break;
            }
            // Выполянем очередную попытку передачи команды-данные.
            link_tx_d(link);
            break;
        }
        break;

    case TIMER_PING:
        putdbg('p');
        if (link->timer_ping == NULL) break;
        timer_stop(link->timer_ack);
        if (link->st != link_st_up) break;
        // Передаём команду квитанцию
        link->tx.attempts = 0;
        // Передача команды-квитанции
        link_tx_c(link);
        break;
    }
}

//! Передать сообщение.
int link_put_msg(link_t* link, link_msg_t* msg)
{
    assert(link->magic == LINK_MAGIC);
    assert(msg->magic == MSG_MAGIC);

    if (link->st != link_st_up) {
        log("Ошибка передачи: отсутствует звеньевое соединение.");
        link_msg_del(msg);
        return APP_E_NO_LINK; // звеньевое соединение не установлено
    }

    if (link->tx.B) {
        log("Ошибка передачи: местный аппарат занят.");
        link_msg_del(msg);
        return APP_E_RX_BUSY; // приёмный буфер "местного" занят
    }

    if (link->tx.current) {
        log("Ошибка передачи: передатчик занят.");
        link_msg_del(msg);
        return APP_E_TX_BUSY;
    }
    
    link->tx.current = msg;
    link_d_st(link, link_d_st_wait_chan); // ожидание освобождения канала

    if (link->tx.st == link_st_tx_wait_ping) {
        // Звеньевое соединение установлено
        timer_stop(link->timer_ping);
        // Обнуляем счётчик числа попыток
        link->tx.attempts = 0;
        // Передаём
        link_tx_d(link);
    }
    else if (link->tx.st == link_st_tx_wait_c_ack && 
             !list_is_empty(&link->tx_rq.list) &&
             link->tx.attempts == 0)
    {   // Если мы передаём команду-квитанцию и она находится в очереди на
        // передачу, то отменяем передачу (не отменяем очередную попытку 
        // передачи команды-квитанции).
        link_cancel_rq_tx(link);
        
        timer_stop(link->timer_ping);
        // Обнуляем счётчик числа попыток
        link->tx.attempts = 0;
        // Передаём
        link_tx_d(link);                
    }
            
    return APP_E_OK;
}

//////////////////////////// РАБОТА С СООБЩЕНИЯМИ //////////////////////////////

typedef struct {
  int create;
  int remove;\
} lmsg_stat_t;

lmsg_stat_t lmsg_stat;

//! Создать сообщение.
//!@param size Размер поля данных сообщения.
link_msg_t* link_msg_new(u16_t size)
{
    link_msg_t* msg = (link_msg_t*)link_malloc(sizeof(link_msg_t) + size);
    if (msg == NULL) FATAL_ERROR();
    
    memset(msg, 0, sizeof(link_msg_t) + size);

    msg->magic = MSG_MAGIC;
    msg->len = size;
    list_init(&msg->list);

lmsg_stat.create++;

    return msg;
}

//! Создать копию существующего сообщения.
link_msg_t* link_msg_clone(const link_msg_t* msg)
{
    link_msg_t* clone = link_msg_new(msg->len);

    clone->addr = msg->addr;
    ////clone->r_addr = msg->r_addr;
    clone->N = msg->N;
    clone->C = msg->C;
    clone->B = msg->B;
    clone->S = msg->S;

    memcpy(clone->body, msg->body, msg->len);

    return clone;
}

//! Удалить сообщение.
//!@param msg Удаляемое сообщение.
void link_msg_del(link_msg_t* msg)
{
    assert(msg->magic == MSG_MAGIC);
    
    assert(list_is_empty(&msg->list));

    memset(msg, 0, sizeof(link_msg_t));
    link_free(msg);
lmsg_stat.remove++;
}

#ifdef CHECK_MALLOC


// КОД ДЛЯ ПОИСКА УТЕЧКИ ПАМЯТИ

#warning Включена проверка кода на утечку памяти.

#define LINK_MEM_ARRAY_SIZE 1024
#define LINK_MAGIC_SIZE 4

typedef struct
{
        size_t size;
        void* buf;      
}mem_t;

static struct
{
        mem_t array[LINK_MEM_ARRAY_SIZE];
        u32_t used;
        pthread_mutex_t mutex;
}link_mem;

void link_mem_init()
{
        memset(&link_mem, 0, sizeof(link_mem));
        pthread_mutex_init(&link_mem.mutex, NULL);
}

void* link_malloc(size_t size)
{
        int i;
        pthread_mutex_lock(&link_mem.mutex);
        for (i=0; i<sizeof(link_mem.array)/sizeof(link_mem.array[0]); i++)
        {
                mem_t* const me = &link_mem.array[i];
                if (me->buf == NULL)
                {
                        me->buf = malloc(size + LINK_MAGIC_SIZE);
                        if (me->buf == NULL) FATAL_ERROR();
                        me->size = size;
                        link_mem.used += size;
                        memset((char*)me->buf + size, 'm', LINK_MAGIC_SIZE);
                        pthread_mutex_unlock(&link_mem.mutex);
                        return me->buf;
                }
        }
        FATAL_ERROR();
}

void link_free(void* mem_ptr)
{
        int i, j;
        pthread_mutex_lock(&link_mem.mutex);
        for (i=0; i<sizeof(link_mem.array)/sizeof(link_mem.array[0]); i++)
        {
                mem_t* const me = &link_mem.array[i];
                if (me->buf == mem_ptr)
                {
                        char* b = (char*)mem_ptr + me->size;
                        for (j=0; j<LINK_MAGIC_SIZE; j++)
                        {
                                if (b[j] != 'm') FATAL_ERROR();
                        }
                        memset(mem_ptr, 0xD1, me->size);
                        link_mem.used -= me->size;
                        me->buf = NULL;
                        me->size = 0;
                        free(mem_ptr);
                        pthread_mutex_unlock(&link_mem.mutex);
                        return;
                }
        }
        FATAL_ERROR();
}

u32_t link_head_mem()
{
        return link_mem.used;
}


#endif // CHECK_MALLOC
