#ifndef _PCM_DURATION_CALC_H
#define _PCM_DURATION_CALC_H

#include "DurationCalcInterface.h"

class PcmDurationCalcParser : public IDurationCalc
{
	unsigned _blockAlign;
	unsigned _nch, _rate, _bits;
	bool _float;

public:
	PcmDurationCalcParser(unsigned nch,unsigned rate,unsigned bits,bool ieee_float) throw() :
		_nch(nch),
		_rate(rate),
		_bits(bits),
		_float(ieee_float) {}

public:
	bool Initialize(unsigned char* extradata,unsigned len) { _blockAlign = _nch * (_bits / 8); return true; }
	bool Calc(unsigned char* buf,unsigned size,double* time);
	void DeleteMe() { delete this; }
};

inline IDurationCalc* CreatePcmDurationCalcEngine(unsigned nch,unsigned rate,unsigned bits,bool ieee_float) throw()
{
	if (nch == 0 || rate == 0 || bits == 0)
		return nullptr;
	return new PcmDurationCalcParser(nch,rate,bits,ieee_float);
}

#endif //_PCM_DURATION_CALC_H