#include "WV4Help.h"

bool MKVBlockFrameToWV4BlockHead(unsigned char* frame,unsigned frameLen,WAVPACK4_BLOCK_HEADER* wv4)
{
	if (frameLen < 12)
		return false;

	unsigned samples = *(unsigned*)frame;
	unsigned flags = *(unsigned*)(frame + 4);
	unsigned crc = *(unsigned*)(frame + 8);

	wv4->DefaultInit();
	wv4->block_size = frameLen + 12;
	wv4->block_samples = samples;
	wv4->flags = flags;
	wv4->crc = crc;

	return true;
}