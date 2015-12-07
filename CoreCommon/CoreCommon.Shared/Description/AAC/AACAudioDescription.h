#ifndef __AAC_AUDIO_DESCRIPTION_H
#define __AAC_AUDIO_DESCRIPTION_H

#include <memory.h>

#include "AAC_ADTS_Parser.h"
#include "AAC_LATM_Parser.h"
#include "AVDescription.h"

#define AAC_ADTS_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('A','D','T','S')
#define AAC_RAW_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('A','A','C',' ')

struct AAC_PROFILE_SPEC
{
	int profile;
	int level;
	int rate_index;
	int he_with_lc_core;
};

class ADTSAudioDescription : public IAudioDescription
{
public:
	ADTSAudioDescription(unsigned* ph) throw();
	ADTSAudioDescription(unsigned* ph,bool asc) throw();

public:
	int GetType();
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	const unsigned char* GetPrivateData() { return _asc; }
	unsigned GetPrivateSize() { return GetExtradataSize(); }

	bool GetAudioDescription(AudioBasicDescription* desc);
	bool ExternalUpdateAudioDescription(AudioBasicDescription* desc)
	{ _basic_desc = *desc; return true; }

protected:
	void FlushAudioDescription(unsigned char* ph);
	void FlushAudioDescription2(unsigned char* ph);

private:
	AudioBasicDescription _basic_desc;
	AAC_PROFILE_SPEC _profile;

	unsigned char _asc[2];
};

#endif //__AAC_AUDIO_DESCRIPTION_H