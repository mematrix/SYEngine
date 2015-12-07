#include "MPEGAudioDescription.h"

MPEGAudioDescription::MPEGAudioDescription(unsigned* ph) throw()
{
	memset(&_basic_desc,0,sizeof(_basic_desc));

	if (ph != nullptr)
		FlushAudioDescription((unsigned char*)ph);
}

int MPEGAudioDescription::GetType()
{
	switch (_profile.layer)
	{
	case MPEG_AUDIO_L1:
		return MPEG_MP1_AUDIO_DESC_TYPE;
	case MPEG_AUDIO_L2:
		return MPEG_MP2_AUDIO_DESC_TYPE;
	case MPEG_AUDIO_L3:
		return MPEG_MP3_AUDIO_DESC_TYPE;
	default:
		return -1;
	}
}

bool MPEGAudioDescription::GetProfile(void* profile)
{
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool MPEGAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void MPEGAudioDescription::FlushAudioDescription(unsigned char* ph)
{
	MPEGAudioParseHead(ph,(int*)&_basic_desc.nch,(int*)&_basic_desc.srate,&_profile.bitrate,&_profile.layer);
	_basic_desc.bitrate = _profile.bitrate * 1000;
	_basic_desc.compressed = true;
}