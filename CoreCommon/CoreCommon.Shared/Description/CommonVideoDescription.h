#ifndef __COMMON_VIDEO_DESCRIPTION_H
#define __COMMON_VIDEO_DESCRIPTION_H

#include "AVDescription.h"

struct CommonVideoCore
{
	int type;
	VideoBasicDescription desc;
	unsigned char* profile;
	unsigned profile_size;
	unsigned char* extradata;
	unsigned extradata_size;
};

class CommonVideoDescription : public IVideoDescription
{
public:
	CommonVideoDescription(CommonVideoCore& core) throw();
	virtual ~CommonVideoDescription() throw();

public:
	int GetType();
	bool GetProfile(void* profile);
	bool GetExtradata(void* p);
	unsigned GetExtradataSize();

	bool GetVideoDescription(VideoBasicDescription* desc);

protected:
	void CopyVideoCore(CommonVideoCore* pCore);

private:
	CommonVideoCore _core;
};

#endif //__COMMON_VIDEO_DESCRIPTION_H