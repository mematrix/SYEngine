/*
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "GolombBuffer.h"
#include <memory.h>
#include <assert.h>

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

static void RemoveMpegEscapeCode(unsigned char* dst, unsigned char* src, int& length)
{
	memset(dst, 0, length);
	int si = 0;
	int di = 0;
	while (si + 2 < length) {
		if (src[si + 2] > 3) {
			dst[di++] = src[si++];
			dst[di++] = src[si++];
		} else if (src[si] == 0 && src[si + 1] == 0) {
			dst[di++] = 0;
			dst[di++] = 0;
			if (src[si + 2] == 3) { // escape
				si += 3;
			} else {
				si += 2;
			}
			continue;
		}

		dst[di++] = src[si++];
	}
	while (si < length) {
		dst[di++] = src[si++];
	}

	length = di;
}

CGolombBuffer::CGolombBuffer(unsigned char* pBuffer, int nSize, bool bRemoveMpegEscapes/* = false*/)
	: m_bRemoveMpegEscapes(bRemoveMpegEscapes)
	, m_pTmpBuffer(nullptr)
{
	Reset(pBuffer, nSize);
}

CGolombBuffer::~CGolombBuffer()
{
	if (m_pTmpBuffer)
		delete[] m_pTmpBuffer;
}

unsigned long long CGolombBuffer::BitRead(int nBits, bool fPeek)
{
	//ASSERT(nBits >= 0 && nBits <= 64);
	while (m_bitlen < nBits) {
		m_bitbuff <<= 8;

		if (m_nBitPos >= m_nSize) {
			return 0;
		}

		*(unsigned char*)&m_bitbuff = m_pBuffer[m_nBitPos++];
		m_bitlen += 8;
	}

	int bitlen = m_bitlen - nBits;

	unsigned long long ret;
	// The shift to 64 bits can give incorrect results.
	// "The behavior is undefined if the right operand is negative, or greater than or equal to the length in bits of the promoted left operand."
	if (nBits == 64) {
		ret = m_bitbuff;
	} else {
		ret = (m_bitbuff >> bitlen) & ((1ULL << nBits) - 1);
	}

	if (!fPeek) {
		m_bitbuff	&= ((1ULL << bitlen) - 1);
		m_bitlen	= bitlen;
	}

	return ret;
}

unsigned long long CGolombBuffer::UExpGolombRead()
{
	int n = -1;
	for (unsigned char b = 0; !b && !IsEOF(); n++) {
		b = (unsigned char)BitRead(1);
	}
	return (1ULL << n) - 1 + BitRead(n);
}

unsigned int CGolombBuffer::UintGolombRead()
{
	unsigned int value = 0, count = 0;
	while (!BitRead(1) && !IsEOF()) {
		count++;
		value <<= 1;
		value |= BitRead(1);
	}

	return (1 << count) - 1 + value;
}

long long CGolombBuffer::SExpGolombRead()
{
	unsigned long long k = UExpGolombRead();
	return ((k&1) ? 1 : -1) * ((k + 1) >> 1);
}

void CGolombBuffer::BitByteAlign()
{
	m_bitlen &= ~7;
}

int CGolombBuffer::GetPos()
{
	return m_nBitPos - (m_bitlen>>3);
}

void CGolombBuffer::ReadBuffer(unsigned char* pDest, int nSize)
{
	assert(m_nBitPos + nSize <= m_nSize);
	assert(m_bitlen == 0);
	nSize = min (nSize, m_nSize - m_nBitPos);

	memcpy(pDest, m_pBuffer + m_nBitPos, nSize);
	m_nBitPos += nSize;
}

void CGolombBuffer::Reset()
{
	m_nBitPos	= 0;
	m_bitlen	= 0;
	m_bitbuff	= 0;
}

void CGolombBuffer::Reset(unsigned char* pNewBuffer, int nNewSize)
{
	if (m_bRemoveMpegEscapes) {
		delete[] m_pTmpBuffer;
		m_pTmpBuffer = new unsigned char[nNewSize];

		RemoveMpegEscapeCode(m_pTmpBuffer, pNewBuffer, nNewSize);
		m_pBuffer	= m_pTmpBuffer;
		m_nSize		= nNewSize;
	} else {
		m_pBuffer	= pNewBuffer;
		m_nSize		= nNewSize;
	}

	Reset();
}

void CGolombBuffer::SkipBytes(int nCount)
{
	m_nBitPos  += nCount;
	m_bitlen	= 0;
	m_bitbuff	= 0;
}

void CGolombBuffer::Seek(int nCount)
{
	m_nBitPos	= nCount;
	m_bitlen	= 0;
	m_bitbuff	= 0;
}
