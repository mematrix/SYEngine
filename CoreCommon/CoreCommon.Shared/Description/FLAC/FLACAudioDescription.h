#ifndef __FLAC_AUDIO_DESCRIPTION_H
#define __FLAC_AUDIO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "FLACStreamInfoParser.h"
#include "AVDescription.h"

#define FLAC_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('F','L','A','C')

struct FLAC_PROFILE_SPEC
{
	unsigned min_block_size;
	unsigned max_block_size;
	unsigned min_frame_size;
	unsigned max_frame_size;
};

class FLACAudioDescription : public IAudioDescription
{
public:
	FLACAudioDescription(unsigned char* ph,unsigned size) throw();

public:
	int GetType() { return FLAC_AUDIO_DESC_TYPE; }
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void FlushAudioDescription(unsigned char* ph,unsigned size);

private:
	AudioBasicDescription _basic_desc;
	FLAC_PROFILE_SPEC _profile;

	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_len;
};

#endif //__FLAC_AUDIO_DESCRIPTION_H