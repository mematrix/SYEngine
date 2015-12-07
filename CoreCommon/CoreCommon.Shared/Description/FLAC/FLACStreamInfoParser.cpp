#include "stagefright/ABitReader.h"
#include "FLACStreamInfoParser.h"

int FindFlacStreamInfoOffset(unsigned char* p,unsigned len)
{
	if (p[0] != 0x66 ||
		p[1] != 0x4C ||
		p[2] != 0x61 ||
		p[3] != 0x43)
		return -1; //flaC

	unsigned char* pf = p + 4;
	for (unsigned i = 0;i < (len - 5);i++)
		if (pf[i] == 0x00 || pf[i] == 0x80)
			return i + 4;

	return -1;
}

unsigned ParseFlacStreamInfo(unsigned char* pb,FlacStreamInfo* info)
{
	if (pb == nullptr ||
		info == nullptr)
		return 0;

	unsigned size = pb[3];
	if (size < 14 ||
		size > 128)
		return 0;

	stagefright::ABitReader br(pb + 4,size);
	info->MinBlockSize = br.getBits(16);
	info->MaxBlockSize = br.getBits(16);
	info->MinFrameSize = br.getBits(24);
	info->MaxFrameSize = br.getBits(24);
	info->SampleRate = br.getBits(20);
	info->Channels = br.getBits(3) + 1;
	info->Bits = br.getBits(5) + 1;

	return 4 + size;
}