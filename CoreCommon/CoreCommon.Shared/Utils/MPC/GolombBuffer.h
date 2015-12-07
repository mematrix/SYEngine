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

#pragma once

class CGolombBuffer
{
public:
	CGolombBuffer(unsigned char* pBuffer, int nSize, bool bRemoveMpegEscapes = false);
	~CGolombBuffer();

	unsigned long long			BitRead(int nBits, bool fPeek = false);
	unsigned long long			UExpGolombRead();
	unsigned int	UintGolombRead();
	long long			SExpGolombRead();
	void			BitByteAlign();

	inline unsigned char		ReadByte()		{ return (unsigned char)BitRead (8); };
	inline short	ReadShort()		{ return (short)BitRead (16); };
	inline unsigned	ReadDword()		{ return (unsigned)BitRead (32); };
	inline short	ReadShortLE()	{ return (short)ReadByte() | (short)ReadByte() << 8; };
	inline unsigned	ReadDwordLE()	{ return (unsigned)(ReadByte() | ReadByte() << 8 | ReadByte() << 16 | ReadByte() << 24); };
	void			ReadBuffer(unsigned char* pDest, int nSize);

	void			Reset();
	void			Reset(unsigned char* pNewBuffer, int nNewSize);

	void			SetSize(int nValue) { m_nSize = nValue; };
	int				GetSize() const     { return m_nSize; };
	int				RemainingSize() const { return m_nSize - m_nBitPos; };
	bool			IsEOF() const { return m_nBitPos >= m_nSize; };
	int				GetPos();
	unsigned char*			GetBufferPos() { return m_pBuffer + m_nBitPos; };

	void			SkipBytes(int nCount);
	void			Seek(int nPos);

private :
	unsigned char*			m_pBuffer;
	int				m_nSize;
	int				m_nBitPos;
	int				m_bitlen;
	long long			m_bitbuff;

	unsigned char*			m_pTmpBuffer;
	bool			m_bRemoveMpegEscapes;
};
