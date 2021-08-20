/*
#include "stdint.h"
#include "ks.h"
#include "restart.h"
#include <rng.h>
*/
#include <types.h>
#include <ks.h>

static const u8_t SKS[255] = /* ТАБЛИЦА СТЕПЕНЕЙ S */
   {   1,   2,   4,   8,  16,  32,  64, 128,  29,  58,
     116, 232, 205, 135,  19,  38,  76, 152,  45,  90,
     180, 117, 234, 201, 143,   3,   6,  12,  24,  48,
      96, 192, 157,  39,  78, 156,  37,  74, 148,  53,
     106, 212, 181, 119, 238, 193, 159,  35,  70, 140,
       5,  10,  20,  40,  80, 160,  93, 186, 105, 210,
     185, 111, 222, 161,  95, 190,  97, 194, 153,  47,
      94, 188, 101, 202, 137,  15,  30,  60, 120, 240,
     253, 231, 211, 187, 107, 214, 177, 127, 254, 225,
     223, 163,  91, 182, 113, 226, 217, 175,  67, 134,
      17,  34,  68, 136,  13,  26,  52, 104, 208, 189,
     103, 206, 129,  31,  62, 124, 248, 237, 199, 147,
      59, 118, 236, 197, 151,  51, 102, 204, 133,  23,
      46,  92, 184, 109, 218, 169,  79, 158,  33,  66,
     132,  21,  42,  84, 168,  77, 154,  41,  82, 164,
      85, 170,  73, 146,  57, 114, 228, 213, 183, 115,
     230, 209, 191,  99, 198, 145,  63, 126, 252, 229,
     215, 179, 123, 246, 241, 255, 227, 219, 171,  75,
     150,  49,  98, 196, 149,  55, 110, 220, 165,  87,
     174,  65, 130,  25,  50, 100, 200, 141,   7,  14,
      28,  56, 112, 224, 221, 167,  83, 166,  81, 162,
      89, 178, 121, 242, 249, 239, 195, 155,  43,  86,
     172,  69, 138,   9,  18,  36,  72, 144,  61, 122,
     244, 245, 247, 243, 251, 235, 203, 139,  11,  22,
      44,  88, 176, 125, 250, 233, 207, 131,  27,  54,
     108, 216, 173,  71, 142};

static const u8_t LKS[256] =   /* ТАБЛИЦА ЛОГАРИФМОВ L */
/* в качестве нулевого элемента массива добавлен лишний 0 */
  { 0,   0,   1,  25,   2,  50,  26, 198,   3, 223,  51,
  238,  27, 104, 199,  75,   4, 100, 224,  14,  52,
  141, 239, 129,  28, 193, 105, 248, 200,   8,  76,
  113,   5, 138, 101,  47, 225,  36,  15,  33,  53,
  147, 142, 218, 240,  18, 130,  69,  29, 181, 194,
  125, 106,  39, 249, 185, 201, 154,   9, 120,  77,
  228, 114, 166,   6, 191, 139,  98, 102, 221,  48,
  253, 226, 152,  37, 179,  16, 145,  34, 136,  54,
  208, 148, 206, 143, 150, 219, 189, 241, 210,  19,
   92, 131,  56,  70,  64,  30,  66, 182, 163, 195,
   72, 126, 110, 107,  58,  40,  84, 250, 133, 186,
   61, 202,  94, 155, 159,  10,  21, 121,  43,  78,
  212, 229, 172, 115, 243, 167,  87,   7, 112, 192,
  247, 140, 128,  99,  13, 103,  74, 222, 237,  49,
  197, 254,  24, 227, 165, 153, 119,  38, 184, 180,
  124,  17,  68, 146, 217,  35,  32, 137,  46,  55,
   63, 209,  91, 149, 188, 207, 205, 144, 135, 151,
  178, 220, 252, 190,  97, 242,  86, 211, 171,  20,
   42,  93, 158, 132,  60,  57,  83,  71, 109,  65,
  162,  31,  45,  67, 216, 183, 123, 164, 118, 196,
   23,  73, 236, 127,  12, 111, 246, 108, 161,  59,
   82,  41, 157,  85, 170, 251,  96, 134, 177, 187,
  204,  62,  90, 203,  89,  95, 176, 156, 169, 160,
   81,  11, 245,  22, 235, 122, 117,  44, 215,  79,
  174, 213, 233, 230, 231, 173, 232, 116, 214, 244,
  234, 168,  80,  88, 175                        };


