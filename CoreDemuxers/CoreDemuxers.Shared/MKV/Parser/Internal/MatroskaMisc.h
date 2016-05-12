#ifndef _MATROSKA_MISC_H
#define _MATROSKA_MISC_H

#ifndef _MSC_VER
#include <limits.h>
#define MKV_SWAP16(x) \
	((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define MKV_SWAP32(x) \
	((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | \
	(((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))
#define MKV_SWAP64(x) \
	((((x) & 0xFF00000000000000ULL) >> 56) | (((x) & 0x00FF000000000000ULL) >> 40) | \
	(((x) & 0x0000FF0000000000ULL) >> 24) | (((x) & 0x000000FF00000000ULL) >> 8) | \
	(((x) & 0x00000000FF000000ULL) << 8) | (((x) & 0x0000000000FF0000ULL) << 24) | \
	(((x) & 0x000000000000FF00ULL) << 40) | (((x) & 0x00000000000000FFULL) << 56))
#else
#include <stdlib.h>
#define MKV_SWAP16(x) _byteswap_ushort(x)
#define MKV_SWAP32(x) _byteswap_ulong(x)
#define MKV_SWAP64(x) _byteswap_uint64(x)
#endif

#define MKV_ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 8) //4bytes.
#define MKV_ARRAY_COUNT(ary) (sizeof(ary) / sizeof(ary[0]))

#if defined(_MSC_VER) && defined(_MKV_EXPORT_CLASS)
#define MKV_EXPORT __declspec(dllexport)
#else
#define MKV_EXPORT
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#define MKV_DBG_BREAK __debugbreak()
#else
#define MKV_DBG_BREAK
#endif

#endif //_MATROSKA_MISC_H