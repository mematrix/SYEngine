#include <malloc.h>
#include "AVC1VideoDescription.h"

AVC1VideoDescription::AVC1VideoDescription(unsigned char* avcc,unsigned avcc_size,unsigned width,unsigned height)
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
}

AVC1VideoDescription::~AVC1VideoDescription()
{
	if (_avcc)
		free(_avcc);
}