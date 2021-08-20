#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <types.h>
#include <stdio.h>
#include <string.h>
#include <mlink.h>
#include <olist.h>
#include <restart.h>
#include <assert.h>
#include <MDR32Fx.h>
#include <app.h>
#include <serial.h>
#include <ks.h>

typedef SemaphoreHandle_t pthread_mutex_t;
typedef SemaphoreHandle_t sem_t;
typedef TaskHandle_t pthread_t;

#define ASSERT assert 

// ����� ������ �������� ������ �� ��������� (����� �� ��������� �����) � ��
#define SLEEP_TIME_MS 200
#define APP_MAX_DATA_SIZE 1024


static u8_t mask[8];
static u8_t app_rx_buf[APP_MAX_DATA_SIZE];


// ������� ���������
typedef struct {        
    pthread_mutex_t mutex;  // ����������� ������� �������� ���������
    list_t list;            // ���������� ��������� ������ ��������� � �������
    sem_t sem_ready;        // ������� "���� �������� � �������"
} app_msg_q_t;

typedef struct
{
    pthread_mutex_t mutex;      // ��������� � ���, ����� ��� �������� �������
    pthread_mutex_t mutex_msg;  // ������ ������ � ����������� (��������, ������������)
    link_t link;
    link_rx_machin_t rx_sm;
    pthread_t th;               // ������������� ������ ��������
    app_msg_q_t rep_q;          // ������� ���������� �������
    u16_t sleep;                // ����� ������ �������� ������ ������ � ��

    sem_t sem_sleep; // �����, ������������ ������������ ������� ����� ������,
                     // "����" �� ������ �������� � ������� app_cb_t.sleep.
                     // ��������� �������� ����� �����.

    struct {
        struct {
            u32_t frame;
            u32_t rep;
        } rx;
    
        struct {
            u32_t frame;
            u32_t cmd;
        } tx;
    
        struct {
            u32_t prot;
            u32_t time;
        } e;
    } stat;

} app_cb_t;


static void app_msg_q_put(app_msg_q_t* q, link_msg_t* m);
static void app_msg_q_init(app_msg_q_t* q);
static void app_thread_rx(void* param);

#define MORE_INFO

void print_result(int rv) 
{
#ifdef MORE_INFO
    switch(rv) {
        case APP_E_OK:      printf("\tAPP_E_OK\n"); break;
        case APP_E_TX_BUSY: printf("\tAPP_E_TX_BUSY\n"); break;
        case APP_E_NO_LINK: printf("\tAPP_E_NO_LINK\n"); break;
        default: printf("DEF:%d",rv); break;
    }
#endif    
}

////////////////////// ������������� � FreeRTOS ///////////////////////////////

int pthread_create( pthread_t * id,u32_t * attr, void (* task)(void *), void * arg)
{               //������� �����
 
    xTaskCreate(task, "TH", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, id);

    return 0;
}

int pthread_cancel (pthread_t th)
{
    vTaskDelete(th);
    return 0;
}

int pthread_mutex_init(pthread_mutex_t * mutex, int *attre)
{       
    if (*mutex)
        vSemaphoreDelete(*mutex);
    *mutex = xSemaphoreCreateMutex();
    return 0;
}

int pthread_mutex_lock (pthread_mutex_t * mutex)
{
    if (*mutex == 0)
        return -1;
    xSemaphoreTake(*mutex,portMAX_DELAY);
    return 0;
}

int pthread_mutex_unlock (pthread_mutex_t * mutex)
{
    if (*mutex == 0)
        return -1;
    xSemaphoreGive(*mutex);
    return 0;
}

int sem_init (sem_t * sem, int pshared, unsigned int  value)
{
    if (*sem)
        vSemaphoreDelete(*sem);
 
    *sem = xSemaphoreCreateBinary();
    return 0;
}

int sem_post (sem_t * sem)
{
    if (*sem == 0)
        return -1;
    xSemaphoreGive(*sem);
    return 0;
}

int sem_trywait (sem_t * sem)
{  
    if (xSemaphoreTake(*sem,0) == pdFALSE)
        return -1;
    else
        return 0;
}

