#ifndef _ALAC_DURATION_CALC_H
#define _ALAC_DURATION_CALC_H

#include "DurationCalcInterface.h"

class AlacDurationCalcParser : public IDurationCalc
{
	double duration;

public:
	bool Initialize(unsigned char* extradata,unsigned len);
	bool Calc(unsigned char* buf,unsigned size,double* time);
	void DeleteMe() { delete this; }
};

inline IDurationCalc* CreateAlacDurationCalcEngine() throw()
{
	return new AlacDurationCalcParser();
}

#endif //_ALAC_DURATION_CALC_H