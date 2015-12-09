#include "OpusCalcEngine.h"
#include "OpusHelp.h"
#include <memory.h>
#include <string.h>

bool OpusDurationCalcParser::Initialize(unsigned char* extradata,unsigned len)
{
	if (extradata == nullptr || len < 16)
		return false;

	char buf[10] = {};
	memcpy(buf,extradata,8);

	if (strcmp(buf,"OpusHead") != 0)
		return false;

	_nch = extradata[9];
	_rate = *(unsigned*)&extradata[12];
	return true;
}

bool OpusDurationCalcParser::Calc(unsigned char* buf,unsigned size,double* time)
{
	if (buf == nullptr || size == 0)
		return false;

	if (opus_packet_get_nb_channels(buf) != _nch)
		return false;
	int samples = opus_packet_get_nb_samples(buf,size,_rate);
	if (samples <= 0)
		return false;

	*time = (double)samples / (double)_rate;
	return true;
}