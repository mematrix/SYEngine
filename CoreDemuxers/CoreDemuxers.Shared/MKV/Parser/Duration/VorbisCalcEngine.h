#ifndef _VORBIS_DURATION_CALC_H
#define _VORBIS_DURATION_CALC_H

#include "DurationCalcInterface.h"
#include "vorbis/codec.h"

class VorbisDurationCalcParser : public IDurationCalc
{
	vorbis_info _vi;
	vorbis_comment _vc;

	unsigned _blockSize0, _blockSize1;
	unsigned _prev_blocksize;

public:
	bool Initialize(unsigned char* extradata,unsigned len);
	bool Calc(unsigned char* buf,unsigned size,double* time);
	void DeleteMe()
	{
		if (_vi.channels > 0)
		{
			vorbis_comment_clear(&_vc);
			vorbis_info_clear(&_vi);
		}
		delete this;
	}
};

inline IDurationCalc* CreateVorbisDurationCalcEngine() throw()
{
#ifndef _MKV_PARSER_NOUSE_VORBIS
	return new VorbisDurationCalcParser();
#else
	return nullptr;
#endif
}

#endif //_VORBIS_DURATION_CALC_H