#ifndef __FLAC_STREAMINFO_PARSER_H
#define __FLAC_STREAMINFO_PARSER_H

struct FlacStreamInfo
{
	unsigned MinBlockSize;
	unsigned MaxBlockSize;
	unsigned MinFrameSize;
	unsigned MaxFrameSize;
	unsigned SampleRate;
	unsigned Channels;
	unsigned Bits;
};

int FindFlacStreamInfoOffset(unsigned char* p,unsigned len);
unsigned ParseFlacStreamInfo(unsigned char* pb,FlacStreamInfo* info);

#endif //__FLAC_STREAMINFO_PARSER_H