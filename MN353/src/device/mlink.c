
// ����� �������� ����������

#include <types.h>
#include <app.h>
#include "mlink.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <restart.h>
#include <assert.h>

//#define assert ASSERT

// �������������� ��������
#define TIMER_ACK  1   // ������ �������� ������
#define TIMER_PING 2   // ������ �������� ������� �������� �������-���������

// ������� �������� ������ � �� (4.4 ��� �� ���������)
#define TIMEOUT_ACK   (4400/portTICK_PERIOD_MS) 
// ������� �������� �������-��������� � �� (�� ��������� �� ���� 1 � 2 ���)
#define TIMEOUT_PING  (1500/portTICK_PERIOD_MS) 

// ������������ ����� ������� �������� ���������
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

//////////////////////////////// ��������� /////////////////////////////////////
static void link_tx_c(link_t* link);
static void link_cancel_rq_tx(link_t* link);
static void link_timeout(o_timer_t t, u8_t count, void** params);

static struct
{
    struct
    {
        list_t q;       // ������� �������� �� ��������
        enum {
            g_tx_st_idle,
            g_tx_st_busy
        } st;           // ��������� �����������
        link_t* link;
    }tx;
} g_link =
{
    .tx = {
        .q  = {&g_link.tx.q, &g_link.tx.q},
        .st = g_tx_st_idle
    }
};


//! ���������������� ��������� link_t.
//!@param link ��������� �� ���������, ������� ���������� ����������������.
//!@addr �����.
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

//! ��������� ���������� � ����� ���������, � �����������
//! �������� ������ � ����� ���������.
//!@param link ����������, ������� ���������� ��������� � ����� ���������.
//!@param st ����� ��������� ����������.
static void link_st(link_t* link, link_st_t st)
{
    if (link->st != st) {
        // ��������� ���������� ����������, ��������� ������� �������.
        link->st = st;
        link_callback_st(link);
    }
}

//! ���������� ����� ��������� ����������� �������������� ��������� �
//! ���������� ������� ������� � ����� ���������.
static void link_d_st(link_t* link, link_d_st_t st)
{
    if (link->tx.d_st != st) {
        // ��������� ���������� ����������, ��������� ������� �������.
        link->tx.d_st = st;
        link_callback_d_st(link, st);
    }
}

//! ���������� ������� ��������� �������� ��������.
static void link_B(link_t* link, u8_t B)
{
    if (link->tx.B != B) {
        link->tx.B = B;
        link_callback_B(link, B);
    }
}

//! ������� � ��������� ������������ ����������.
void link_start(link_t* link)
{
    assert(link->magic == LINK_MAGIC);

    link_st(link, link_st_no_bi);
}

//! ���������� ������ �� ������.
void link_stop(link_t* link)
{
    assert(link->magic == LINK_MAGIC);

    if (link->st == link_st_disable) return;

    // ������������� ��� �������
    timer_stop(link->timer_ack);
    timer_stop(link->timer_ping);
    
    // ������� ��������� � ������ �� ��������
    if (link->tx.current) {
        assert(link->tx.current->magic == MSG_MAGIC);
        list_remove(&link->tx.current->list);
        link_msg_del(link->tx.current);
        link->tx.current = NULL;
        link->tx.d_st = link_d_st_free;
    }

    // ���� � ����� ������� �������� ��������� ������ �� �������� ��
    // ������ ����������, �� ������� ������ �� �������.
    link_cancel_rq_tx(link);
    if (g_link.tx.link == link) {
        // ����� ���������� ����� ��������� ������ ���������, �������� �������� - 
        // ������ ������� "���������� ��������" � ��������� � �������� ���������
        // �� ���������� ������.
        g_link.tx.st = g_tx_st_idle;
        g_link.tx.link = NULL;
        link_callback_check_for_tx(link);
    }
    
    // ������������� ��������� ������ "��������"
    link_st(link, link_st_disable);
}

//! ���������� ������ ���������� ���������� ���������� �
//! ��������� ����� �� ���������� ������.
void link_ph_layer_up(link_t* link)
{
    assert(link->magic == LINK_MAGIC);
    // ���������� �� ���������� ������ ����� ��������� ������, ����
    // ��� ������ �� ����.

    if (link->st != link_st_no_bi) return;
    
    // ���������� �� ���������� ������ � ��� ����, ������ ����� ����������
    // ���������� ���������� �� ��������� ������.
    link->tx.N = 0;
    link->tx.B = 1; // �������� ������������� �����
    link->tx.st = link_st_tx_wait_ping;

    // ������� ������� ���������
    link_tx_c(link);

    // ��������� ���������� � ��������� "������������ ����������"
    link_st(link, link_st_down);
}

