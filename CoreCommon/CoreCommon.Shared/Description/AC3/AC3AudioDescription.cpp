#include "AC3AudioDescription.h"

AC3AudioDescription::AC3AudioDescription(unsigned char* ph) throw()
{
	memset(&_basic_desc,0,sizeof(_basic_desc));

	if (ph != nullptr)
		FlushAudioDescription(ph);
}

bool AC3AudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void AC3AudioDescription::FlushAudioDescription(unsigned char* ph)
{
	int flags = 0,br = 0;
	if (AC3ParseFrameInfo(ph,(int*)&_basic_desc.srate,&br,&flags))
	{
		if (flags != 0 &&
			br != 0)
			_basic_desc.nch = GetAC3ChannelsFromFlags(flags);
	}

	_basic_desc.bitrate = br;
	_basic_desc.wav_avg_bytes = br / 8;
	_basic_desc.compressed = true;
}