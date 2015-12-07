#ifndef __ALAC_MAGIC_CODE_PARSER_H
#define __ALAC_MAGIC_CODE_PARSER_H

struct ALACSpecificConfig
{
	unsigned frameLength; //32b
	unsigned compatibleVersion; //8b
	unsigned bitDepth; //8b
	unsigned pb; //8b
	unsigned mb; //8b
	unsigned kb; //8b
	unsigned numChannels; //8b
	unsigned maxRun; //16b
	unsigned maxFrameBytes; //32b
	unsigned avgBitRate; //32b
	unsigned sampleRate; //32b
};

void ParseALACMaigcCode(unsigned char* pb,unsigned len,ALACSpecificConfig* info);

#endif //__ALAC_MAGIC_CODE_PARSER_H