#if 0

/********************************************************/
/*     		Таблицы для sys_get_fcs			*/
/********************************************************/

//Версия 07-09-01 Готово к ГИ
/* Проверочный полином для вычисления FCS                              */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Записан в обратном порядке: старшая степень - наименее значаший бит */
static const unsigned long  cr3tab[] = {/*                 CRC полином    0xedb88320*/
	0x00000000l, 0x77073096l, 0xee0e612cl, 0x990951bal, 0x076dc419l, 0x706af48fl, 0xe963a535l, 0x9e6495a3l,
	0x0edb8832l, 0x79dcb8a4l, 0xe0d5e91el, 0x97d2d988l, 0x09b64c2bl, 0x7eb17cbdl, 0xe7b82d07l, 0x90bf1d91l,
	0x1db71064l, 0x6ab020f2l, 0xf3b97148l, 0x84be41del, 0x1adad47dl, 0x6ddde4ebl, 0xf4d4b551l, 0x83d385c7l,
	0x136c9856l, 0x646ba8c0l, 0xfd62f97al, 0x8a65c9ecl, 0x14015c4fl, 0x63066cd9l, 0xfa0f3d63l, 0x8d080df5l,
	0x3b6e20c8l, 0x4c69105el, 0xd56041e4l, 0xa2677172l, 0x3c03e4d1l, 0x4b04d447l, 0xd20d85fdl, 0xa50ab56bl,
	0x35b5a8fal, 0x42b2986cl, 0xdbbbc9d6l, 0xacbcf940l, 0x32d86ce3l, 0x45df5c75l, 0xdcd60dcfl, 0xabd13d59l,
	0x26d930acl, 0x51de003al, 0xc8d75180l, 0xbfd06116l, 0x21b4f4b5l, 0x56b3c423l, 0xcfba9599l, 0xb8bda50fl,
	0x2802b89el, 0x5f058808l, 0xc60cd9b2l, 0xb10be924l, 0x2f6f7c87l, 0x58684c11l, 0xc1611dabl, 0xb6662d3dl,
	0x76dc4190l, 0x01db7106l, 0x98d220bcl, 0xefd5102al, 0x71b18589l, 0x06b6b51fl, 0x9fbfe4a5l, 0xe8b8d433l,
	0x7807c9a2l, 0x0f00f934l, 0x9609a88el, 0xe10e9818l, 0x7f6a0dbbl, 0x086d3d2dl, 0x91646c97l, 0xe6635c01l,
	0x6b6b51f4l, 0x1c6c6162l, 0x856530d8l, 0xf262004el, 0x6c0695edl, 0x1b01a57bl, 0x8208f4c1l, 0xf50fc457l,
	0x65b0d9c6l, 0x12b7e950l, 0x8bbeb8eal, 0xfcb9887cl, 0x62dd1ddfl, 0x15da2d49l, 0x8cd37cf3l, 0xfbd44c65l,
	0x4db26158l, 0x3ab551cel, 0xa3bc0074l, 0xd4bb30e2l, 0x4adfa541l, 0x3dd895d7l, 0xa4d1c46dl, 0xd3d6f4fbl,
	0x4369e96al, 0x346ed9fcl, 0xad678846l, 0xda60b8d0l, 0x44042d73l, 0x33031de5l, 0xaa0a4c5fl, 0xdd0d7cc9l,
	0x5005713cl, 0x270241aal, 0xbe0b1010l, 0xc90c2086l, 0x5768b525l, 0x206f85b3l, 0xb966d409l, 0xce61e49fl,
	0x5edef90el, 0x29d9c998l, 0xb0d09822l, 0xc7d7a8b4l, 0x59b33d17l, 0x2eb40d81l, 0xb7bd5c3bl, 0xc0ba6cadl,
	0xedb88320l, 0x9abfb3b6l, 0x03b6e20cl, 0x74b1d29al, 0xead54739l, 0x9dd277afl, 0x04db2615l, 0x73dc1683l,
	0xe3630b12l, 0x94643b84l, 0x0d6d6a3el, 0x7a6a5aa8l, 0xe40ecf0bl, 0x9309ff9dl, 0x0a00ae27l, 0x7d079eb1l,
	0xf00f9344l, 0x8708a3d2l, 0x1e01f268l, 0x6906c2fel, 0xf762575dl, 0x806567cbl, 0x196c3671l, 0x6e6b06e7l,
	0xfed41b76l, 0x89d32be0l, 0x10da7a5al, 0x67dd4accl, 0xf9b9df6fl, 0x8ebeeff9l, 0x17b7be43l, 0x60b08ed5l,
	0xd6d6a3e8l, 0xa1d1937el, 0x38d8c2c4l, 0x4fdff252l, 0xd1bb67f1l, 0xa6bc5767l, 0x3fb506ddl, 0x48b2364bl,
	0xd80d2bdal, 0xaf0a1b4cl, 0x36034af6l, 0x41047a60l, 0xdf60efc3l, 0xa867df55l, 0x316e8eefl, 0x4669be79l,
	0xcb61b38cl, 0xbc66831al, 0x256fd2a0l, 0x5268e236l, 0xcc0c7795l, 0xbb0b4703l, 0x220216b9l, 0x5505262fl,
	0xc5ba3bbel, 0xb2bd0b28l, 0x2bb45a92l, 0x5cb36a04l, 0xc2d7ffa7l, 0xb5d0cf31l, 0x2cd99e8bl, 0x5bdeae1dl,
	0x9b64c2b0l, 0xec63f226l, 0x756aa39cl, 0x026d930al, 0x9c0906a9l, 0xeb0e363fl, 0x72076785l, 0x05005713l,
	0x95bf4a82l, 0xe2b87a14l, 0x7bb12bael, 0x0cb61b38l, 0x92d28e9bl, 0xe5d5be0dl, 0x7cdcefb7l, 0x0bdbdf21l,
	0x86d3d2d4l, 0xf1d4e242l, 0x68ddb3f8l, 0x1fda836el, 0x81be16cdl, 0xf6b9265bl, 0x6fb077e1l, 0x18b74777l,
	0x88085ae6l, 0xff0f6a70l, 0x66063bcal, 0x11010b5cl, 0x8f659effl, 0xf862ae69l, 0x616bffd3l, 0x166ccf45l,
	0xa00ae278l, 0xd70dd2eel, 0x4e048354l, 0x3903b3c2l, 0xa7672661l, 0xd06016f7l, 0x4969474dl, 0x3e6e77dbl,
	0xaed16a4al, 0xd9d65adcl, 0x40df0b66l, 0x37d83bf0l, 0xa9bcae53l, 0xdebb9ec5l, 0x47b2cf7fl, 0x30b5ffe9l,
	0xbdbdf21cl, 0xcabac28al, 0x53b39330l, 0x24b4a3a6l, 0xbad03605l, 0xcdd70693l, 0x54de5729l, 0x23d967bfl,
	0xb3667a2el, 0xc4614ab8l, 0x5d681b02l, 0x2a6f2b94l, 0xb40bbe37l, 0xc30c8ea1l, 0x5a05df1bl, 0x2d02ef8dl
}; 


