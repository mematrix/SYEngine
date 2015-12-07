#include "MPEG_Audio_Parser.h"
#include <stdlib.h>

static const unsigned kMPEGAudioV10SampleRateIndex[] = {44100,48000,32000,0};
static const unsigned kMPEGAudioV20SampleRateIndex[] = {22050,24000,16000,0};
static const unsigned kMPEGAudioV25SampleRateIndex[] = {11025,12000,8000,0};

static const short kMPEGAudioL1BitRateIndex[] = {
	0,32,64,96,128,160,192,
	224,256,288,320,352,384,
	416,448,-1
};
static const short kMPEGAudioL2BitRateIndex[] = {
	0,32,48,56,64,80,96,
	112,128,160,192,224,256,
	320,384,-1
};
static const short kMPEGAudioL3BitRateIndex[] = {
	0,32,40,48,56,64,80,96,
	112,128,160,192,
	224,256,320,-1
};

bool MPEGAudioParseHead(unsigned char* pb,int* nch,int* srate,int* bitrate,int* mpeg_audio_level)
{
	if (*pb != 0xFF)
		return false;

	unsigned ver = ((pb[1] >> 3) & 0x03);
	unsigned layer = (pb[1] & 0x06) >> 1;

	unsigned bindex = (pb[2] & 0xF0) >> 4;
	unsigned sindex = (pb[2] & 0x0C) >> 2;

	unsigned ch_mode = ((pb[3] >> 6) & 0x03);

	if (mpeg_audio_level)
		*mpeg_audio_level = layer;

	if (ch_mode > MPEG_AUDIO_CH_MONO)
		return false;

	if (ch_mode != MPEG_AUDIO_CH_MONO)
		*nch = 2;
	else
		*nch = 1;

	switch (ver)
	{
	case MPEG_AUDIO_VERSION_25:
		*srate = kMPEGAudioV25SampleRateIndex[sindex];
		break;
	case MPEG_AUDIO_VERSION_20:
		*srate = kMPEGAudioV20SampleRateIndex[sindex];
		break;
	case MPEG_AUDIO_VERSION_10:
		*srate = kMPEGAudioV10SampleRateIndex[sindex];
		break;
	default:
		return false;
	}

	switch (layer)
	{
	case MPEG_AUDIO_L3:
		if (bindex <= _countof(kMPEGAudioL3BitRateIndex))
			*bitrate = kMPEGAudioL3BitRateIndex[bindex];
		break;
	case MPEG_AUDIO_L2:
		if (bindex <= _countof(kMPEGAudioL2BitRateIndex))
			*bitrate = kMPEGAudioL2BitRateIndex[bindex];
		break;
	case MPEG_AUDIO_L1:
		if (bindex <= _countof(kMPEGAudioL1BitRateIndex))
			*bitrate = kMPEGAudioL1BitRateIndex[bindex];
		break;
	default:
		return false;
	}

	return true;
}