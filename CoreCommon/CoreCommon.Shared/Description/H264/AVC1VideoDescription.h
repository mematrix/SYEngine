#ifndef __AVC1_VIDEO_DESCRIPTION_H
#define __AVC1_VIDEO_DESCRIPTION_H

#include "X264VideoDescription.h"

#define AVC1_VIDEO_DESC_TYPE MAKE_AV_DESC_TYPE('A','V','C','1')

class AVC1VideoDescription : public IVideoDescription
{
public:
	AVC1VideoDescription(unsigned char* avcc,unsigned avcc_size,unsigned width,unsigned height) throw();
	~AVC1VideoDescription() throw();

public:
	int GetType() { return AVC1_VIDEO_DESC_TYPE; }
	bool GetProfile(void* profile)
	{ if (profile) memcpy(profile,&_profile,sizeof(_profile)); return profile != nullptr; }
	bool GetExtradata(void* p)
	{ if (p) memcpy(p,_avcc,_avcc_size); return p != nullptr; }
	unsigned GetExtradataSize()
	{ return _avcc_size; }

	const unsigned char* GetPrivateData() { return _avcc; }
	unsigned GetPrivateSize() { return GetExtradataSize(); }

	bool GetVideoDescription(VideoBasicDescription* desc)
	{ if (desc) *desc = _basic_desc; return desc != nullptr; }
	bool ExternalUpdateVideoDescription(VideoBasicDescription* desc)
	{ _basic_desc = *desc; return true; }

private:
	VideoBasicDescription _basic_desc;
	H264_PROFILE_SPEC _profile;

	unsigned char* _avcc;
	unsigned _avcc_size;
};

#endif //__AVC1_VIDEO_DESCRIPTION_H