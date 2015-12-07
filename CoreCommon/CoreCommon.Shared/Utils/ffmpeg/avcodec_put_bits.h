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

#ifndef ___AVCODEC_PUT_BITS_H___
#define ___AVCODEC_PUT_BITS_H___

#include "avcodec_get_bits.h"

#define FF_WRITE_WL32(p,darg) do {                \
    unsigned d = (darg);                    \
    ((uint8_t*)(p))[0] = (d);               \
    ((uint8_t*)(p))[1] = (d)>>8;            \
    ((uint8_t*)(p))[2] = (d)>>16;           \
    ((uint8_t*)(p))[3] = (d)>>24;           \
    }while(0)
#define FF_WRITE_WB32(p,darg) do {                \
    unsigned d = (darg);                    \
    ((uint8_t*)(p))[3] = (d);               \
    ((uint8_t*)(p))[2] = (d)>>8;            \
    ((uint8_t*)(p))[1] = (d)>>16;           \
    ((uint8_t*)(p))[0] = (d)>>24;           \
    }while(0)

typedef struct PutBitContext {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
    int size_in_bits;
} PutBitContext;

static FF_INLINE void init_put_bits(PutBitContext *s, uint8_t *buffer,
                                 int buffer_size)
{
    if (buffer_size < 0) {
        buffer_size = 0;
        buffer      = NULL;
    }

    s->size_in_bits = 8 * buffer_size;
    s->buf          = buffer;
    s->buf_end      = s->buf + buffer_size;
    s->buf_ptr      = s->buf;
    s->bit_left     = 32;
    s->bit_buf      = 0;
}

static FF_INLINE int put_bits_count(PutBitContext *s)
{
    return (s->buf_ptr - s->buf) * 8 + 32 - s->bit_left;
}

static FF_INLINE int put_bits_left(PutBitContext* s)
{
    return (s->buf_end - s->buf_ptr) * 8 - 32 + s->bit_left;
}

static FF_INLINE void flush_put_bits(PutBitContext *s)
{
#ifndef FF_BITSTREAM_WRITER_LE
    if (s->bit_left < 32)
        s->bit_buf <<= s->bit_left;
#endif
    while (s->bit_left < 32) {
#ifdef FF_BITSTREAM_WRITER_LE
        *s->buf_ptr++ = s->bit_buf;
        s->bit_buf  >>= 8;
#else
        *s->buf_ptr++ = s->bit_buf >> 24;
        s->bit_buf  <<= 8;
#endif
        s->bit_left  += 8;
    }
    s->bit_left = 32;
    s->bit_buf  = 0;
}

static FF_INLINE void put_bits(PutBitContext *s, int n, unsigned int value)
{
    unsigned int bit_buf;
    int bit_left;

    assert(n <= 31 && value < (1U << n));

    bit_buf  = s->bit_buf;
    bit_left = s->bit_left;

#ifdef FF_BITSTREAM_WRITER_LE
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
        assert(s->buf_ptr+3<s->buf_end);
        FF_WRITE_WL32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_buf     = (bit_left == 32) ? 0 : value >> bit_left;
        bit_left   += 32;
    }
    bit_left -= n;
#else
    if (n < bit_left) {
        bit_buf     = (bit_buf << n) | value;
        bit_left   -= n;
    } else {
        bit_buf   <<= bit_left;
        bit_buf    |= value >> (n - bit_left);
        assert(s->buf_ptr+3<s->buf_end);
        FF_WRITE_WB32(s->buf_ptr, bit_buf);
        s->buf_ptr += 4;
        bit_left   += 32 - n;
        bit_buf     = value;
    }
#endif

    s->bit_buf  = bit_buf;
    s->bit_left = bit_left;
}

static FF_INLINE void put_sbits(PutBitContext *pb, int n, int32_t value)
{
    assert(n >= 0 && n <= 31);
    put_bits(pb, n, value & ((1 << n) - 1));
}

static FF_INLINE void put_bits32(PutBitContext *s, uint32_t value)
{
    int lo = value & 0xffff;
    int hi = value >> 16;
#ifdef FF_BITSTREAM_WRITER_LE
    put_bits(s, 16, lo);
    put_bits(s, 16, hi);
#else
    put_bits(s, 16, hi);
    put_bits(s, 16, lo);
#endif
}

static FF_INLINE uint8_t *put_bits_ptr(PutBitContext *s)
{
    return s->buf_ptr;
}

static FF_INLINE void skip_put_bytes(PutBitContext *s, int n)
{
    assert((put_bits_count(s) & 7) == 0);
    assert(s->bit_left == 32);
    s->buf_ptr += n;
}

static FF_INLINE void skip_put_bits(PutBitContext *s, int n)
{
    s->bit_left -= n;
    s->buf_ptr  -= 4 * (s->bit_left >> 5);
    s->bit_left &= 31;
}

static FF_INLINE void set_put_bits_buffer_size(PutBitContext *s, int size)
{
    s->buf_end = s->buf + size;
}

#endif //___AVCODEC_PUT_BITS_H___