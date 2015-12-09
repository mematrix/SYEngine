#include "AlacCalcEngine.h"

bool AlacDurationCalcParser::Initialize(unsigned char* extradata,unsigned len)
{
	if (extradata == nullptr || len < 24)
		return false;

	unsigned frameLength = DURATION_CALC_SWAP32(*(unsigned*)extradata);
	unsigned sampleRate = DURATION_CALC_SWAP32(*(unsigned*)(extradata + 20));
	if (frameLength < 128 || frameLength > 32768)
		return false;
	if (sampleRate == 0)
		return false;

	duration = (double)frameLength / (double)sampleRate;
	return true;
}

bool AlacDurationCalcParser::Calc(unsigned char* buf,unsigned size,double* time)
{
	if (buf == nullptr || size == 0)
		return false;

	*time = duration;
	return true;
}