int sem_timedwait(sem_t * sem, u32_t ms)
{
    if (xSemaphoreTake(*sem,ms/portTICK_PERIOD_MS) == pdFALSE)
        return -1;
    else
        return 0;  
}
///////////////////////////////////////////////////////////////////////////////



static app_cb_t app;


// ����� ��������� ����������.
void link_callback_st(link_t* link) 
{
    switch (link->st) {
        case link_st_disable:printf("���������� ���������\n"); break;
        case link_st_no_bi:  printf("��� ����������� ����������\n");
                             break;
        case link_st_down:   printf("��� ���������� ����������\n"); 
                             //if (icg_output_exec()) icg_link_down();
                             break;
        case link_st_up:     printf("���� ��������� ����������\n");
                             //if (icg_output_exec()) icg_link_up();
                             break;
        default:  printf("������������ ��������� �����\n"); break;
    }
}

// ����� ��������� ����������� �������������� ���������
void link_callback_d_st(link_t* link, link_d_st_t st) 
{
    switch (link->tx.d_st) {
        case link_d_st_free:      printf("\t��� ��������� ��� ��������\n"); break;
        case link_d_st_wait_chan: printf("\t�������� ������������ ������\n"); break;
        case link_d_st_wait_ack:  printf("\t�������� ��������� (�������� ���������)\n"); break;
        case link_d_st_tx_ok:     printf("\t��������� ��������\n"); break;
        case link_d_st_lost:      printf("\t��������� ��������\n"); break;
        default: printf("\t������������ ��������� ��������\n"); break;
    }
}

// ����� ��������� - ������� ��������� �������� ��������.
void link_callback_B(link_t* link, u8_t B) {;}


//------------------------------------------------------------------------------
// ������� �����-������.
// ��������� ��������� � ������� �������
//------------------------------------------------------------------------------
void link_callback_rx(link_t* link, link_msg_t* msg) 
{
    if (link != &app.link) FATAL_ERROR();
    
    app.stat.rx.rep++;
    app_msg_q_put(&app.rep_q, msg);
    printf("\t������� ����-������\n");
}

//------------------------------------------------------------------------------
// �������� ������ ����� � ���������� �����
//------------------------------------------------------------------------------
void link_callback_tx_bytes(const u8_t* buffer, u32_t size) 
{
    u32_t offset;
    int rs;
  
    offset = 0;
    while (offset < size) {
        rs = serial_write(&buffer[offset], size - offset);
            if (rs < 0) {
                FATAL_ERROR();
            }
            offset += rs;
    }
}

void link_callback_check_for_tx(link_t* link) {;}

//------------------------------------------------------------------------------
// ���������������� �������.
//------------------------------------------------------------------------------
static void app_msg_q_init(app_msg_q_t* q)
{
    memset(q, 0, sizeof(app_msg_q_t));
    pthread_mutex_init(&q->mutex, NULL);
    list_init(&q->list);
    sem_init(&q->sem_ready, 0, 0);
}


//------------------------------------------------------------------------------
// ��������� ��������� � �������.
//    q - �������, ���� ���������� ���������;
//    m - ���������� � ������� ���������.
//------------------------------------------------------------------------------
static void app_msg_q_put(app_msg_q_t* q, link_msg_t* m)
{
    // ��������� �� ������ ���� ��� �������� � �����-���� ������� (������)
    ASSERT(list_is_empty(&m->list));

    pthread_mutex_lock(&q->mutex);

    list_add(&q->list, &m->list);
    sem_post(&q->sem_ready);

    pthread_mutex_unlock(&q->mutex);
}


//------------------------------------------------------------------------------
// ������� ��������� �� �������.
//------------------------------------------------------------------------------
static link_msg_t * app_msg_q_get(app_msg_q_t* q)
{
    link_msg_t* m;
    list_t* list;

    if (sem_trywait(&q->sem_ready) != 0) return NULL;

    pthread_mutex_lock(&q->mutex);

    ASSERT(!list_is_empty(&q->list));

    list = q->list.next;
    list_remove(list);

    m = list_entry(list, link_msg_t);

    ASSERT(m->magic == MSG_MAGIC);

    pthread_mutex_unlock(&q->mutex);

    return m;
}


