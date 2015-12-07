#include "ALACMagicCodeParser.h"

#define ALAC_SWAP16(x) ((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define ALAC_SWAP32(x) ((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | (((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))

void ParseALACMaigcCode(unsigned char* pb,unsigned len,ALACSpecificConfig* info)
{
	if (len < 24)
		return;

	decltype(pb) p = pb;
	info->frameLength = ALAC_SWAP32(*(unsigned*)p);
	p += 4;
	info->compatibleVersion = p[0];
	info->bitDepth = p[1];
	info->pb = p[2];
	info->mb = p[3];
	info->kb = p[4];
	info->numChannels = p[5];
	p += 6;
	info->maxRun = ALAC_SWAP16(*(unsigned short*)p);
	p += 2;
	info->maxFrameBytes = ALAC_SWAP32(*(unsigned*)p);
	p += 4;
	info->avgBitRate = ALAC_SWAP32(*(unsigned*)p);
	p += 4;
	info->sampleRate = ALAC_SWAP32(*(unsigned*)p);
}