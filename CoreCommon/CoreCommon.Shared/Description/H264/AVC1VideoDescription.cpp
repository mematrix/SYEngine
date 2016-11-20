#include <stdlib.h>
#include "AVC1VideoDescription.h"

unsigned AVCDecoderConfigurationRecord2AnnexB(unsigned char* src,unsigned char** dst,unsigned* profile,unsigned* level,unsigned* nal_size,unsigned max_annexb_size);

AVC1VideoDescription::AVC1VideoDescription(unsigned char* avcc,unsigned avcc_size,unsigned width,unsigned height) throw()
{
	memset(&_profile,0,sizeof(_profile));
	_avcc = nullptr;
	if (avcc)
	{
		_avcc = (decltype(_avcc))malloc(avcc_size / 4 * 4 + 8);
		if (_avcc)
		{
			memcpy(_avcc,avcc,avcc_size);
			_avcc_size = avcc_size;
		}
		_profile.profile = avcc[1];
		_profile.level = avcc[3];
	}
	_basic_desc.width = width;
	_basic_desc.height = height;

	unsigned nal_size = 0;
	unsigned char* ab = nullptr;
	unsigned size = AVCDecoderConfigurationRecord2AnnexB(avcc,&ab,nullptr,nullptr,&nal_size,2048);
	if (ab != nullptr)
	{
		auto h264 = std::make_shared<X264VideoDescription>(ab,size);
		free(ab);

		H264_PROFILE_SPEC profile = {};
		h264->GetProfile(&profile);
		if (profile.profile > 0 && profile.level > 0) {
			_profile = profile;
			h264->GetVideoDescription(&_basic_desc);
		}
	}
}

AVC1VideoDescription::~AVC1VideoDescription()
{
	if (_avcc)
		free(_avcc);
}