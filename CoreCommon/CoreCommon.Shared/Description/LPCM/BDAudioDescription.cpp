#include "BDAudioDescription.h"

BDAudioDescription::BDAudioDescription(unsigned* ph) throw()
{
	memset(&_basic_desc,0,sizeof(_basic_desc));

	if (ph != nullptr)
		FlushAudioDescription((unsigned char*)ph);
}

bool BDAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void BDAudioDescription::FlushAudioDescription(unsigned char* ph)
{
	BDAudioHeaderParse(ph,(int*)&_basic_desc.nch,(int*)&_basic_desc.srate,(int*)&_basic_desc.bits,&_basic_desc.ch_layout);
	_basic_desc.compressed = false;
}