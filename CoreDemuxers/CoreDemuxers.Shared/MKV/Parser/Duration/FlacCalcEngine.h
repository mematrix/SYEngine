#ifndef _FLAC_DURATION_CALC_H
#define _FLAC_DURATION_CALC_H

#include "DurationCalcInterface.h"

class FlacDurationCalcParser : public IDurationCalc
{
	unsigned short _maxBlockSize, _minBlockSize;
	unsigned _nch, _rate;

public:
	bool Initialize(unsigned char* extradata,unsigned len);
	bool Calc(unsigned char* buf,unsigned size,double* time);
	void DeleteMe() { delete this; }
};

inline IDurationCalc* CreateFlacDurationCalcEngine() throw()
{
	return new FlacDurationCalcParser();
}

#endif //_FLAC_DURATION_CALC_H