//------------------------------------------------------------------------------
// ������� ��������� �� ������� ���, ���� ������ ����� ��������� ��������� �
// ������� ��������� ��������� �������.
//      q  - ������� ���������;
//      ms - ����� �������� ��������� � ��;
// ���������� ���������� ���������, ���� NULL, ���� ���� ������� ��������.
//------------------------------------------------------------------------------
static link_msg_t * app_msg_q_wait(app_msg_q_t* q, u32_t ms)
{
    link_msg_t* m;
    list_t* list;
/*  struct timespec ts;
    u32_t ticks;

    ticks = clock() + ms*CLOCKS_PER_SEC/1000;

    ts.tv_sec  = ticks / CLOCKS_PER_SEC;
    ticks %= CLOCKS_PER_SEC;
    ts.tv_nsec = ticks * 1000000 / (CLOCKS_PER_SEC/1000);
*/
    if (sem_timedwait(&q->sem_ready, ms)) return NULL;

    pthread_mutex_lock(&q->mutex);

    ASSERT(!list_is_empty(&q->list));

    list = q->list.next;
    list_remove(list);

    m = list_entry(list, link_msg_t);

    ASSERT(m->magic == MSG_MAGIC);

    pthread_mutex_unlock(&q->mutex);

    return m;
}


//------------------------------------------------------------------------------
// �������� ������� ���������.
//------------------------------------------------------------------------------
static void app_msg_q_clear(app_msg_q_t* q)
{
    link_msg_t* m;

    while (m = app_msg_q_get(q)) link_msg_del(m);
}


//------------------------------------------------------------------------------
// ������������� ������ ������ � �������� ����� ������ �/�.
//------------------------------------------------------------------------------
void app_init()
{
    memset(&app, 0, sizeof(app)); 
    app_msg_q_init(&app.rep_q);
    sem_init(&app.sem_sleep, 0, 0);
    app.sleep = SLEEP_TIME_MS;
}

//------------------------------------------------------------------------------
// ��������� �������������� � ��������� ����� ������ �/�.
//------------------------------------------------------------------------------
int app_start()
{
    if (app.th == 0) {
   
        mlink_init(&app.link, 0);
        link_rx_machin_init(&app.rx_sm);

        link_start(&app.link); 
    
        //serial_open(APP_SERIAL_N, APP_SERIAL_CONF);
        serial_init(MDR_UART2);

        //pthread_create (&app.th, NULL, app_thread_rx, NULL);
        //pthread_detach(app.th);
        xTaskCreate(app_thread_rx, "APPTH", configMINIMAL_STACK_SIZE, NULL, 
                    tskIDLE_PRIORITY, &app.th);
   
        link_ph_layer_up(&app.link);
    }
    return 0;
}

//------------------------------------------------------------------------------
// ���������� �������������� � ��������� ����� ������ �/�.
//------------------------------------------------------------------------------
int app_stop()
{ 
    if (app.th) { 
        link_stop(&app.link);  
        serial_close(MDR_UART2);
        pthread_cancel (app.th);
        app.th = 0;
    }
    return 0;
}

//------------------------------------------------------------------------------
// �������� ���� �� �������� ������.
//     app    - ������������� ������;
//     rx_buf - ������ �� ������������ �����;
//     size   - ������ ������� ������� @data � ������.
//------------------------------------------------------------------------------
static int mlink_read(app_cb_t *app, u8_t *rx_buf, int size)
{
    int rc,len;
    list_t msg_list;
    link_msg_t *msg;
    
    list_init(&msg_list); 
    
    len = serial_read(rx_buf, size);
    if (!len) return 0;
      
    rc = mlink_rx_bytes(&app->rx_sm, rx_buf, len, &msg_list);
    if (rc) {
        msg = list_entry(msg_list.next, link_msg_t);
        list_remove(&msg->list);
        link_rx_msg(&app->link, msg);
        return 1;
    }
    return 0;
}