// Полином 0x8408 (0x11021)
const unsigned short int fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

const unsigned short fcstab_x25[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/*-----------------------------------------------------------------------------------
  Таблица остатков от деления (X^32)*C на полином 0x1143F3DD6, где C=0...255
  -----------------------------------------------------------------------------------*/
static const unsigned long restab[]={
0x00000000l,0x143f3dd9l,0x287e7bb2l,0x3c41466bl,0x50fcf764l,0x44c3cabdl,0x78828cd6l,0x6cbdb10fl,
0xa1f9eec8l,0xb5c6d311l,0x8987957al,0x9db8a8a3l,0xf10519acl,0xe53a2475l,0xd97b621el,0xcd445fc7l,
0x57cce049l,0x43f3dd90l,0x7fb29bfbl,0x6b8da622l,0x0730172dl,0x130f2af4l,0x2f4e6c9fl,0x3b715146l,
0xf6350e81l,0xe20a3358l,0xde4b7533l,0xca7448eal,0xa6c9f9e5l,0xb2f6c43cl,0x8eb78257l,0x9a88bf8el,
0xaf99c092l,0xbba6fd4bl,0x87e7bb20l,0x93d886f9l,0xff6537f6l,0xeb5a0a2fl,0xd71b4c44l,0xc324719dl,
0x0e602e5al,0x1a5f1383l,0x261e55e8l,0x32216831l,0x5e9cd93el,0x4aa3e4e7l,0x76e2a28cl,0x62dd9f55l,
0xf85520dbl,0xec6a1d02l,0xd02b5b69l,0xc41466b0l,0xa8a9d7bfl,0xbc96ea66l,0x80d7ac0dl,0x94e891d4l,
0x59acce13l,0x4d93f3cal,0x71d2b5a1l,0x65ed8878l,0x09503977l,0x1d6f04ael,0x212e42c5l,0x35117f1cl,
0x4b0cbcfdl,0x5f338124l,0x6372c74fl,0x774dfa96l,0x1bf04b99l,0x0fcf7640l,0x338e302bl,0x27b10df2l,
0xeaf55235l,0xfeca6fecl,0xc28b2987l,0xd6b4145el,0xba09a551l,0xae369888l,0x9277dee3l,0x8648e33al,
0x1cc05cb4l,0x08ff616dl,0x34be2706l,0x20811adfl,0x4c3cabd0l,0x58039609l,0x6442d062l,0x707dedbbl,
0xbd39b27cl,0xa9068fa5l,0x9547c9cel,0x8178f417l,0xedc54518l,0xf9fa78c1l,0xc5bb3eaal,0xd1840373l,
0xe4957c6fl,0xf0aa41b6l,0xcceb07ddl,0xd8d43a04l,0xb4698b0bl,0xa056b6d2l,0x9c17f0b9l,0x8828cd60l,
0x456c92a7l,0x5153af7el,0x6d12e915l,0x792dd4ccl,0x159065c3l,0x01af581al,0x3dee1e71l,0x29d123a8l,
0xb3599c26l,0xa766a1ffl,0x9b27e794l,0x8f18da4dl,0xe3a56b42l,0xf79a569bl,0xcbdb10f0l,0xdfe42d29l,
0x12a072eel,0x069f4f37l,0x3ade095cl,0x2ee13485l,0x425c858al,0x5663b853l,0x6a22fe38l,0x7e1dc3e1l,
0x961979fal,0x82264423l,0xbe670248l,0xaa583f91l,0xc6e58e9el,0xd2dab347l,0xee9bf52cl,0xfaa4c8f5l,
0x37e09732l,0x23dfaaebl,0x1f9eec80l,0x0ba1d159l,0x671c6056l,0x73235d8fl,0x4f621be4l,0x5b5d263dl,
0xc1d599b3l,0xd5eaa46al,0xe9abe201l,0xfd94dfd8l,0x91296ed7l,0x8516530el,0xb9571565l,0xad6828bcl,
0x602c777bl,0x74134aa2l,0x48520cc9l,0x5c6d3110l,0x30d0801fl,0x24efbdc6l,0x18aefbadl,0x0c91c674l,
0x3980b968l,0x2dbf84b1l,0x11fec2dal,0x05c1ff03l,0x697c4e0cl,0x7d4373d5l,0x410235bel,0x553d0867l,
0x987957a0l,0x8c466a79l,0xb0072c12l,0xa43811cbl,0xc885a0c4l,0xdcba9d1dl,0xe0fbdb76l,0xf4c4e6afl,
0x6e4c5921l,0x7a7364f8l,0x46322293l,0x520d1f4al,0x3eb0ae45l,0x2a8f939cl,0x16ced5f7l,0x02f1e82el,
0xcfb5b7e9l,0xdb8a8a30l,0xe7cbcc5bl,0xf3f4f182l,0x9f49408dl,0x8b767d54l,0xb7373b3fl,0xa30806e6l,
0xdd15c507l,0xc92af8del,0xf56bbeb5l,0xe154836cl,0x8de93263l,0x99d60fbal,0xa59749d1l,0xb1a87408l,
0x7cec2bcfl,0x68d31616l,0x5492507dl,0x40ad6da4l,0x2c10dcabl,0x382fe172l,0x046ea719l,0x10519ac0l,
0x8ad9254el,0x9ee61897l,0xa2a75efcl,0xb6986325l,0xda25d22al,0xce1aeff3l,0xf25ba998l,0xe6649441l,
0x2b20cb86l,0x3f1ff65fl,0x035eb034l,0x17618dedl,0x7bdc3ce2l,0x6fe3013bl,0x53a24750l,0x479d7a89l,
0x728c0595l,0x66b3384cl,0x5af27e27l,0x4ecd43fel,0x2270f2f1l,0x364fcf28l,0x0a0e8943l,0x1e31b49al,
0xd375eb5dl,0xc74ad684l,0xfb0b90efl,0xef34ad36l,0x83891c39l,0x97b621e0l,0xabf7678bl,0xbfc85a52l,
0x2540e5dcl,0x317fd805l,0x0d3e9e6el,0x1901a3b7l,0x75bc12b8l,0x61832f61l,0x5dc2690al,0x49fd54d3l,
0x84b90b14l,0x908636cdl,0xacc770a6l,0xb8f84d7fl,0xd445fc70l,0xc07ac1a9l,0xfc3b87c2l,0xe804ba1bl
};

#define MAX_PERM_SIZE 0x8000 // максимальная длина подстановки

static uint16_t _noise[MAX_PERM_SIZE];
static uint16_t _os_perm[MAX_PERM_SIZE];

#endif /* 0 */

// ОПИСАНИЕ: вычисление контрольной суммы Ларина.
uint32_t get_ksum_l(unsigned int len, const void* vsource)
{
    union _sp{
		uint32_t D;
		uint8_t B[4];
    }SK;
    unsigned int i;
	const uint8_t * const source = (uint8_t*)vsource;
    
	SK.D = 0;
    
    for(i=0; i<len; i++)
	{
		const uint8_t b = source[i]; 
		if (b)
		{
			SK.B[0] ^= b;
			SK.B[1] ^= SKS[(LKS[b]+i+1)%255];
			SK.B[2] ^= SKS[(LKS[b]+((i+1)%len)+1)%255];
			SK.B[3] ^= SKS[(LKS[b]+len-i)%255];
		}
    }
    
	return SK.D;
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// ОПИСАНИЕ: вычисления контрольной суммы Ларина по подстановке.
// АРГУМЕНТЫ:
// @len - длина информации в байтах (не больше 255);
// @data - информация;
// ВОЗВРАЩАЕТ: контрольная сумма.
u32_t get_ksum_l_subst(u32_t len, const void* data)
{
    union _sp{
		uint32_t D;
		uint8_t B[4];
    }SK;

    unsigned int i;
    u8_t b;
	u8_t* const d = (u8_t*)data;
	
	if (len > MAX_PERM_SIZE) FATAL_ERROR();
    
	if (rng_read(_noise, len*2) != (len*2)) FATAL_ERROR();
	
    i = sys_get_perm_e(len, _os_perm, _noise);
    if (i != len) restart("Ошибка получения подстановки");
    
	SK.D = 0;
    
    for(i=0; i<len; i++)
	{
		b = d[_os_perm[i]];
		
		if (b)
		{
			SK.B[0] ^= b;
			SK.B[1] ^= SKS[(LKS[b] +_os_perm[i] + 1) % 255];
			SK.B[2] ^= SKS[(LKS[b] + ((_os_perm[i] + 1) % len) + 1) % 255];
			SK.B[3] ^= SKS[(LKS[b] + len -_os_perm[i]) % 255];
		}
    }
    return SK.D;
}

// ОПИСАНИЕ: проверка контрольной суммы Ларина для информации, представленной 
// ввиде двух компонентов.
// АРГУМЕНТЫ:
// @len - длина каждого компонента в байтах без учёта КС(не больше 255);
// @с1 - компонент №1 + его КС;
// @с2 - компонент №2 + его КС;
// ВОЗВРАЩАЕТ: 0 - норма, иначе ошибка КС.
int check_ksum_l(u32_t len, const void* v1, const void* v2)
{
    union _sp{
		u32_t D;
		u8_t B[4];
    }ks, nks;
    u32_t i;
	u8_t* const c1 = (u8_t*)v1;
	u8_t* const c2 = (u8_t*)v2;
	u8_t n[256];
	
	if (len > 255) FATAL_ERROR();
	// Запрос шума
	if (rng_read(n, len) != len) FATAL_ERROR(); 
    // Вычисление КС шума
	nks.D = get_ksum_l(len, n);
	// Для того чтобы не объединять компонент №1 и компонент №2 в открытом
	// виде, объединяем их с шумом.
	// Шум + компонент №1
	for(i=0; i<len; i++) n[i] ^= c1[i];
	// Шум + компонент №1 + компонент №2
	for(i=0; i<len; i++) n[i] ^= c2[i];
	// Вычисление КС[Шум + компонент №1 + компонент №2]
	ks.D = get_ksum_l_subst(len, n);
	
	// Вычисление КС[шум] + КС[Компонент №1] + КС[Компонент №2]
	nks.B[0] ^= c1[len + 0];
	nks.B[1] ^= c1[len + 1];
	nks.B[2] ^= c1[len + 2];
	nks.B[3] ^= c1[len + 3];
	
	nks.B[0] ^= c2[len + 0];
	nks.B[1] ^= c2[len + 1];
	nks.B[2] ^= c2[len + 2];
	nks.B[3] ^= c2[len + 3];
	
	// Проверка:
	// КС[шум] + КС[Компонент №1] + КС[Компонент №2] ==
	// == КС[шум + Компонент №1 + Компонент №2]
	
	if (ks.D == nks.D) return 0;

    return -1;
}

// ОПИСАНИЕ: вычисление КС Флетчера.
void rz_get_ks(const void* data, u16_t len, u8_t* calc_ks){
	int i, c0 = 0, c1 = 0;
	const u8_t* const ptr = (u8_t*)data;
	int t;
	for (i=0; i<len; i++)
	{
		c0 = (c0 + ptr[i]) % 255;
		c1 = (c1 + c0) % 255;
	}
	t = (c0 + c1) % 255;
	calc_ks[0] = (255 - t) % 255;
	calc_ks[1] = c1 % 255;
}

// ОПИСАНИЕ: проверить КС Флетчера (КС - последние 2 байта).
// ВОЗВРАЩАЕТ: 0 - успех, иначе ошибка КС.
int rz_check_ks(const void* data, u16_t len){
	int i, c0 = 0, c1 = 0;
	const u8_t* const ptr = (u8_t*)data;
	for (i=0; i<len; i++)
	{
		c0 = (c0 + ptr[i]) % 255;
		c1 = (c1 + c0) % 255;
	}
	return ((c0 == 0) && (c1 == 0)) ? 0 : 0x0F;
}

// ОПИСАНИЕ: вычисление CRC16.
u16_t CRC16(u16_t fcs, const void* data, u32_t size )
{
	const u8_t *ptr = (const u8_t*)data;
	while(size)
	{
		fcs = FCS(fcs, *ptr++);
		size--;
	}
	return fcs;
}

// ОПИСАНИЕ: вычисление CRC16 X25.
u16_t CRC16_X25(const void* data, u32_t size )
{
	const u8_t *ptr = (const u8_t*)data;
	u16_t fcs = 0xFFFF;

	while(size)
	{
		fcs = FCS_X25(fcs, *ptr++);
		size--;
	}
	return ~fcs;
}

// ОПИСАНИЕ: подсчёт контрольной суммы CRC32.
// АРГУМЕНТЫ:
// @start - адрес начала;
// @end - адрес последнего байта, для которого подсчитывается
// КС (включительно).
u32_t CRC32( u8_t *start, u8_t *end )
{
	u32_t j, crc;
	u8_t b;
  
  for ( crc = 0x0; start <= end; start++ ) 
	{
     b = *start;
     for (j=0;j<8;j++) 
		 {
        if (((b&0x80)!=0)&&((crc & 0x80000000)==0)||
            ((b&0x80)==0)&&((crc & 0x80000000)!=0))
           crc = (crc<<1)^0x04C11DB7;
        else 
           crc <<= 1;
        b <<= 1;
     }
  }
	return crc;
}

// ОПИСАНИЕ: подсчёт контрольной суммы CRC32.
// АРГУМЕНТЫ:
// @start - адрес начала;
// @end - адрес последнего байта, для которого подсчитывается
// КС (включительно).
u32_t crc32(u32_t crc, const void* data, u32_t size)
{
	u32_t i, j;
	u8_t b;  
 	for (i=0; i < size; i++ ) 
	{
		b = ((u8_t*)data)[i];
		
     	for (j=0;j<8;j++) 
		{
			if (((b&0x80)!=0)&&((crc & 0x80000000)==0)||
				((b&0x80)==0)&&((crc & 0x80000000)!=0))
				crc = (crc<<1)^0x04C11DB7;
			else 
				crc <<= 1;
		
			b <<= 1;
		}
 	}
	
	return crc;
}

#define Z_32UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFl))

u32_t sys_get_fcs_e(u32_t fcs, const void* vsrc, u32_t len)
{
   u32_t i;
   u8_t tmp;
   const u8_t* src = (u8_t*)vsrc;
   
   for(i=0; i< len; i++)
   {
     tmp = *src++;
     fcs = Z_32UpdateCRC(tmp, fcs);
   }
   
   return fcs;
}

u32_t sys_get_fcs(const void* vsrc, u32_t len)
{
   u32_t i, fcs = 0xFFFFFFFF;
   u8_t tmp;
   const u8_t* src = (u8_t*)vsrc;
   
   for(i=0; i< len; i++)
   {
     tmp = *src++;
     fcs = Z_32UpdateCRC(tmp, fcs);
   }
   
   return ~fcs;
}

// ОПИСАНИЕ: формирование случайной подстановки из шума.
// АРГУМЕНТЫ:
// @len - число слов подстановки (не более 65535);
// @dst - [out] полученная подстановка (len слов);
// @noise - [in] шум для формирования подстановки (@len слов).
// ВОЗВРАЩАЕТ: число байт, записаных в @dst.
int sys_get_perm_e(int len, u16_t * dst, const u16_t * noise)
{
    int i;
    u16_t sigma, tmp;

    for(i=0; i<len; i++) dst[i] = i;
	
    for(i=0; i<len-1; i++)
	{
        sigma =  noise[i];
        tmp = sigma % (len-i);
		sigma = dst[tmp];
        dst[tmp] = dst[len-i-1];
        dst[len-i-1]   = sigma;
    }
    return len;
}

// ОПИСАНИЕ: получение слуайной подстановки.
// АРГУМЕНТЫ:
// @len - число байт (не более 255);
// @dst - полученная подстановка.
// ВОЗВРАЩАЕТ: число полученных байт.
int sys_get_perm(int len, u8_t * dst)
{
    int i;
    u8_t sigma, tmp;
	u8_t noise[256];
	
	if (len > 256) FATAL_ERROR();

	if (rng_read(noise, len) != len) FATAL_ERROR();
	
    for(i=0;i<len;i++) dst[i] = i;
	
    for(i=0;i<len-1;i++)
	{
        sigma =  * (noise+i);
        tmp = sigma % (len-i);
		sigma = dst[tmp];
        dst[tmp] = dst[len - i - 1];
        dst[len - i - 1]   = sigma;
    }
	
    return len;
}

// ОПИСАНИЕ: сравнение данных по подстановке.
// АРГУМЕНТЫ:
// @d1 - сравниваемые данные;
// @d2 - сравниваемые данные;;
// @size - число байт в @d1/@d2;
// ВОЗВРАЩАЕТ: аналогично memcmp().
int sys_memcmp(const void* d1, const void* d2, u32_t size)
{
	const u8_t* const b1 = (u8_t*)d1;
	const u8_t* const b2 = (u8_t*)d2;
	u32_t i;
	
	ASSERT(size < MAX_PERM_SIZE);
	
	if (rng_read(_noise, size*2) != (size*2)) FATAL_ERROR();
	
	if (sys_get_perm_e(size, _os_perm, _noise) != size) FATAL_ERROR();
	
	for (i=0; i<size; i++)
	{
		int rs;
		u16_t perm = _os_perm[i];
		rs = b1[perm] - b2[perm];
		if (rs != 0) return rs;
	}
	
	return 0;
}

// ОПИСАНИЕ: сравнение побайтно двух массив из 4 байт.
// ВОЗВРАЩАЕТ: 0 - равны, иначе не равны.
int sys_cmp_dword_equ(const void * a, const void * b){
	int i;
	u8_t * p1, * p2;
	p1 = (u8_t *) a;
	p2 = (u8_t *) b;
	for(i=0;i<4;i++){
		if(* (p1+i) != * (p2+i))  return -1;
	}
	if(p1 == p2) return -1;
	else return 0;
}


DWORD Z_32UpdateResidue(BYTE c, DWORD residue){
/*----------------------------------------------------------------
  Накопление остатка от деления на полином 0x1143F3DD9
  ----------------------------------------------------------------*/
  BYTE b;
  DWORD rez;

  b = (BYTE)(((residue & 0xff000000l)>>24)&0xff);
  rez = restab[b];
  rez = rez^((residue<<8)+c);
  return rez;
}

// изменение порядка следования N байтов в строке на противоположный
void rearrange_bytes(int N, BYTE * src){
   int i;
   BYTE symbol;

   for(i = 0; i < N/2; i++){
               symbol = * (src+N-1-i);
        * (src+N-1-i) = * (src+i);
            * (src+i) = symbol;
   }
}

DWORD sys_get_residue(const BYTE * src, int len){
/*---------------------------------------------------------------
   Получить остаток от деления на полином 0x1143F3DD9
  ---------------------------------------------------------------*/
  int i;
  DWORD residue = 0l;
  BYTE tmp;
  BYTE rear[4];

  for(i=0;i< len;i++){
    tmp= *src++;
    residue=Z_32UpdateResidue(tmp,residue);
  }
  * (DWORD *)rear = residue;
  rearrange_bytes(4,rear);
  residue = * (DWORD *)rear;
  return residue;
}

#endif /* 0 */
///////////////////////////////////////////////////////////////////////////////


