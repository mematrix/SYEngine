#ifndef __AC3_AUDIO_DESCRIPTION_H
#define __AC3_AUDIO_DESCRIPTION_H

#include <memory.h>

#include "A52_Parser.h"
#include "AVDescription.h"

#define A52_AUDIO_DESC_TYPE MAKE_AV_DESC_TYPE('A','5','2',' ')

class AC3AudioDescription : public IAudioDescription
{
public:
	AC3AudioDescription(unsigned char* ph) throw();

public:
	int GetType() { return A52_AUDIO_DESC_TYPE; }
	bool GetProfile(void* profile) { return false; }
	bool GetExtradata(void*) { return false; }
	unsigned GetExtradataSize() { return 0; }

	bool GetAudioDescription(AudioBasicDescription* desc);

protected:
	void FlushAudioDescription(unsigned char* ph);

private:
	AudioBasicDescription _basic_desc;
};

#endif //__AC3_AUDIO_DESCRIPTION_H