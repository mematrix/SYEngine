#ifndef _OPUS_DURATION_CALC_H
#define _OPUS_DURATION_CALC_H

#include "DurationCalcInterface.h"

class OpusDurationCalcParser : public IDurationCalc
{
	unsigned _nch, _rate;

public:
	bool Initialize(unsigned char* extradata,unsigned len);
	bool Calc(unsigned char* buf,unsigned size,double* time);
	void DeleteMe() { delete this; }
};

inline IDurationCalc* CreateOpusDurationCalcEngine() throw()
{
	return new OpusDurationCalcParser();
}

#endif //_OPUS_DURATION_CALC_H