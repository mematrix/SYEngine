#include "MPEG4VideoDescription.h"

MPEG4VideoDescription::MPEG4VideoDescription(unsigned char* pb,unsigned len) throw()
{
	_extradata_size = 0;
	memset(&_basic_desc,0,sizeof(_basic_desc));

	FlushDesc(pb,len);
}

bool MPEG4VideoDescription::GetProfile(void* profile)
{
	if (_extradata_size == 0)
		return false;
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool MPEG4VideoDescription::GetExtradata(void* p)
{
	if (p != nullptr && _extradata_size != 0)
	{
		memcpy(p,_extradata.get(),_extradata_size);
		return true;
	}

	return false;
}

unsigned MPEG4VideoDescription::GetExtradataSize()
{
	return _extradata_size;
}

bool MPEG4VideoDescription::GetVideoDescription(VideoBasicDescription* desc)
{
	if (desc == nullptr)
		return false;
	if (_basic_desc.width == 0 ||
		_basic_desc.height == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void MPEG4VideoDescription::FlushDesc(unsigned char* pb,unsigned size)
{
	MPEG4V_PrivateData info = {};
	if (!ParseMPEG4VPrivateData(pb,size,&info))
		return;
	if (info.width == 0 || info.height == 0)
		return;

	_basic_desc.compressed = true;
	_basic_desc.width = info.width;
	_basic_desc.height = info.height;
	_basic_desc.aspect_ratio.num = info.parWidth;
	_basic_desc.aspect_ratio.den = info.parHeight;
	if (info.vop_time_increment_resolution != 0 && info.fixed_vop_time_increment != 0)
	{
		_basic_desc.frame_rate.num = info.vop_time_increment_resolution;
		_basic_desc.frame_rate.den = info.fixed_vop_time_increment;
	}else{
		_basic_desc.frame_rate.num = 1000000;
		_basic_desc.frame_rate.den = 417083;
	}

	_basic_desc.scan_mode = VideoScanModeProgressive;
	if (info.interlaced)
		_basic_desc.scan_mode = VideoScanModeInterleavedUpperFirst;

	_profile.encoder = info.Encoder;
	_profile.profile = info.ProfileLevel;
	_profile.chroma420 = (info.chromaFormat == 1);
	_profile.h263 = (info.quant_type == 1);
	_profile.bits = info.bitsPerPixel;

	_extradata_size = size;
	_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_size + 1]);
	memcpy(_extradata.get(),pb,_extradata_size);
}