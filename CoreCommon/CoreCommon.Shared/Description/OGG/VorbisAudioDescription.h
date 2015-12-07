#ifndef __VORBIS_AUDIO_DESCRIPTION_H
#define __VORBIS_AUDIO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "AVDescription.h"

#define OGG_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('O','G','G','S')

struct VORBIS_PROFILE_SPEC
{
	unsigned max_bitrate;
	unsigned min_bitrate;
	unsigned bitrate;
	unsigned block_size0;
	unsigned block_size1;
};

class VorbisAudioDescription : public IAudioDescription
{
public:
	VorbisAudioDescription(unsigned char* ph,unsigned size) throw();

public:
	int GetType() { return OGG_AUDIO_DESC_TYPE; }
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void FlushAudioDescription(unsigned char* ph,unsigned size);

private:
	AudioBasicDescription _basic_desc;
	VORBIS_PROFILE_SPEC _profile;

	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_len;
};

#endif //__VORBIS_AUDIO_DESCRIPTION_H