#ifndef __MPEG4_VIDEO_DESCRIPTION_H
#define __MPEG4_VIDEO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "MPEG4HeadParser.h"
#include "AVDescription.h"

#define MPEG4P2_VIDEO_DESC_TYPE MAKE_AV_DESC_TYPE('M','4','P','2')

struct MPEG4_PROFILE_SPEC
{
	MPEG4V_UserData_Encoder encoder;
	MPEG4V_Profile_Level profile;
	bool chroma420;
	bool h263;
	int bits;
};

class MPEG4VideoDescription : public IVideoDescription
{
public:
	MPEG4VideoDescription(unsigned char* pb,unsigned len) throw();

public:
	int GetType() { return MPEG4P2_VIDEO_DESC_TYPE; }
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetVideoDescription(VideoBasicDescription* desc);

private:
	void FlushDesc(unsigned char* pb,unsigned size);

private:
	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_size;

	MPEG4_PROFILE_SPEC _profile;
	VideoBasicDescription _basic_desc;
};

#endif //__MPEG4_VIDEO_DESCRIPTION_H