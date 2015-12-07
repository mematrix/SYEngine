#ifndef __MPEG_AUDIO_DESCRIPTION_H
#define __MPEG_AUDIO_DESCRIPTION_H

#include <memory.h>

#include "MPEG_Audio_Parser.h"
#include "AVDescription.h"

#define MPEG_MP1_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('M','P','1','A')
#define MPEG_MP2_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('M','P','2','A')
#define MPEG_MP3_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('M','P','3','A')

struct MPEG_AUDIO_PROFILE_SPEC
{
	int bitrate;
	int layer;
};

class MPEGAudioDescription : public IAudioDescription
{
public:
	MPEGAudioDescription(unsigned* ph) throw();

public:
	int GetType();
	bool GetProfile(void* profile);
	bool GetExtradata(void*) { return false; }
	unsigned GetExtradataSize() { return 0; }

	bool GetAudioDescription(AudioBasicDescription* desc);
	bool ExternalUpdateAudioDescription(AudioBasicDescription* desc)
	{ _basic_desc = *desc; return true; }

protected:
	void FlushAudioDescription(unsigned char* ph);

private:
	AudioBasicDescription _basic_desc;
	MPEG_AUDIO_PROFILE_SPEC _profile;
};

#endif //__MPEG_AUDIO_DESCRIPTION_H