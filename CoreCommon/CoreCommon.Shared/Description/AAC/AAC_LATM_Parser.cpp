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

	unsigned aot = 0;
	unsigned rate = 0;
	if (asc.audioObjectType == 5 || asc.audioObjectType == 29)
	{
		stagefright::ABitReader br(ph,size);
		br.skipBits(5 + 4 + 4);
		unsigned extensionSamplingFrequencyIndex = br.getBits(4);
		if (extensionSamplingFrequencyIndex == 14)
			rate = br.getBits(24);
		else
			asc.samplingFrequencyIndex = extensionSamplingFrequencyIndex;

		aot = br.getBits(5); //audioObjectType
		if (aot == 22)
			br.skipBits(4);
		asc.GASpecificConfig.frameLengthFlag = br.getBits(1);
		asc.GASpecificConfig.dependsOnCoreCoder = br.getBits(1);
		asc.GASpecificConfig.extensionFlag = br.getBits(1);
	}

	*profile = aot > 1 ? aot - 1 : asc.audioObjectType - 1;
	*nch = asc.channelConfiguration;
	*srate = aac_sample_rate_table[asc.samplingFrequencyIndex];
	if (rate > 8000 && rate < 96000)
		*srate = rate;

	//MPEG-4 HE-AAC v1 (multichannel) with AAC-LC core.
	//MPEG-4 HE-AAC v2 (stereo) with AAC-LC core.
	if (asc.audioObjectType == 5 && asc.GASpecificConfig.dependsOnCoreCoder == 1)
	{
		*he_lc_core = 1;
		*profile = 1;
	}

	/*
	�����HE-AAC��������explicit��implicitһ����������ģʽ��������ֻ֧������explicitģʽ
	��explicitģʽһ��hierarchical signaling����AOT��5��Ȼ����channels֮�������չ�Ĳ����ʺ�AOT�ֶΣ������AOT���ڱ�����������룬һ����2 AAC-LC����fdk_aac���õ����ַ�ʽ��
	��explicitģʽ����backward compatible signaling����AOT��Ȼ��2��AAC-LC��������GASpecificConfig�����ͬ����0x2b7��sbrPresentFlag��libaacplus���õ������ַ�ʽ��
	��implicitģʽ��AOT��Ȼ��2��AAC-LC����AudioSpecificConfigû���κ���չ����ֻ��2���ֽڣ���Ҫ����������AAC�������ҵ�SBR�����ݡ�
	*/

	if (asc.audioObjectType != 5 && size >= 5)
	{
		stagefright::ABitReader br(ph + 2,3);
		if (br.getBits(11) == 0x2B7 && br.getBits(5) == 5 && br.getBits(1) == 1)
		{
			unsigned extensionSamplingFrequencyIndex = br.getBits(4);
			if (extensionSamplingFrequencyIndex <= 14)
				*srate = aac_sample_rate_table[extensionSamplingFrequencyIndex];
		}
	}
	return true;
}