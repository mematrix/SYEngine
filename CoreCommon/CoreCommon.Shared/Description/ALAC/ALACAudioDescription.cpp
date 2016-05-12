#include "ALACAudioDescription.h"

ALACAudioDescription::ALACAudioDescription(unsigned char* ph,unsigned size) throw()
{
	memset(&_basic_desc,0,sizeof(_basic_desc));
	FlushAudioDescription(ph,size);
}

bool ALACAudioDescription::GetExtradata(void* p)
{
	if (p == nullptr ||
		_extradata.get() == nullptr)
		return 0;

	memcpy(p,_extradata.get(),_extradata_len);
	return true;
}

unsigned ALACAudioDescription::GetExtradataSize()
{
	return _extradata_len;
}

bool ALACAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void ALACAudioDescription::FlushAudioDescription(unsigned char* ph,unsigned size)
{
	if (ph == nullptr ||
		size < 24)
		return;

	ALACSpecificConfig info = {};
	ParseALACMaigcCode(ph,size,&info);
	if (info.frameLength == 0)
		return;

	_basic_desc.compressed = true;
	_basic_desc.nch = info.numChannels;
	_basic_desc.srate = info.sampleRate;
	_basic_desc.bits = info.bitDepth;
	_basic_desc.bitrate = info.avgBitRate;

	_extradata_len = 24;
	_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_len]);
	memcpy(_extradata.get(),ph,_extradata_len);
}