//------------------------------------------------------------------------------
// ����� �������� (��������� �������� �� ������� ������� ������,
// �������� ��������� � ��������� �� � ������ ���������� �������).
//------------------------------------------------------------------------------
static void  app_thread_rx(void* param)
{
    int rc;
    //struct timespec ts_sleep;
    //u32_t ticks;
    
    while (1) {
      
        rc = mlink_read(&app, app_rx_buf, sizeof(app_rx_buf));
        
        if (rc) {
                // ������� ����� ����
                app.stat.rx.frame++;
        }
        else {
            // �������� �� ����� app.sleep, ���� ���� �� �������� (��������
            // �������).
          /*
            ticks = clock() + app.sleep * CLOCKS_PER_SEC/1000;

            ts_sleep.tv_sec  = ticks / CLOCKS_PER_SEC;
            ticks %= CLOCKS_PER_SEC;
            ts_sleep.tv_nsec = ticks * 1000000 / (CLOCKS_PER_SEC/1000);
           */
            sem_timedwait(&app.sem_sleep, app.sleep);
        }
    }
}

void show_link_err()
{
    
    printf("\r\n������ �����\r\n"
           "������������ ���� - %d\r\n"
           "������������      - %d\r\n"
           "��������� ������� - %d\r\n"
           "��������� �����   - %d\r\n",
           app.rx_sm.cs,
           app.rx_sm.ovf,
           app.rx_sm.mark,
           app.rx_sm.trun);
}




//------------------------------------------------------------------------------
// ���������� ��������� ���������� ������ ������ ����� ��� �� ���� cab.
//------------------------------------------------------------------------------
int sync_cmd(u8_t cab)
{  
    link_msg_t *m;
    int i;
  
    m = link_msg_new(9);
    
    m->body[0] = APP_XSTART + cab;
    //if (rng_read(mask, sizeof(mask)) != sizeof(mask)) FATAL_ERROR();  
    for (i=0; i<sizeof(mask); i++)
        mask[i] = i+1;
    
    for (i=0; i<8; i++)
        m->body[i+1] = mask[i];
    
    pthread_mutex_lock(&app.mutex);
    
    // ������� ������� ���������� �������
    app_msg_q_clear(&app.rep_q);
  
    i = link_put_msg(&app.link, m);
    print_result(i);
    
    if (i == APP_E_OK) {   
        // ��������� ����� ������ � ��������� ����� ������, ���� �� ����.
        app.sleep = 1;
        sem_post(&app.sem_sleep);
        
        m = app_msg_q_wait(&app.rep_q, APP_XSTART_TIME_MS);
        app.sleep = SLEEP_TIME_MS;
        
        if (m != NULL) {

            PRINT_FRAME(m)

            if ((m->body[0] & 0xF0) == APP_XSTART) {
                if ((m->body[0] & 0x0F) == 0) {
                    for (i=0; i<8; i++)
                        mask[i] ^= m->body[i+1];
                    i = APP_E_OK;
                }
                else i = m->body[0] & 0x0F;
            }
            else i = APP_E_UNEXPECT;
          
            link_msg_del(m); 
        }
        else i = APP_E_TIME; 
    }
    pthread_mutex_unlock(&app.mutex);
    return i;
}
  
//------------------------------------------------------------------------------
// ���������� ��������� ���������� ������ ����� � ��������� ��� �� ���� cab.
//------------------------------------------------------------------------------
int loop_cmd(u8_t cab)
{ 
    link_msg_t *m;
    int i;
    u8_t lb_buf[16];
  
    m = link_msg_new(17);
  
    m->body[0] = APP_INLPBACK + cab;
    //if (rng_read(lb_buf,16) != 16) FATAL_ERROR();  
    for (i=0; i<16; i++)
        lb_buf[i] = i+3;
    
    for (i=0; i<16; i++)
        m->body[i+1] ^= lb_buf[i]^mask[i%8];
    
    pthread_mutex_lock(&app.mutex);
    
    // ������� ������� ���������� �������
    app_msg_q_clear(&app.rep_q);
  
    i = link_put_msg(&app.link, m);
    print_result(i);
  
    if (i == APP_E_OK) {
        // ��������� ����� ������ � ��������� ����� ������, ���� �� ����.
        app.sleep = 1;
        sem_post(&app.sem_sleep);
        
        m = app_msg_q_wait(&app.rep_q, APP_INLPBACK_TIME_MS);
        app.sleep = SLEEP_TIME_MS;
        
        if (m != NULL) {
        
            PRINT_FRAME(m)
            PRINT_UNMASKED(m)
                
            if ((m->body[0] & 0xF0) == APP_INLPBACK) {
                if ((m->body[0] & 0x0F) == 0) {
                    for (i=0; i<16; i++) {
                        if ((m->body[i+1]^mask[i%8]^lb_buf[i]) != 0xFF)
                            break;
                    }
                    if (i == 16) i = APP_E_OK;
                    else i = APP_E_MISMATCH;
                }
                else i = m->body[0] & 0x0F;
            }
            else i = APP_E_UNEXPECT;
          
            link_msg_del(m);
        }
        else i = APP_E_TIME;  
    }
    pthread_mutex_unlock(&app.mutex);
    return i;
}
 
