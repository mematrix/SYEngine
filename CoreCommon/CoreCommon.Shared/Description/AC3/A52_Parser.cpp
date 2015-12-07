#include "A52_Parser.h"

#define A52_CHANNEL 0
#define A52_MONO 1
#define A52_STEREO 2
#define A52_3F 3
#define A52_2F1R 4
#define A52_3F1R 5
#define A52_2F2R 6
#define A52_3F2R 7
#define A52_CHANNEL1 8
#define A52_CHANNEL2 9
#define A52_DOLBY 10
#define A52_CHANNEL_MASK 15
#define A52_LFE 16

bool AC3ParseFrameInfo(unsigned char* pb,int* srate,int* brate,int* flags)
{
	static const unsigned char halfrate[12] = {0,0,0,0,0,0,0,0,0,1,2,3};
	static const int rate[] = {32,40,48,56,64,80,96,112,128,160,192,224,256,320,384,448,512,576,640};
	static const unsigned char lfeon[8] = {0x10,0x10,0x04,0x04,0x04,0x01,0x04,0x01};

	int frmsizecod;
	int bitrate;
	int half;
	int acmod;

	if (pb[0] != 0x0B ||
		pb[1] != 0x77)
		return false;

	if (pb[5] >= 0x60)
		return false;

	half = halfrate[pb[5] >> 3];
	acmod = pb[6] >> 5;
	*flags = ((((pb[6] & 0xF8) == 0x50) ? A52_DOLBY:acmod) | 
		((pb[6] & lfeon[acmod]) ? A52_LFE:0));

	frmsizecod = pb[4] & 63;
	if (frmsizecod >= 38)
		return false;

	bitrate = rate[frmsizecod >> 1];
	*brate = (bitrate * 1000) >> half;

	switch (pb[4] & 0xC0)
	{
	case 0:
		*srate = 48000 >> half;
		break;
	case 0x40:
		*srate = 44100 >> half;
		break;
	case 0x80:
		*srate = 32000 >> half;
		break;
	default:
		return false;
	}

	return true;
}

int GetAC3ChannelsFromFlags(int flags)
{
	int channels = 0,temp = flags & A52_CHANNEL_MASK;

	switch (temp)
	{
	case A52_CHANNEL1:
	case A52_CHANNEL2:
	case A52_MONO:
		channels = 1;
		break;
	case A52_CHANNEL:
	case A52_STEREO:
	case A52_DOLBY:
		channels = 2;
		break;
	case A52_3F:
	case A52_2F1R:
		channels = 3;
		break;
	case A52_3F1R:
	case A52_2F2R:
		channels = 4;
		break;
	case A52_3F2R:
		channels = 5;
		break;
	}

	if (channels && (flags & A52_LFE))
		channels++;

	return channels;
}