#ifndef __COMMON_AUDIO_DESCRIPTION_H
#define __COMMON_AUDIO_DESCRIPTION_H

#include "AVDescription.h"

struct CommonAudioCore
{
	int type;
	AudioBasicDescription desc;
	unsigned char* profile;
	unsigned profile_size;
	unsigned char* extradata;
	unsigned extradata_size;
};

class CommonAudioDescription : public IAudioDescription
{
public:
	CommonAudioDescription(CommonAudioCore& core) throw();
	virtual ~CommonAudioDescription() throw();

public:
	int GetType();
	bool GetProfile(void* profile);
	bool GetExtradata(void* p);
	unsigned GetExtradataSize();

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void CopyAudioCore(CommonAudioCore* pCore);

private:
	CommonAudioCore _core;
};

#endif //__COMMON_AUDIO_DESCRIPTION_H