//------------------------------------------------------------------------------
// ���������� ��������� ���������� ������ ������ ���������� ��� �� ���� cab
// � ��������� ���������� � ������ ������ � ����� props.
//------------------------------------------------------------------------------
int props_cmd(u8_t cab, u8_t *props) 
{  
    link_msg_t *m;
    int i,N;
    union _sp{
        u32_t D;
        u8_t B[4];
    } KS;
  
    m = link_msg_new(1);
    m->body[0] = APP_GETPROPS + cab;
    
    pthread_mutex_lock(&app.mutex);
    
    // ������� ������� ���������� �������
    app_msg_q_clear(&app.rep_q);
  
    i = link_put_msg(&app.link, m);
    print_result(i);

    if (i == APP_E_OK) {
        // ��������� ����� ������ � ��������� ����� ������, ���� �� ����.
        app.sleep = 1;
        sem_post(&app.sem_sleep);
        
        m = app_msg_q_wait(&app.rep_q, APP_GETPROPS_TIME_MS);
        app.sleep = SLEEP_TIME_MS;
        
        if (m != NULL) {

            PRINT_FRAME(m)
            PRINT_UNMASKED(m) 
         
            if ((m->body[0] & 0xF0) == APP_GETPROPS) { 
                if ((m->body[0] & 0x0F) == 0) {
                    N = m->len;
                    for (i=0; i<N-1; i++)
                        m->body[i+1] ^= mask[i%8];
                
                    KS.D = get_ksum_l(N-4,&m->body[0]);
                    i = KS.B[0]^m->body[N-4] | KS.B[1]^m->body[N-3] | 
                        KS.B[2]^m->body[N-2] | KS.B[3]^m->body[N-1];
                
                    if (i) i = APP_E_DIR_CF;  
                    else {
#ifdef AUXILIARY_FUNC
                        memset(props,0xFF,14);  // ���� ���������� ����� �����
#endif 
                        for (i=0; i<N-5; i++)
                            props[i] = m->body[i+1];    
                        i = APP_E_OK;
                    }
                }
                else i = m->body[0] & 0x0F;
            }
            else i = APP_E_UNEXPECT;
          
            link_msg_del(m);
        }
        else i = APP_E_TIME;
    } 
    pthread_mutex_unlock(&app.mutex); 
    return i;
}

//------------------------------------------------------------------------------
// ���������� ��������� ���������� ������ ��������� ����� ��� �� ���� cab.
//------------------------------------------------------------------------------
int end_cmd(u8_t cab)
{  
    link_msg_t *m;
    int i;
  
    m = link_msg_new(1);
    m->body[0] = APP_XSTOP + cab;
    
    pthread_mutex_lock(&app.mutex);
    
    // ������� ������� ���������� �������
    app_msg_q_clear(&app.rep_q);
  
    i = link_put_msg(&app.link, m);
    print_result(i);

    if (i == APP_E_OK) {
        // ��������� ����� ������ � ��������� ����� ������, ���� �� ����.
        app.sleep = 1;
        sem_post(&app.sem_sleep);
        
        m = app_msg_q_wait(&app.rep_q, APP_XSTOP_TIME_MS);
        app.sleep = SLEEP_TIME_MS;
        
        if (m != NULL) {

            PRINT_FRAME(m)
            PRINT_UNMASKED(m) 
          
            if ((m->body[0] & 0xF0) == APP_XSTOP) {
                if ((m->body[0] & 0x0F) == 0)
                    i = APP_E_OK;
                else
                    i = m->body[0] & 0x0F;
            }
            else i = APP_E_UNEXPECT;
          
            link_msg_del(m);
        }
        else i = APP_E_TIME;
    }
    pthread_mutex_unlock(&app.mutex);  
    return i;
}




