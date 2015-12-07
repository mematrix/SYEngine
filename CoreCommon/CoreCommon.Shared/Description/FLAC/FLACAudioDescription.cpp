#include "FLACAudioDescription.h"

FLACAudioDescription::FLACAudioDescription(unsigned char* ph,unsigned size)
{
	memset(&_basic_desc,0,sizeof(_basic_desc));
	memset(&_profile,0,sizeof(_profile));

	FlushAudioDescription(ph,size);
}

bool FLACAudioDescription::GetProfile(void* profile)
{
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool FLACAudioDescription::GetExtradata(void* p)
{
	if (p == nullptr ||
		_extradata.get() == nullptr)
		return 0;

	memcpy(p,_extradata.get(),_extradata_len);
	return true;
}

unsigned FLACAudioDescription::GetExtradataSize()
{
	return _extradata_len;
}

bool FLACAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void FLACAudioDescription::FlushAudioDescription(unsigned char* ph,unsigned size)
{
	if (ph == nullptr ||
		size == 0)
		return;

	int offset = FindFlacStreamInfoOffset(ph,size);
	if (offset <= 0)
		return;

	FlacStreamInfo info = {};
	unsigned n = ParseFlacStreamInfo(ph + offset,&info);
	if (n == 0)
		return;

	_profile.max_block_size = info.MaxBlockSize;
	_profile.max_frame_size = info.MaxFrameSize;
	_profile.min_block_size = info.MinBlockSize;
	_profile.min_frame_size = info.MinFrameSize;

	_basic_desc.compressed = true;
	_basic_desc.nch = info.Channels;
	_basic_desc.bits = info.Bits;
	_basic_desc.srate = info.SampleRate;

	_extradata_len = n + 4;
	_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_len]);
	memcpy(_extradata.get(),ph,_extradata_len);
	_extradata.get()[4] = 0x80; //mark StreamInfo with end-flag.
}