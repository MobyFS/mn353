#ifndef _GUNZIP_H
#define _GUNZIP_H

/* unzip code taken from bios-lt project */
/* http://sourceforge.net/projects/bios-lt */
/* maintainer Liu Tao */

#define ERR_BADMAGIC         1
#define ERR_BADMETHOD        2
#define ERR_ENCRYPTED        3
#define ERR_MULTIPART        4
#define ERR_INVALIDFLAGS     5
#define ERR_BADFORMAT1       6
#define ERR_BADFORMAT2       7
#define ERR_MEM              8
#define ERR_BADFORMAT        9
#define ERR_CRC              10
#define ERR_LENGTH           11

#define	PACK_MAGIC     "\037\036" /* Magic header for packed files */
#define	GZIP_MAGIC     "\037\213" /* Magic header for gzip files, 1F 8B */
#define	OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */
#define	LZH_MAGIC      "\037\240" /* Magic header for SCO LZH Compress files*/
#define PKZIP_MAGIC    "\120\113\003\004" /* Magic header for pkzip files */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

/* If B_MAX needs to be larger than 16, then h and x[] should be ulong. */
#define B_MAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */


#define NULL            0
/**/
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long int   ulong;
typedef ushort          huft_code;
typedef uchar           huft_bits;
/**/
/*
#include <sys/types.h>
typedef unsigned char   uchar;
typedef unsigned short  huft_code;
typedef uchar           huft_bits;
*/


//#define get_uchar(guz) (uchar)(guz->inptr < guz->insize ? ( guz->inbuf[guz->inptr++] ) : 0)



#define get_ushort(guz) ((ushort)get_uchar(guz) | (ushort)get_uchar(guz) << 8)
#define get_ulong(guz) ((ulong)get_uchar(guz)|(ulong)get_uchar(guz)<<8|(ulong)get_uchar(guz)<<16|(ulong)get_uchar(guz) << 24)
#define output_char(guz, ch) (guz)->outbuf[(guz)->outptr++] = ch

typedef struct {
	uchar *inbuf;
	ulong insize;
	ulong inptr;

	uchar *outbuf;
	ulong outsize;
	ulong outptr;

	ulong bitbuf;
	ulong bufbits;

} gunzip_t;
/*
uchar get_uchar(gunzip_t *guz) 
{
    uchar rv;
    if (guz->inptr < guz->insize) {
        rv = guz->inbuf[guz->inptr];
        guz->inptr++;
        return rv;
    }
    else return (uchar)0;
}
*/
int gunzip(unsigned char *inbuf, unsigned long *insize,
	unsigned char *outbuf, unsigned long *outsize);

#endif	/* _GUNZIP_H */