//! ��������� ������ ��������� ���������� ���������� � ������ ����������� ����������.
void link_ph_layer_down(link_t* link)
{
    assert(link->magic == LINK_MAGIC);
    
    if (link->st == link_st_disable || link->st == link_st_no_bi) return;

    timer_stop(link->timer_ack);
    timer_stop(link->timer_ping);

    // ������� ��������� � ������ �� ��������
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

//! ��������� �� (�� - ��������� 2 �����).
//!@return ���������� 0 - �����, ����� ������ ��.
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

//! ���������������� ���������, ���������� �������� �������, �����������
//! ��� ��������� ����� �� ��������� ������ ���������� �����.
//!@param m ��������� �� ���������, ������� ���������� ����������������.
void link_rx_machin_init(link_rx_machin_t* m)
{
    memset(m, 0, sizeof(link_rx_machin_t));
    m->st = rx_st_mb;
}

//! ���������� ����� ������, ���������� �� ���������� ����� �����.
//!@param m      ��������� ���������, ���������� ������ ��������� ��������, 
//!              ������������� ��� ��������� ����� �� ������ ����.
//!@param buffer ���������� �� ���������� ����� ����� ������.
//!@param size   ����� ���� � ������� buffer.
//!@param[in,out] msg_list ������, � ������� ���������� ��������� ����������
//!                        ����� (���������).
//!@return ����� ���������� ��������� � ���������� � ������ msg_list.
int mlink_rx_bytes(link_rx_machin_t* m, const u8_t* buffer, u32_t size, list_t* msg_list)
{
    u32_t i;
    int msg_count = 0;
    
    // ������ �����:
    // <B><H><SLUG><������><KS0><KS1><B><K>
    // ������ ����� <H> ��� ������ ��������� ����� ���� ���� <P>

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
    begin:          // ������� ������ ������
                    m->st = rx_st_data;
                    m->len = 0;
                }
                else
                    m->st = rx_st_mb;
                continue;

        case rx_st_data:
                if (byte == MB) {
                    // ������ ���� ������� �����, ������ ���������� ������, 
                    // ���� �����������
                    m->st = rx_st_mk;
                    continue;
                }
        put_byte:
                if (m->len == LINK_MAX_FRAME_SIZE) {
                    // ���������� ���� ������� �����
                    m->ovf++;
                    m->st = rx_st_mb;
                    continue;
                }
                m->buffer[m->len++] = byte;
                continue;

        case rx_st_mk:
                if (byte == MB) {
                    // ���� �������
                    m->st = rx_st_data;
                    goto put_byte;
                }
                if ((byte == MH) || (byte == MP)) {
                    // �������� ������ ������, ����������� ������� ����
                    goto begin;
                }
                
                m->st = rx_st_mb;

                if (byte == MK) {
                    // �������� ������ �����
                    link_msg_t* msg;
                    //log("<��>\n");
                    // ��������� ����������� ���������
                    m->st = rx_st_mb;
                    // �������� ����� �� ����������� ������ 
                    if (m->len < LINK_MIN_FRAME_SIZE) {
                        // ����������� �������� ����
                        m->trun++;
                        continue;
                    }
                    // �������� ����������� �����
                    if (link_check_ks(m->buffer, m->len)) {
                        // ������ � ��
                        m->cs++;
                        for (int k=0; k<m->len; k++) printf("%02X ",m->buffer[k]);
                        printf("\r\n");
                        continue;
                    }
                    // ����� � ����������� ����� ����� ���������, ������ �� ����
                    // ��������� ���������� ������.
                    msg = link_msg_new(m->len - (1 + 2)); // ��������� ������� + ��
                    // ���� ���������
                    memcpy(msg->body, &m->buffer[1], msg->len);
                    
                    // ���� �� ��������� �������
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
                    // ��������� ��������� � ������ �������� ���������
                    list_add(msg_list, &msg->list);
                    // �������������� ����� ����������� ��������� � ������
                    msg_count++;
                }
                else
                    m->mark++; // ������ ��������� ������� �����
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

//! �������� ��������� � �����.
//!@msg ���������, ���������� ��������.
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

    ptr[0] = MK; // ������ �
    ptr[1] = MB; // ������ ������

    if (msg->len == 0)
        ptr[2] = MP; // ����-���������
    else
        ptr[2] = MH; // ����-������

    // ��������� �������
    byte = msg->addr & 0xF;
    if (msg->len && msg->S) byte |= (1 << 4);
    if (msg->B) byte |= (1 << 5);
    if (msg->C) byte |= (1 << 6);
    if (msg->N) byte |= (1 << 7);
    ptr[3] = byte;
    size = 4;
    if (byte == MB) ptr[size++] = MB;
    ks = fletcher_put(ks, byte);

    // ������
    for (i=0; i<msg->len; i++) {
        byte = msg->body[i];
        ptr[size++] = byte;
        if (byte == MB) ptr[size++] = MB;
        ks = fletcher_put(ks, byte);
    }
    // ����������� �����
    ks = fletcher_put(ks, 0);
    ks = fletcher_put(ks, 0);
    ks = fletcher_make(ks);

    byte = (u8_t)ks;
    ptr[size++] = byte;
    if (byte == MB) ptr[size++] = MB;
    byte = (u8_t)(ks >> 8);
    ptr[size++] = byte;
    if (byte == MB) ptr[size++] = MB;

    // ������ �����
    ptr[size++] = MB;
    ptr[size++] = MK;

    // �������� ������ ����� � ���������� �����
    link_callback_tx_bytes(ptr, size);

    link_free(ptr);
}

