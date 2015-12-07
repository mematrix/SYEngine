#include "AAC_ADTS_Parser.h"

bool ADTSHeaderParse(unsigned char* ph,int* profile,int* nch,int* srate)
{
	static const int aac_sample_rate_table[] = {
		96000,88200,64000,48000,44100,32000,
		24000,22050,16000,12000,11025,8000,7350,
		0,0,0,0};

	if (*ph != 0xFF)
		return false;

	int index = (int)((ph[2] & 0x3C) >> 2);

	*profile = (int)((ph[2] & 0xC0) >> 6);
	if (index < 16)
		*srate = aac_sample_rate_table[index];
	else
		*srate = 0;

	int ch = (int)(((ph[2] & 0x01) << 2) | ((ph[3] & 0xC0) >> 6));
	if (ch == 7)
		ch = 8;
	else if (ch > 8)
		ch = 0;

	*nch = ch;

	return true;
}