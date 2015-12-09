#ifndef _DURATION_CALC_INTERFACE_H
#define _DURATION_CALC_INTERFACE_H

#define DURATION_CALC_SWAP16(x) \
	((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define DURATION_CALC_SWAP32(x) \
	((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | \
	(((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))

struct IDurationCalc
{
	virtual bool Initialize(unsigned char* extradata = nullptr,unsigned len = 0) = 0;
	virtual void SetDefaultTimestamp(double timestamp) {}
	virtual bool Calc(unsigned char* buf,unsigned size,double* time) = 0;
	virtual void DeleteMe() = 0;
};

#endif //_DURATION_CALC_INTERFACE_H