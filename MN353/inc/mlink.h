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
{	// ��������� ���������� ���������� � ��������� 
	link_st_disable, // ���������� ���������
	link_st_no_bi,   // ��� ����������� ����������
	link_st_down,    // ��� ���������� ���������� (�������������)
	link_st_up       // ���� ��������� ����������
}link_st_t;


typedef enum
{
	link_st_tx_wait_c_ack, // �������� ������ �� �������-���������
	link_st_tx_wait_d_ack, // �������� ������ �� �������-������
	link_st_tx_wait_ping   // �������� ����������� ������� �������� �������-���������
}link_st_tx_t;

// ��������� ����������� "�������-������"
typedef enum
{
	link_d_st_free,      // ��� ��������� ��� ��������
	link_d_st_wait_chan, // �������� ������������ ������
	link_d_st_wait_ack,  // �������� ��������� (�������� ���������)
	link_d_st_tx_ok,     // ��������� ��������
	link_d_st_lost       // ��������� ��������
}link_d_st_t;

typedef struct
{
	u16_t len;     // ������ � ������ ���� ������ (��� ����� �� � ��������)
	u32_t magic;
	list_t list;
	u8_t addr : 4, // ��������� �����
		N : 1, // ����������� �����
		C : 1, // ��� ������� (1) / ������ (0)
		B : 1, // ��� ���������
		S : 1; // ��� ��������
	u8_t body[1];  // ������ ���� ������ ���������
} link_msg_t;

// ������ �� �������� ��������� � ������� � ������ ������� ��������.
typedef struct
{
	link_msg_t* msg;        // ���������, ������� ���������� ��������
	list_t list;            // ��� ��������� � ������� (�� ������ ������)
	link_st_tx_t tx_st;     // ��������� ����������� ����� �������� ���������
	struct link_t* link;    // ��������� ����������, � �������� ��������� ������������ ���������
} link_tx_rq;

typedef struct link_t
{
	u32_t magic;
	// ��������� ���������� ���������� � ���������
	link_st_t st;

	u8_t ca369; // !0 - ����� "��369"

	u8_t addr; // �����

	struct
	{
		u8_t N : 1, // ����������� ����� �� ��������
		     B : 1; // ������� ��������� ��������������
	
		link_msg_t* current;  // �������-������, ���������� ��������
		link_st_tx_t st;      // ��������� �����������
		u8_t attempts;        // ����� ����������� ������� ��������
		link_d_st_t d_st;     // ��������� ����������� "�������-������"
	}tx;

	o_timer_t timer_ack;
	o_timer_t timer_ping;

	link_tx_rq tx_rq;
} link_t;



// ����������� ���������� ����� ����� � ������ ��������� �������,
// ������ � ����������� �����
#define LINK_MAX_FRAME_SIZE (1+2048+2+16)

// ���������� ���������� ����� ����� � ������ ��������� ������� �
// ����������� �����
#define LINK_MIN_FRAME_SIZE (1+0+2)

//! ��������� ��� ��������� ����� �� ����� �� ������ ����.
typedef struct
{	//! ��������� �������� ��������
	enum
	{
		rx_st_idle,  // ���� ��������
		rx_st_mb,    // �������� ������ ����� ������� ������
		rx_st_mh,    // �������� ������� ����� ������� ������
		rx_st_data,  // �������� ����� ������ �����
		rx_st_mk     // �������� ������� ������ ������� �����
	}st;

	u16_t len; // ����� ���������� ���� ��������� (����� ���� � ������)

	u8_t buffer[LINK_MAX_FRAME_SIZE];
	
	////u8_t ca369;   // !0 - ������� ���� � �������� ������ ���������������� ������ ��369

	// ���������� ������
	u32_t ovf;    // ����� ������������ �������� ������
	u32_t trun;   // ����� ������: ����������� �������� ����
	u32_t mark;   // ����� ������ ��������� ��������
	u32_t cs;     // ����� ������ � ����������� �����

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

// �������, �� ������� ��������� ������ link.c
extern void link_callback_timeout(void* p1, void* p2);
extern void link_callback_st(link_t* link);
extern void link_callback_d_st(link_t* link, link_d_st_t st);
extern void link_callback_B(link_t* link, u8_t B);
extern void link_callback_rx(link_t* link, link_msg_t* msg);
extern void link_callback_tx_bytes(const u8_t* buffer, u32_t size);
extern void link_callback_check_for_tx(link_t* link);
#endif // _LINK_H

