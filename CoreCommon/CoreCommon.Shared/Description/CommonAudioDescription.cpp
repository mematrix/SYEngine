#include <stdlib.h>
#include <memory.h>
#include "CommonAudioDescription.h"

CommonAudioDescription::CommonAudioDescription(CommonAudioCore& core) throw()
{
	memset(&_core,0,sizeof(_core));
	CopyAudioCore(&core);
}

CommonAudioDescription::~CommonAudioDescription()
{
	if (_core.extradata)
		free(_core.extradata);
	if (_core.profile)
		free(_core.profile);

	memset(&_core,0,sizeof(_core));
}

int CommonAudioDescription::GetType()
{
	return _core.type;
}

bool CommonAudioDescription::GetProfile(void* profile)
{
	if (profile == nullptr)
		return false;

	if (_core.profile == nullptr ||
		_core.profile_size == 0)
		return false;

	memcpy(profile,_core.profile,_core.profile_size);
	return true;
}

bool CommonAudioDescription::GetExtradata(void* p)
{
	if (p == nullptr)
		return false;

	if (_core.extradata == nullptr ||
		_core.extradata_size == 0)
		return false;

	memcpy(p,_core.extradata,_core.extradata_size);
	return true;
}

unsigned CommonAudioDescription::GetExtradataSize()
{
	if (_core.extradata == nullptr)
		return 0;

	return _core.extradata_size;
}

bool CommonAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr)
		return false;

	*desc = _core.desc;
	return true;
}

void CommonAudioDescription::CopyAudioCore(CommonAudioCore* pCore)
{
	_core.type = pCore->type;
	_core.desc = pCore->desc;
	
	if (pCore->profile != nullptr &&
		pCore->profile_size > 0) {
		_core.profile_size = pCore->profile_size;
		_core.profile = (unsigned char*)malloc(pCore->profile_size);
		if (_core.profile)
			memcpy(_core.profile,pCore->profile,pCore->profile_size);
	}

	if (pCore->extradata != nullptr &&
		pCore->extradata_size > 0) {
		_core.extradata_size = pCore->extradata_size;
		_core.extradata = (unsigned char*)malloc(pCore->extradata_size);
		if (_core.extradata)
			memcpy(_core.extradata,pCore->extradata,pCore->extradata_size);
	}
}