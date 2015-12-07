#include "MPEG2VideoDescription.h"

MPEG2VideoDescription::MPEG2VideoDescription(unsigned char* pb,unsigned len) throw()
{
	_extradata_size = 0;
	memset(&_basic_desc,0,sizeof(_basic_desc));

	FlushDesc(pb,len);
}

bool MPEG2VideoDescription::GetProfile(void* profile)
{
	if (_extradata_size == 0)
		return false;
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool MPEG2VideoDescription::GetExtradata(void* p)
{
	if (p != nullptr && _extradata_size != 0)
	{
		memcpy(p,_extradata.get(),_extradata_size);
		return true;
	}

	return false;
}

unsigned MPEG2VideoDescription::GetExtradataSize()
{
	return _extradata_size;
}

bool MPEG2VideoDescription::GetVideoDescription(VideoBasicDescription* desc)
{
	if (desc == nullptr)
		return false;
	if (_basic_desc.width == 0 ||
		_basic_desc.height == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void MPEG2VideoDescription::FlushDesc(unsigned char* pb,unsigned size)
{
	MPEG2Headers head = {};
	ParseMPEG2Headers(pb,size,&head);
	if (head.head.marker_bit == 0)
		return;

	InitMPEG2Headers(&head);

	_basic_desc.width = head.head.horizontal_size_value;
	_basic_desc.height = head.head.vertical_size_value;
	_basic_desc.bitrate = head._bitrate;
	
	_basic_desc.frame_rate.num = head._fps_num;
	_basic_desc.frame_rate.den = head._fps_den;
	_basic_desc.aspect_ratio.num = head._ar_num;
	_basic_desc.aspect_ratio.den = head._ar_den;

	_basic_desc.scan_mode = VideoScanModeProgressive;
	if (head.ex.progressive_sequence == 0) //http://bbs.dvbcn.com/showtopic-3526-1.html
		_basic_desc.scan_mode = VideoScanModeMixedInterlaceOrProgressive;

	_basic_desc.compressed = true;

	_profile.exists = false;
	if (head.ex.marker_bit)
	{
		//profile
		_profile.exists = true;

		_profile.colors = head._colors;
		_profile.profile = head._profile;
		_profile.level = head._level;
	}

	int offset = FindMPEG2StartCodeOffset(pb,size,MPEG2_START_CODE_GOP);
	if (offset < SIZE_OF_MPEG2_START_CODE)
		_extradata_size = size;
	else
		_extradata_size = offset - SIZE_OF_MPEG2_START_CODE;

	_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_size + 1]);
	memcpy(_extradata.get(),pb,_extradata_size);
}