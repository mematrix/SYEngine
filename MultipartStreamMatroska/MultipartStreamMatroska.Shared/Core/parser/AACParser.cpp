#include "AACParser.h"

static void ParseAudioSpecificConfig(unsigned char* pb, AACParser::AudioSpecificConfig& asc)
{
	asc.audioObjectType = (pb[0] & 0xF8) >> 3;
	asc.samplingFrequencyIndex = ((pb[0] & 0x07) << 1) | (pb[1] >> 7);
	asc.channelConfiguration = (pb[1] >> 3) & 0x0F;

	asc.GASpecificConfig.frameLengthFlag = (pb[1] >> 2) & 0x01;
	asc.GASpecificConfig.dependsOnCoreCoder = (pb[1] >> 1) & 0x01;
	asc.GASpecificConfig.extensionFlag = pb[1] & 0x01;
}

void AACParser::Parse(unsigned char* buf) throw()
{
	static const int aac_sample_rate_table[] = {
		96000, 88200, 64000, 48000, 44100, 32000,
		24000, 22050, 16000, 12000, 11025, 8000, 7350,
		0, 0, 0, 0
	};

	ParseAudioSpecificConfig(buf, config);
	if (config.samplingFrequencyIndex > 15)
		return;
	if (config.channelConfiguration == 0 || config.channelConfiguration > 8)
		return;

	channels = config.channelConfiguration;
	samplerate = aac_sample_rate_table[config.samplingFrequencyIndex];
	if (config.audioObjectType == 5 && config.GASpecificConfig.dependsOnCoreCoder == 1)
		he_lc_core_flag = 1;
}