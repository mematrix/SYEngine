#ifndef __BD_AUDIO_DESCRIPTION_H
#define __BD_AUDIO_DESCRIPTION_H

#include <memory.h>

#include "BD_LPCM_Parser.h"
#include "AVDescription.h"

#define BD_LPCM_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('B','D','A','D')

class BDAudioDescription : public IAudioDescription
{
public:
	BDAudioDescription(unsigned* ph) throw();

public:
	int GetType() { return BD_LPCM_AUDIO_DESC_TYPE; }
	bool GetProfile(void*) { return false; }
	bool GetExtradata(void*) { return false; }
	unsigned GetExtradataSize() { return 0; }

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void FlushAudioDescription(unsigned char* ph);

private:
	AudioBasicDescription _basic_desc;
};

#endif //__BD_AUDIO_DESCRIPTION_H