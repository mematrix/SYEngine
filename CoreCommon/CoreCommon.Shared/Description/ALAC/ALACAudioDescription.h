#ifndef __ALAC_AUDIO_DESCRIPTION_H
#define __ALAC_AUDIO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "ALACMagicCodeParser.h"
#include "AVDescription.h"

#define ALAC_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('A','L','A','C')

class ALACAudioDescription : public IAudioDescription
{
public:
	ALACAudioDescription(unsigned char* ph,unsigned size) throw();

public:
	int GetType() { return ALAC_AUDIO_DESC_TYPE; }
	bool GetProfile(void* profile) { return false; }
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void FlushAudioDescription(unsigned char* ph,unsigned size);

private:
	AudioBasicDescription _basic_desc;

	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_len;
};

#endif //__ALAC_AUDIO_DESCRIPTION_H