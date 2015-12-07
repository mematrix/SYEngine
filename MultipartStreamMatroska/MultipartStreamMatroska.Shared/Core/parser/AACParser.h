#ifndef __AAC_PARSER_H
#define __AAC_PARSER_H

struct AACParser
{
	unsigned channels;
	unsigned samplerate;
	int he_lc_core_flag;

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
	AudioSpecificConfig config;

	void Parse(unsigned char* buf) throw(); //2bytes need.
};

#endif //__AAC_PARSER_H