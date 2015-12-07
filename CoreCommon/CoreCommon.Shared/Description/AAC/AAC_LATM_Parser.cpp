#include "AAC_LATM_Parser.h"

struct AudioSpecificConfig
{
	unsigned audioObjectType; //5bit
	unsigned samplingFrequencyIndex; //4bit
	unsigned channelConfiguration; //4bit
	struct //3bit
	{
		unsigned frameLengthFlag;
		unsigned dependsOnCoreCoder;
		unsigned extensionFlag;
	} GASpecificConfig;
};

void ParseAACAudioSpecificConfig(unsigned char* pb,AudioSpecificConfig& asc)
{
	asc.audioObjectType = (pb[0] & 0xF8) >> 3;
	asc.samplingFrequencyIndex = ((pb[0] & 0x07) << 1) | (pb[1] >> 7);
	asc.channelConfiguration = (pb[1] >> 3) & 0x0F;

	asc.GASpecificConfig.frameLengthFlag = (pb[1] >> 2) & 0x01;
	asc.GASpecificConfig.dependsOnCoreCoder = (pb[1] >> 1) & 0x01;
	asc.GASpecificConfig.extensionFlag = pb[1] & 0x01;
}

bool LATMHeaderParse(unsigned char* ph,int* profile,int* nch,int* srate,int* he_lc_core)
{
	AudioSpecificConfig asc = {};
	ParseAACAudioSpecificConfig(ph,asc);

	static const int aac_sample_rate_table[] = {
		96000,88200,64000,48000,44100,32000,
		24000,22050,16000,12000,11025,8000,7350,
		0,0,0,0};

	if (asc.samplingFrequencyIndex > 16)
		return false;
	if (asc.channelConfiguration == 0 || asc.channelConfiguration > 8)
		return false;

	*profile = asc.audioObjectType - 1;
	*nch = asc.channelConfiguration;
	*srate = aac_sample_rate_table[asc.samplingFrequencyIndex];

	//MPEG-4 HE-AAC v1 (multichannel) with AAC-LC core.
	//MPEG-4 HE-AAC v2 (stereo) with AAC-LC core.
	if (asc.audioObjectType == 5 && asc.GASpecificConfig.dependsOnCoreCoder == 1)
	{
		*he_lc_core = 1;
		*profile = 1;
	}

	return true;
}