// ��������: ��������� ��������� � ������ ������� ��������� �� �������� � �������.
// ���������:
// @link - ��������� ����������, � �������� ���������� ���������;
// @msg - ��������� ���������� ��������;
// @tx_st - ��������� �����������, ����� �������� ������� ��������� (�������
// ���� ��� �������: �������-��������� ��� �������-������);
static void link_want_tx(link_t* link, link_msg_t* msg, link_st_tx_t tx_st)
{
    // ���������, ��� ��������� tx_rq �� ������������.
    assert(list_is_empty(&link->tx_rq.list));

    link->tx_rq.msg = msg;
    link->tx_rq.tx_st = tx_st;

    if (msg->addr == 0)
            list_add_head(&g_link.tx.q, &link->tx_rq.list);
    else
            list_add(&g_link.tx.q, &link->tx_rq.list);
    
    // ��������� ����� �� �������� ��������� �� �������.
    link_check_for_tx(link);
}

// ��������: �������� ������ �� ��������.
// ���������:
// @link ��������� ����������, ������ �� �������� ��������� ����� ������� ����������
// ��������.
static void link_cancel_rq_tx(link_t* link)
{       
    if (list_is_empty(&link->tx_rq.list)) 
            return; // �� ������� ���������� � ������� �������� �� �������� ������� ���.

    // ������� ������ �� ��������
    list_remove(&link->tx_rq.list);

    // ������� �� ���������� ��������� (������ �������-���������, �������-������ ���������
    // � ������� � ����� ������� ��� �������� ������� ��� ��� �������� �������� ���������)
    if (link->tx_rq.tx_st == link_st_tx_wait_c_ack && link->tx_rq.msg)
            link_msg_del(link->tx_rq.msg);
    link->tx_rq.msg = NULL;
}


// ��������: ��������� ����� �� �������� ��������� �� ������ ������� ���������
// �� ��������.
void link_check_for_tx(link_t* link)
{
    link_tx_rq* rq;

    if (g_link.tx.st != g_tx_st_idle) { 
        // ���������� �����
        putdbg('*'); 
        return; 
    } 

    if (list_is_empty(&g_link.tx.q)) { 
        // ��� �������� �� ��������
        putdbg('.'); 
        return; 
    } 

    // ��������� �� ������� �������� �� ��������, ������ �� ���, ������ ���,
    // ������� �������� ������ ����.
    rq = list_entry(g_link.tx.q.next, link_tx_rq);
    list_remove(&rq->list);

    // �������� ���������
    link_send(rq->msg);
    putdbg('S');
    // ����� ������� �������� ���������
    timer_start(rq->link->timer_ack, rq->link, TIMER_ACK, TIMEOUT_ACK);
    // ��������� ���������� � ��������� "�����" (�������� ������)
    g_link.tx.st = g_tx_st_busy;
    g_link.tx.link = rq->link;   // ��������� ��������� �� ����������, ���������
                                 // ��������� �������� �� ������ � ��������� ������

    // ������� ���������� ���������, ���� ��� ������ �� �����.
    if (rq->tx_st == link_st_tx_wait_c_ack) 
        link_msg_del(rq->msg);
    else {
        // �������� � ����� �������-������ - ����� ��������� ���������: 
        // �������� ��������� �� �������� ��������.
        link_d_st(rq->link, link_d_st_wait_ack);
    }
    // ������� ������ �� ��������� (�� ������ ������).
    rq->msg = NULL;
}

