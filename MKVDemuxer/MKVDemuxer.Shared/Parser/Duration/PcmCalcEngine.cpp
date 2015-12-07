#include "PcmCalcEngine.h"

bool PcmDurationCalcParser::Calc(unsigned char* buf,unsigned size,double* time)
{
	if (size == 0)
		return false;

	unsigned samples = size / _blockAlign;
	*time = (double)samples / (double)_rate;
	return true;
}