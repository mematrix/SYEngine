#ifndef __AKA_MATROSKA_GLOBAL_H
#define __AKA_MATROSKA_GLOBAL_H

#include "AkaMatroskaPrehead.h"

#ifndef __EBML_GLOBAL_MSB_
#if defined(__linux__)
#include <endian.h>
#if (__BYTE_ORDER == __BIG_ENDIAN)
#define __EBML_GLOBAL_MSB_
#endif
#endif
#if defined(__sparc__) || defined(__alpha__) || defined(__mips__) || defined(__ppc__) || defined(__PPC__) || defined(__BIG_ENDIAN__)
#define __EBML_GLOBAL_MSB_
#endif
#endif

#ifndef __EBML_GLOBAL_MSB_
#ifndef _MSC_VER
#define EBML_SWAP16(x) \
	((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define EBML_SWAP32(x) \
	((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | \
	(((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))
#define EBML_SWAP64(x) \
	((((x) & 0xFF00000000000000ULL) >> 56) | (((x) & 0x00FF000000000000ULL) >> 40) | \
	(((x) & 0x0000FF0000000000ULL) >> 24) | (((x) & 0x000000FF00000000ULL) >> 8) | \
	(((x) & 0x00000000FF000000ULL) << 8) | (((x) & 0x0000000000FF0000ULL) << 24) | \
	(((x) & 0x000000000000FF00ULL) << 40) | (((x) & 0x00000000000000FFULL) << 56))
#else
#define EBML_SWAP16(x) (_byteswap_ushort(x))
#define EBML_SWAP32(x) (_byteswap_ulong(x))
#define EBML_SWAP64(x) (_byteswap_uint64(x))
#endif
#else
#define EBML_SWAP16(x) (x)
#define EBML_SWAP32(x) (x)
#define EBML_SWAP64(x) (x)
#endif

#define EBML_ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 8)
#define EBML_ARRAY_COUNT(ary) (sizeof(ary) / sizeof(ary[0]))

#endif //__AKA_MATROSKA_GLOBAL_H