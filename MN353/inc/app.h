
#include <stdint.h>
//#include <config.h>
/*
#define malloc	_New
#define free	_Free
*/
#define APP_MAX_BLOCK   2030

// Типы агентов безопасности (КАБ)

#ifdef KULC_4SA

#define SA_KULON_C1     0
#define SA_KULON_C2     1
#define SA_KULON_C3     2
#define SA_KULON_C4     3

#else

#define SA_KULON_C      0

#endif

#define SA_KULON_A      4
#define SA_SCC          5
#define SA_SKZI_C       8
#define SA_SKZI_S1_C    9
#define SA_SKZI_E1_C    10
#define SA_SKZI_IP_C    11
#define SA_SKZI_A       12
#define SA_SKZI_S1_A    13
#define SA_SKZI_E1_A    14
#define SA_SKZI_IP_A    15

// Директивы сеансового уровня

#define APP_XSTART      0x00    // Начать обмен
#define APP_INLPBACK    0x10    // Шлейф с инверсией
#define APP_GETPROPS    0x20    // Сообщить реквизиты
#define APP_TAKEPARAM   0x30    // Принять параметр
#define APP_VERIFYDS    0x40    // Проверить ЭЦП
#define APP_GIVEPARAM   0x50    // Передать параметр
#define APP_VERIFYINT   0x60    // Проверить целостность
#define APP_UKEYSTATE   0x70    // Состояние ключа НСД
#define APP_XSTOP       0x80    // Закончить обмен
#define APP_SETNUM      0xA0    // Установить номер

// Ошибки обмена сеансового уровня

#define APP_E_OK 	0
#define APP_E_NO_AGENT  1       // Отсутствие требуемого агента
#define APP_E_NO_SYNC   2       // Отсутствие синхронизации
#define APP_E_DIR_CF    3       // Ошибка КПД
#define APP_E_PAR_CF    5       // Ошибка КПП
#define APP_E_OP_CHAIN  6       // Недопустимая последовательность действий
#define APP_E_BUSY      7       // ЗУ параметров занято
#define APP_E_WRITE     8       // Запись параметра не прошла
#define APP_E_DECRYPT   9       // Ошибка расшифрования
#define APP_E_NO_PARAM  10      // Отсутствие требуемого параметра
#define APP_E_PILLEGAL  13      // Недопустимый тип параметра 
#define APP_E_NO_UKEY   12      // Не введен ключ НСД
#define APP_E_RESERVED  15      // Резервный код ошибки

// Для приема из ЦУБ, т.к. расшифрования нет, а контрольная
// сумма на файлы есть
#define APP_E_CHECKF  APP_E_DECRYPT

// Дополнение для ЦУБ в режиме ведомого (проверка целостности ПО)
#define SCC_E_ACCEPT    APP_E_WRITE
#define SCC_E_BUSY      APP_E_BUSY

#define APP_E_NO_LINK	16
#define APP_E_RX_BUSY	17
#define APP_E_TX_BUSY	18
#define APP_E_UNEXPECT  19
#define APP_E_MISMATCH  20
#define APP_E_TIME      21
#define APP_E_ACK_CF    22       // Ошибка КПК
#define APP_E_SIZE      23

// Таймауты исполнения директив сеансового уровня

#define APP_XSTART_TIME_MS      2000
#define APP_INLPBACK_TIME_MS    2000
#define APP_GETPROPS_TIME_MS    3000
#define APP_XSTOP_TIME_MS       2000
#define APP_TAKEPARAM_TIME_MS   8000
#define APP_GIVEPARAM_TIME_MS   5000
#define APP_UKEYSTATE_TIME_MS   5000
#define APP_VERIFYINT_TIME_MS   75000
#define SCC_VERIFYINT_TIME_MS   1000
#define APP_SETNUM_TIME_MS      5000

// Типы параметров

#define PAR_CG_INIT     0x15    // КГ КНР
#define PAR_CG_WORK     0x25    // Рабочая КГ
#define PAR_CG_SOFT     0x3F    // КГ СПО
#define PAR_KTRANS      0x35    // Транспортировочный ключ
#define PAR_PKEY        0x50    // Открытый ключ проверки ЭЦП
#define PAR_DSIGN       0x60    // ЭЦП
#define PAR_FSERV       0x61    // Служебный файл транспортировочного РИК-2
#define PAR_VECTOR      0x62    // Вектор НСД файл транспортировочного РИК-2
#define PAR_UKEY        0x63    // Ключ НСД
#define PAR_LOG         0x64    // Журнал
#define PAR_CNLIST      0x70    // Скомпрометированный номер

// Типы ключей защиты от НСД
#define UAA_KULON_C     0x32
#define UAA_KULON_A     0x31
#define UAA_SCC         0x33
#define UAA_UA353       0x34
#define UAA_SKZI        0x35

typedef struct PAR_PROPS {
    u8_t  type;                 // Тип
    u8_t  shift;                // Смена/назначение
    u16_t chain;                // Номер цепи
    u8_t  year;                 // Год изготовления
    u16_t series;               // Номер серии
} par_props_t;


typedef struct {
        char *name;     // Имя аппарата
        u8_t sa_id;     // Код АБ
        u8_t ukey;      // Тип ключа НСД
        char *label;    // Метка для журнала
} n2id_t;

#define DEV_LIST_SIZE 13

extern void app_init();
extern int app_start();
extern int app_stop();
extern int sync_cmd(u8_t cab);
extern int loop_cmd(u8_t cab);
extern int props_cmd(u8_t cab, u8_t *props);
extern int end_cmd(u8_t cab);
extern int send_param_cmd(u8_t cab, par_props_t *props, u8_t *data, int size);
extern int send_soft_cmd(u8_t cab, par_props_t *props, u8_t *data, int size);
extern int verify_cmd(u8_t cab);
extern int recv_param_cmd(u8_t cab, u8_t type, u8_t *form, u8_t *data, int *size);
extern int ukstat_cmd(u8_t cab, u8_t *data);
extern int icheck_cmd(u8_t cab, u16_t mod, u8_t *data);
extern int setnum_cmd(u8_t cab, u16_t sn);

#define APP_DEBUG_PRINT

#ifdef APP_DEBUG_PRINT  

#define PRINT_FRAME(m)  printf("\t->"); \
                        for (i=0; i<m->len; i++) \
                            printf("%02x ",m->body[i]); \
                        printf("\n");

#define PRINT_UNMASKED(m) if (m->len > 1) { \
                          printf("\t     "); \
                          for (i=1; i<m->len; i++) \
                              printf("%02x ",m->body[i]^mask[(i-1)%8]); \
                          printf("\n"); \
                          };   

//#define dbg_printf(...) printf(__VA_ARGS__)

#else

#define PRINT_FRAME(m)
#define PRINT_UNMASKED(m)
#define dbg_printf(...) 

#endif



  


