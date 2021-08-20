#ifndef _KS_H
#define _KS_H

#include <stdint.h>

#define Z_32UpdateCRC(c,crc) (cr3tab[((int) crc ^ c) & 0xff] ^ ((crc >> 8) & 0x00FFFFFFl))

#define INITFCS 0xffff  
#define GOODFCS 0xf0b8  
#define GOODFCS_X25 0x0

#define CRC32_OK 0xD52256B0

#define FCS(fcs, c) (((fcs) >> 8) ^ fcstab[((fcs) ^ (c)) & 0xff])
#define FCS_X25(fcs, c) (((fcs) << 8) ^ fcstab_x25[(((fcs) >> 8) ^ (c)) & 0xff])

extern const unsigned short int fcstab[256];
extern const unsigned short int fcstab_x25[256];

u32_t CRC32( u8_t*, u8_t* );
u32_t crc32(u32_t crc, const void* data, u32_t size);

u16_t CRC16(u16_t fcs, const void* data, u32_t size);
u16_t CRC16_X25(const void* data, u32_t size);

u32_t sys_get_fcs_e(u32_t fcs, const void* vsrc, u32_t len);
u32_t sys_get_fcs(const void* vsrc, u32_t len);

extern uint32_t get_ksum_l(unsigned int len, const void* vsource);
extern u32_t get_ksum_l_subst(u32_t len,  const void* data);
extern int check_ksum_l(u32_t len, const void* v1, const void* v2);

extern int sys_get_perm(int len, uint8_t * dst);
extern int sys_get_perm_e(int len, u16_t * dst, const u16_t * noise);

extern int sys_memcmp(const void* d1, const void* d2, u32_t size);

extern u32_t sys_get_residue(const u8_t * src, int len);

extern void rearrange_bytes(int N, u8_t * src);

extern int sys_cmp_dword_equ(const void * a, const void * b);
#endif // _KS_H