//! ��������� ��������� ������� �������� ������� ��������� �� �������
//! ��������� �� ��������.
static void link_tx_d(link_t* link)
{
    link_msg_t* msg;

    // �������� ��������� �� ������ ��������� � �������.
    assert(link->tx.current && link->tx.current->magic == MSG_MAGIC);

    msg = link->tx.current;
    msg->N = link->tx.N;
    msg->C = 1;
    msg->B = 0; // � ������� �� �����
    msg->addr = link->addr;

    link->tx.st = link_st_tx_wait_d_ack;

    link_want_tx(link, msg, link_st_tx_wait_d_ack);
}

//! �������� �������-���������.
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


//! �������� ���������, ����������.
//!@param[in] link �����, �� �������� �������� ���������.
//!@param[in] msg ���������� ���������.
void link_rx_msg(link_t* link, link_msg_t* ack)
{
    assert(link->magic = LINK_MAGIC);
    assert(ack->magic == MSG_MAGIC);

    if (link->st == link_st_disable || link->st == link_st_no_bi)
    {   // ������ � ��������� ����������, ���� ����������� ���������� ����������.
        link_msg_del(ack);
        return;
    }

    link_B(link, ack->B);

    if (link->tx.st == link_st_tx_wait_ping)
    {   // ����� �� �������.
        log("����� �� ������� link->addr: %d, ack->addr: %d\n", link->addr, ack->addr);
        link_msg_del(ack);
        return;
    }

    // ���� �������� �������, ������� �����.

    if (ack->N != link->tx.N) {
        // ����������� ����� ����������� ������ �� ������������� ������������
        // ����� ���������� ���������.
        log("������ � N ���������.\n");
        link_msg_del(ack);
        return;         
    }

    // �������� �������� ����� ���� ������� �������� ��������� �� ��������� �������
    // �������� �������, ���������� ������ �� �������� ��������� ������ � ����� �����
    // ������ ��������� �� ���������� ������� �������� �������. ������� ����� ���������
    // ��������� ������ �������� ������� �������� �� ������ link, ��� ��� ��������� ���������
    // ������� ��� ��� �������������.
    
    link_cancel_rq_tx(link);       // �������� ������ �� ��������.
    if (g_link.tx.link == link) {
        g_link.tx.st = g_tx_st_idle;
        g_link.tx.link = NULL;
        link_callback_check_for_tx(link);
    }

    link->tx.N ^= 1;
    
    // ������������� ������ �� �������� ���������
    timer_stop(link->timer_ack);

    if (link->st == link_st_down) {
        // ��������� � ��������� ������������ ���������� ����������.
        // ��������� � ��������� "���������� �����������".
        link_st(link, link_st_up);
    }

    if (link->tx.st == link_st_tx_wait_d_ack) {
        // ���� �������� �������-������.
        link_msg_t* msg;
        // � ������� ������� ���� �� ������� ���� ���� ���������.
        assert(link->tx.current);
        // �������� ��������� �� ������ ��������� � �������.
        msg = link->tx.current;
        assert(msg->magic == MSG_MAGIC);
        // ������� ��������� �� �������.
        list_remove(&msg->list);
        // ������� ��������� �� ���.
        link_msg_del(msg);
        link->tx.current = NULL;
        // ��������� ��������� ����������� "��������"
        // (����� ������� ������� ������� �� �������� �������� ���������)
        link_d_st(link, link_d_st_tx_ok);
        // ��������� ��������� ����������� � "��������"
        link_d_st(link, link_d_st_free);
    }

    // ��������� ���� �� ������������ ��������� � ������� �� ��������, ���� ����
    // �� �������, ����� ��������� ������ �� �������� �������-����������.
    if ((link->tx.current == NULL) || link->tx.B) {
        // ��� ��������� ��� �������� ��� ������� ����� "��������" �����, 
        // ��������� ������, �� ��������� ������� ��������� �������-���������.
        timer_start(link->timer_ping, link, TIMER_PING, TIMEOUT_PING);
        link->tx.st = link_st_tx_wait_ping;             
    }
    else {
        // ���� ��������� ��� ��������, ������� ���.
        link->tx.attempts = 0; // �������� ������� ����� �������. 
        link_tx_d(link);       // �������.
    }

    // ���� ���������� ����� �������� �������-�������, 
    // �� ������� ��� �� ������� �������.
    if (ack->len != 0) 
        link_callback_rx(link, ack);
    else 
        link_msg_del(ack);
}

