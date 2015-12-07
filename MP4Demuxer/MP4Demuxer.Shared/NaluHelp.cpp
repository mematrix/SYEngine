#include "MP4MediaFormat.h"

static inline unsigned GetFrameNALUSize(unsigned char* p,unsigned lengthSizeMinusOne)
{
	if (lengthSizeMinusOne == 4)
		return ISOM_SWAP32(*(unsigned*)p);
	else if (lengthSizeMinusOne == 3)
		return ((ISOM_SWAP32(*(unsigned*)p)) >> 8);
	else if (lengthSizeMinusOne == 2)
		return ISOM_SWAP16(*(unsigned short*)p);
	else if (lengthSizeMinusOne == 1)
		return *p;

	return 0xFFFFFFFF;
}

unsigned ScanAllFrameNALUToAnnexB(unsigned char* source,unsigned len)
{
	decltype(source) src = source;
	unsigned count = 0,size = 0;
	while (1)
	{
		unsigned nal_size = *(unsigned*)src;
#ifndef _MSC_VER
		nal_size = MKV_SWAP32(nal_size);
#else
		nal_size = _byteswap_ulong(nal_size);
#endif

		if (nal_size == 0)
		{
			src += 4;
			continue;
		}

		if (nal_size > (len - size))
			break;

		*(unsigned*)&src[0] = 0x01000000;
		size = size + 4 + nal_size;
		src = source + size;

		if (size >= len)
			break;
	}
	return size;
}

unsigned ScanAllFrameNALU(unsigned char* psrc,unsigned char* pdst,unsigned len,unsigned lengthSizeMinusOne)
{
	if (len < lengthSizeMinusOne)
		return 0;

	decltype(psrc) ps = psrc;
	decltype(pdst) pd = pdst;

	unsigned size = 0,size2 = 0;
	unsigned count = 0;
	while (1)
	{
		unsigned nal_size = GetFrameNALUSize(ps,lengthSizeMinusOne);
		if (nal_size == 0)
		{
			ps += lengthSizeMinusOne;
			continue;
		}
		
		if (nal_size > len)
			return 0;

		/*
		pd[0] = 0;
		pd[1] = 0;
		pd[2] = 0;
		pd[3] = 1;
		*/
		*(unsigned*)&pd[0] = 0x01000000;

		memcpy(&pd[4],ps + lengthSizeMinusOne,nal_size);

		size = size + lengthSizeMinusOne + nal_size;
		if (lengthSizeMinusOne == 4)
			size2 = size;
		else
			size2 = size2 + 4 + nal_size;

		pd = pdst + size2;
		ps = psrc + size;

		if (size >= len)
			break;
	}

	return size2;
}