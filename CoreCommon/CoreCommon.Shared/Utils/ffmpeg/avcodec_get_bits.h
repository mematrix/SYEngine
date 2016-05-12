/*
 * copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ___AVCODEC_GET_BITS_H___
#define ___AVCODEC_GET_BITS_H___

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#ifdef _MSC_VER
#define FF_INLINE __forceinline
#else
#define FF_INLINE inline
#endif

#ifdef __GNUC__
#define FF_DECLARE_ALIGNED(n,t,v) t __attribute__ ((aligned(n))) v
#endif
#ifdef _MSC_VER
#define FF_DECLARE_ALIGNED(n,t,v) __declspec(align(n)) t v
#endif

#define FF_READ_LE32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
    (((const uint8_t*)(x))[2] << 16) |    \
    (((const uint8_t*)(x))[1] <<  8) |    \
    ((const uint8_t*)(x))[0])

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define FFALIGN(x,a) (((x) + (a) - 1) & ~((a) - 1))
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

static FF_INLINE int ff_sign_extend(int val,unsigned bits)
{
    unsigned shift = 8 * sizeof(int) - bits;
    union { unsigned u; int s; } v = { (unsigned) val << shift };
    return v.s >> shift;
}

static FF_INLINE unsigned ff_zero_extend(unsigned val,unsigned bits)
{
    return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}

static FF_INLINE int ff_av_clip(int a,int amin,int amax)
{
    if (a < amin) return amin;
    else if (a > amax) return amax;
    else return a;
}

//////////////////////////////////////////////////

typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
    int size_in_bits_plus8;
} GetBitContext;

#define FF_ERROR_INVALIDDATA -1

#define OPEN_READER(name, gb)                   \
    unsigned int name ## _index = (gb)->index;  \
    unsigned int name ## _cache
#define CLOSE_READER(name, gb) (gb)->index = name ## _index

# define UPDATE_CACHE(name, gb) name ## _cache = \
      FF_READ_LE32((gb)->buffer + (name ## _index >> 3)) >> (name ## _index & 7)
# define SKIP_CACHE(name, gb, num) name ## _cache >>= (num)

#define SKIP_COUNTER(name, gb, num) name ## _index += (num)
#define SKIP_BITS(name, gb, num)                \
    do {                                        \
        SKIP_CACHE(name, gb, num);              \
        SKIP_COUNTER(name, gb, num);            \
    } while (0)

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define SHOW_UBITS(name, gb, num) ff_zero_extend(name ## _cache, num)
#define SHOW_SBITS(name, gb, num) ff_sign_extend(name ## _cache, num)

#define GET_CACHE(name, gb) ((uint32_t) name ## _cache)
#define NEG_USR32(a,s) (((unsigned)(a)) >> (32 - (s)))

static FF_INLINE int get_bits_count(const GetBitContext *s)
{
    return s->index;
}

static FF_INLINE void skip_bits_long(GetBitContext *s, int n)
{
    s->index += n;
}

static FF_INLINE int get_xbits(GetBitContext *s, int n)
{
    register int sign;
    register int32_t cache;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    cache = GET_CACHE(re, s);
    sign  = ~cache >> 31;
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return (NEG_USR32(sign ^ cache, n) ^ sign) - sign;
}

static FF_INLINE int get_sbits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_SBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

static FF_INLINE unsigned int get_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
    return tmp;
}

static FF_INLINE unsigned int show_bits(GetBitContext *s, int n)
{
    register int tmp;
    OPEN_READER(re, s);
    UPDATE_CACHE(re, s);
    tmp = SHOW_UBITS(re, s, n);
    return tmp;
}

#pragma warning(push)
#pragma warning(disable:4101)
static FF_INLINE void skip_bits(GetBitContext *s, int n)
{
    OPEN_READER(re, s);
    LAST_SKIP_BITS(re, s, n);
    CLOSE_READER(re, s);
}
#pragma warning(pop)

static FF_INLINE unsigned int get_bits1(GetBitContext *s)
{
    unsigned int index = s->index;
    uint8_t result     = s->buffer[index >> 3];
    result >>= index & 7;
    result  &= 1;
        index++;
    s->index = index;

    return result;
}

static FF_INLINE unsigned int show_bits1(GetBitContext *s)
{
    return show_bits(s, 1);
}

static FF_INLINE void skip_bits1(GetBitContext *s)
{
    skip_bits(s, 1);
}

static FF_INLINE unsigned int get_bits_long(GetBitContext *s, int n)
{
    if (!n) {
        return 0;
    } else if (n <= 25) {
        return get_bits(s, n);
    } else {
        unsigned ret = get_bits(s, 16);
        return ret | (get_bits(s, n - 16) << 16);
    }
}

static FF_INLINE uint64_t get_bits64(GetBitContext *s, int n)
{
    if (n <= 32) {
        return get_bits_long(s, n);
    } else {
        uint64_t ret = get_bits_long(s, 32);
        return ret | (uint64_t) get_bits_long(s, n - 32) << 32;
    }
}

static FF_INLINE int get_sbits_long(GetBitContext *s, int n)
{
    return ff_sign_extend(get_bits_long(s, n), n);
}

static FF_INLINE unsigned int show_bits_long(GetBitContext *s, int n)
{
    if (n <= 25) {
        return show_bits(s, n);
    } else {
        GetBitContext gb = *s;
        return get_bits_long(&gb, n);
    }
}

static FF_INLINE int check_marker(GetBitContext *s, const char *msg)
{
    int bit = get_bits1(s);
    return bit;
}

static FF_INLINE int init_get_bits(GetBitContext *s, const uint8_t *buffer,
                                int bit_size)
{
    int buffer_size;
    int ret = 0;

    if (bit_size >= INT_MAX - 7 || bit_size < 0 || !buffer) {
        buffer_size = bit_size = 0;
        buffer      = NULL;
        ret         = FF_ERROR_INVALIDDATA;
    }

    buffer_size = (bit_size + 7) >> 3;

    s->buffer             = buffer;
    s->size_in_bits       = bit_size;
    s->size_in_bits_plus8 = bit_size + 8;
    s->buffer_end         = buffer + buffer_size;
    s->index              = 0;

    return ret;
}

static FF_INLINE int init_get_bits8(GetBitContext *s, const uint8_t *buffer,
                                 int byte_size)
{
    if (byte_size > INT_MAX / 8 || byte_size < 0)
        byte_size = -1;
    return init_get_bits(s, buffer, byte_size * 8);
}

static FF_INLINE const uint8_t *align_get_bits(GetBitContext *s)
{
    int n = -get_bits_count(s) & 7;
    if (n)
        skip_bits(s, n);
    return s->buffer + (s->index >> 3);
}

static FF_INLINE int get_bits_left(GetBitContext *gb)
{
    return gb->size_in_bits - get_bits_count(gb);
}

static FF_INLINE int get_unary(GetBitContext *gb, int stop, int len)
{
    int i;
    for(i = 0; i < len && get_bits1(gb) != stop; i++);
    return i;
}

#endif //___AVCODEC_GET_BITS_H___