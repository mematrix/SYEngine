#ifndef __MPEG2_VIDEO_DESCRIPTION_H
#define __MPEG2_VIDEO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "MPEG2HeadParser.h"
#include "AVDescription.h"

#define MPEG_VIDEO_DESC_TYPE MAKE_AV_DESC_TYPE('M','P','E','G')

struct MPEG2_PROFILE_SPEC
{
	bool exists;
	MPEG2Profile profile;
	MPEG2Level level;
	MPEG2Colors colors;
};

class MPEG2VideoDescription : public IVideoDescription
{
public:
	MPEG2VideoDescription(unsigned char* pb,unsigned len) throw();

public:
	int GetType() { return MPEG_VIDEO_DESC_TYPE; }
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetVideoDescription(VideoBasicDescription* desc);

private:
	void FlushDesc(unsigned char* pb,unsigned size);

private:
	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_size;

	MPEG2_PROFILE_SPEC _profile;
	VideoBasicDescription _basic_desc;
};

#endif //__MPEG2_VIDEO_DESCRIPTION_H