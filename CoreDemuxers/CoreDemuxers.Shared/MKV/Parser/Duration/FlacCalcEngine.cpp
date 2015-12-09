#include "FlacCalcEngine.h"

static unsigned FlacCalcFrameSamples(unsigned flag,unsigned char* buf,unsigned size)
{
	if (flag == 0)
		return 0;

	int varsize = buf[1] & 0x01;

	if (flag == 1)
		return 192;
	else if (flag < 6)
		return 576 << (flag - 2);
	else if (flag == 6 || flag == 7)
		return 0;
	else
		return 256 << (flag - 8);
}

bool FlacDurationCalcParser::Initialize(unsigned char* extradata,unsigned len)
{
	if (extradata == nullptr || len < 40)
		return false;
	if (extradata[0] != 0x66 || extradata[1] != 0x4C || extradata[2] != 0x61 || extradata[3] != 0x43)
		return false;

	unsigned offset = 0;
	unsigned char* p = extradata + 4;
	for (unsigned i = 0;i < (len - 5);i++)
	{
		if (p[i] == 0x00 || p[i] == 0x80)
		{
			offset = i + 4;
			break;
		}
	}

	if (offset == 0)
		return false;
	offset += 4;

	_minBlockSize = *(unsigned short*)(extradata + offset);
	_maxBlockSize = *(unsigned short*)(extradata + offset + 2);
	_minBlockSize = DURATION_CALC_SWAP16(_minBlockSize);
	_maxBlockSize = DURATION_CALC_SWAP16(_maxBlockSize);

	offset += 10;
	unsigned temp = *(unsigned*)(extradata + offset);
	temp = DURATION_CALC_SWAP32(temp);

	unsigned rate = (temp & 0xFFFFF000) >> 12;
	unsigned channel = ((temp & 0xE00) >> 9) + 1;
	unsigned bits = ((temp & 0x1F0) >> 4) + 1;

	_nch = channel;
	_rate = rate;
	return true;
}

bool FlacDurationCalcParser::Calc(unsigned char* buf,unsigned size,double* time)
{
	if (buf == nullptr || size < 4)
		return false;
	if (*buf != 0xFF)
		return false;

	unsigned char flag = buf[2];
	flag = (flag & 0xF0) >> 4;
	unsigned samples = FlacCalcFrameSamples(flag,buf,size);
	if (samples == 0)
		return false;
	if (samples > _maxBlockSize)
		return false;

	*time = (double)samples / (double)_rate;
	return true;
}