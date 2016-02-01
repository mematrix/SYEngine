#include "AAC_LATM_Parser.h"
#include <stagefright/ABitReader.h>

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

bool LATMHeaderParse(unsigned char* ph,unsigned size,int* profile,int* nch,int* srate,int* he_lc_core)
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

	/*
	如果是HE-AAC，有两种explicit和implicit一共三种声明模式。（这里只支持两种explicit模式
	在explicit模式一（hierarchical signaling），AOT是5，然后在channels之后会有扩展的采样率和AOT字段（这里的AOT用于表明基本层编码，一般是2 AAC-LC），fdk_aac采用的这种方式；
	在explicit模式二（backward compatible signaling），AOT仍然是2（AAC-LC），但在GASpecificConfig后会有同步字0x2b7和sbrPresentFlag，libaacplus采用的是这种方式；
	在implicit模式，AOT仍然是2（AAC-LC），AudioSpecificConfig没有任何扩展，仍只是2个字节，需要靠解码器在AAC码流中找到SBR的数据。
	*/

	if (asc.audioObjectType != 5 && size >= 5)
	{
		stagefright::ABitReader br(ph + 2,3);
		if (br.getBits(11) == 0x2B7 && br.getBits(5) == 5 && br.getBits(1) == 1)
		{
			unsigned extensionSamplingFrequencyIndex = br.getBits(4);
			if (extensionSamplingFrequencyIndex <= 16)
				*srate = aac_sample_rate_table[extensionSamplingFrequencyIndex];
		}
	}
	return true;
}