//------------------------------------------------------------------------------
// ���������� ��������.
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

        // ���� ������ �������� ���������
        if (link->st == link_st_disable || link->st == link_st_no_bi) 
            // ������ � ��������� ���������� ��� ��� ���. ������
            break; 
        
        dbg("TIMEOUT, addr: %d\n", link->addr);
        
        if (link->st == link_st_down) {
            // ����������� ��������� ����������.
            assert(link->tx.st == link_st_tx_wait_c_ack);
            // ���������� ������� �������� ������� ���������
            link_tx_c(link);
            break;
        }
        // ���������� ����������� 
        if (link->tx.st == link_st_tx_wait_c_ack) {
            // ������������ �������-���������
            link->tx.attempts++;
            if (link->tx.attempts == MAX_ATTEMTPS) {
                // ���������� ������������ ����� ������� �������� �������-���������.
                // ��������� � ����� ������������ ����������.
                link->tx.N = 0;
                link->tx.B = 1;   // �������� ������������� �����
                // ������� ��������� � ������ �� ��������
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
            // �������� �������-���������
            link_tx_c(link);
            break;
        }
        else if (link->tx.st == link_st_tx_wait_d_ack) {
            // ������������ �������-������
            link->tx.attempts++;
            if (link->tx.attempts == MAX_ATTEMTPS) {
                // ���������� ������������ ����� ������� �������� �������-������.
                // ������� ��������� � ������ �� ��������
                if (link->tx.current) {
                    assert(link->tx.current->magic == MSG_MAGIC);
                    list_remove(&link->tx.current->list);
                    link_msg_del(link->tx.current);
                    link->tx.current = NULL;
                    link_d_st(link, link_d_st_lost);
                    link->tx.d_st = link_d_st_free;
                }
                // ��������� � ����� ������������ ����������.
                link_st(link, link_st_down);
                // �������� �������-���������
                link_tx_c(link);
                break;
            }
            // ��������� ��������� ������� �������� �������-������.
            link_tx_d(link);
            break;
        }
        break;

    case TIMER_PING:
        putdbg('p');
        if (link->timer_ping == NULL) break;
        timer_stop(link->timer_ack);
        if (link->st != link_st_up) break;
        // ������� ������� ���������
        link->tx.attempts = 0;
        // �������� �������-���������
        link_tx_c(link);
        break;
    }
}

//! �������� ���������.
int link_put_msg(link_t* link, link_msg_t* msg)
{
    assert(link->magic == LINK_MAGIC);
    assert(msg->magic == MSG_MAGIC);

    if (link->st != link_st_up) {
        log("������ ��������: ����������� ��������� ����������.");
        link_msg_del(msg);
        return APP_E_NO_LINK; // ��������� ���������� �� �����������
    }

    if (link->tx.B) {
        log("������ ��������: ������� ������� �����.");
        link_msg_del(msg);
        return APP_E_RX_BUSY; // ������� ����� "��������" �����
    }

    if (link->tx.current) {
        log("������ ��������: ���������� �����.");
        link_msg_del(msg);
        return APP_E_TX_BUSY;
    }
    
    link->tx.current = msg;
    link_d_st(link, link_d_st_wait_chan); // �������� ������������ ������

    if (link->tx.st == link_st_tx_wait_ping) {
        // ��������� ���������� �����������
        timer_stop(link->timer_ping);
        // �������� ������� ����� �������
        link->tx.attempts = 0;
        // �������
        link_tx_d(link);
    }
    else if (link->tx.st == link_st_tx_wait_c_ack && 
             !list_is_empty(&link->tx_rq.list) &&
             link->tx.attempts == 0)
    {   // ���� �� ������� �������-��������� � ��� ��������� � ������� ��
        // ��������, �� �������� �������� (�� �������� ��������� ������� 
        // �������� �������-���������).
        link_cancel_rq_tx(link);
        
        timer_stop(link->timer_ping);
        // �������� ������� ����� �������
        link->tx.attempts = 0;
        // �������
        link_tx_d(link);                
    }
            
    return APP_E_OK;
}

//////////////////////////// ������ � ����������� //////////////////////////////

typedef struct {
  int create;
  int remove;\
} lmsg_stat_t;

lmsg_stat_t lmsg_stat;

//! ������� ���������.
//!@param size ������ ���� ������ ���������.
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

//! ������� ����� ������������� ���������.
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

//! ������� ���������.
//!@param msg ��������� ���������.
void link_msg_del(link_msg_t* msg)
{
    assert(msg->magic == MSG_MAGIC);
    
    assert(list_is_empty(&msg->list));

    memset(msg, 0, sizeof(link_msg_t));
    link_free(msg);
lmsg_stat.remove++;
}

#ifdef CHECK_MALLOC


// ��� ��� ������ ������ ������

#warning �������� �������� ���� �� ������ ������.

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
