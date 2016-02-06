#include "AACAudioDescription.h"

ADTSAudioDescription::ADTSAudioDescription(unsigned* ph) throw()
{
	_asc_size = 2;
	memset(&_asc,0,32);
	memset(&_basic_desc,0,sizeof(_basic_desc));
	memset(&_profile,0,sizeof(_profile));

	if (ph != nullptr)
		FlushAudioDescription((unsigned char*)ph);
}

ADTSAudioDescription::ADTSAudioDescription(unsigned* ph,bool asc,unsigned asc_size) throw()
{
	_asc_size = 2;
	memset(&_asc,0,32);
	memset(&_basic_desc,0,sizeof(_basic_desc));
	memset(&_profile,0,sizeof(_profile));

	if (ph != nullptr)
		FlushAudioDescription2((unsigned char*)ph,asc_size > 2 ? asc_size:2);
	if (ph != nullptr && asc_size > 2 && asc_size < 32)
		memcpy(_asc,ph,asc_size),_asc_size = asc_size;
}

int ADTSAudioDescription::GetType()
{
	if (_asc[0] == 0)
		return AAC_ADTS_AUDIO_DESC_TYPE;
	else
		return AAC_RAW_AUDIO_DESC_TYPE;
}

bool ADTSAudioDescription::GetProfile(void* profile)
{
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool ADTSAudioDescription::GetExtradata(void* p)
{
	if (_asc[0] == 0)
		return false;

	memcpy(p,_asc,_asc_size);
	return true;
}

unsigned ADTSAudioDescription::GetExtradataSize()
{
	if (_asc[0] == 0)
		return 0;

	return _asc_size;
}

bool ADTSAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void ADTSAudioDescription::FlushAudioDescription(unsigned char* ph)
{
	if (*ph != 0xFF)
		return;

	ADTSHeaderParse(ph,&_profile.profile,(int*)&_basic_desc.nch,(int*)&_basic_desc.srate);

	_profile.rate_index = (int)((ph[2] & 0x3C) >> 2);
	_profile.level = 0;

	_basic_desc.bits = 0;
	_basic_desc.ch_layout = 0;
	_basic_desc.compressed = true;
}

void ADTSAudioDescription::FlushAudioDescription2(unsigned char* ph,unsigned size)
{
	memcpy(_asc,ph,2);

	LATMHeaderParse(ph,size,&_profile.profile,(int*)&_basic_desc.nch,(int*)&_basic_desc.srate,(int*)&_profile.he_with_lc_core);

	_profile.rate_index = 0;
	_profile.level = 0;

	_basic_desc.bits = 0;
	_basic_desc.ch_layout = 0;
	_basic_desc.compressed = true;
}