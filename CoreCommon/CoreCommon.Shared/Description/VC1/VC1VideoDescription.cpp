#include "VC1VideoDescription.h"

VC1VideoDescription::VC1VideoDescription(unsigned char* pb,unsigned len) throw()
{
	_extradata_size = 0;
	memset(&_basic_desc,0,sizeof(_basic_desc));

	auto offset = FindVC1StartCodeOffset(pb,len,0);
	FlushDesc(pb + offset,len - offset);
}

bool VC1VideoDescription::GetExtradata(void* p)
{
	if (p != nullptr && _extradata_size != 0)
	{
		memcpy(p,_extradata.get(),_extradata_size);
		return true;
	}

	return false;
}

unsigned VC1VideoDescription::GetExtradataSize()
{
	return _extradata_size;
}

bool VC1VideoDescription::GetVideoDescription(VideoBasicDescription* desc)
{
	if (desc == nullptr)
		return false;
	if (_basic_desc.width == 0 ||
		_basic_desc.height == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void VC1VideoDescription::FlushDesc(unsigned char* pb,unsigned size)
{
	VC1_SEQUENCE_HEADER head = {};
	if (ParseVC1Header(pb,size,&head) > 4)
	{
		_basic_desc.width = head.display_info.width;
		if (_basic_desc.width == 0)
			_basic_desc.width = head.max_coded_width;
		_basic_desc.height = head.display_info.height;
		if (_basic_desc.height == 0)
			_basic_desc.height = head.max_coded_height;

		_basic_desc.frame_rate.den = head.display_info.framerate_num;
		_basic_desc.frame_rate.num = head.display_info.framerate_den;

		_basic_desc.aspect_ratio.den = head.display_info.aspect_ratio_width;
		_basic_desc.aspect_ratio.num = head.display_info.aspect_ratio_height;

		if (head.display_info.aspect_ratio_width == 0)
			_basic_desc.aspect_ratio.den = _basic_desc.aspect_ratio.num = 1;

		if (head.interlace_flag == VC1_PICTURE_MODE_PROGRESSIVE)
			_basic_desc.scan_mode = VideoScanModeProgressive;
		else
			_basic_desc.scan_mode = VideoScanModeMixedInterlaceOrProgressive;

		_basic_desc.compressed = true;

		int offset = FindVC1StartCodeOffset(pb,size - 4,VC1_HEAD_CODE_FRAME);
		if (offset == -1)
			_extradata_size = size;
		else
			_extradata_size = offset;

		_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_size + 1]);
		memcpy(_extradata.get(),pb,_extradata_size);
	}
}