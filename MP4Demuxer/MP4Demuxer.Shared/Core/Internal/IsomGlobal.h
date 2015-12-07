#ifndef __ISOM_GLOBAL_H_
#define __ISOM_GLOBAL_H_

#include "GlobalPrehead.h"

//Default to Little Endian->Big Endian.
//define __ISOM_GLOBAL_MSB_ to raw Big Endian.

#ifndef __ISOM_GLOBAL_MSB_
#ifndef _MSC_VER
#define ISOM_SWAP16(x) \
	((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define ISOM_SWAP32(x) \
	((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | \
	(((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))
#define ISOM_SWAP64(x) \
	((((x) & 0xFF00000000000000ULL) >> 56) | (((x) & 0x00FF000000000000ULL) >> 40) | \
	(((x) & 0x0000FF0000000000ULL) >> 24) | (((x) & 0x000000FF00000000ULL) >> 8) | \
	(((x) & 0x00000000FF000000ULL) << 8) | (((x) & 0x0000000000FF0000ULL) << 24) | \
	(((x) & 0x000000000000FF00ULL) << 40) | (((x) & 0x00000000000000FFULL) << 56))
#else
#define ISOM_SWAP16(x) (_byteswap_ushort(x))
#define ISOM_SWAP32(x) (_byteswap_ulong(x))
#define ISOM_SWAP64(x) (_byteswap_uint64(x))
#endif
#else
#define ISOM_SWAP16(x) (x)
#define ISOM_SWAP32(x) (x)
#define ISOM_SWAP64(x) (x)
#endif

#define ISOM_ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 8)
#define ISOM_ARRAY_COUNT(ary) (sizeof(ary) / sizeof(ary[0]))

#ifndef ISOM_FCC
#ifndef __ISOM_GLOBAL_MSB_
#define ISOM_FCC(ch4) \
	((((unsigned)(ch4) & 0xFF) << 24) | \
	(((unsigned)(ch4) & 0xFF00) << 8) | \
	(((unsigned)(ch4) & 0xFF0000) >> 8) | \
	(((unsigned)(ch4) & 0xFF000000) >> 24))
#else
#define ISOM_FCC(ch4) (ch4)
#endif
#endif

#ifndef ISOM_MKTAG
#define ISOM_MKTAG(a,b,c,d) \
	(a | (b << 8) | (c << 16) | (d << 24))
#endif

#endif //__ISOM_GLOBAL_H_