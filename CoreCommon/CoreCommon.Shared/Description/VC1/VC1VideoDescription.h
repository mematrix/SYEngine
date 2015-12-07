#ifndef __VC1_VIDEO_DESCRIPTION_H
#define __VC1_VIDEO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "VC1HeadParser.h"
#include "AVDescription.h"

#define VC1_VIDEO_DESC_TYPE MAKE_AV_DESC_TYPE('V','C','-','1')

class VC1VideoDescription : public IVideoDescription
{
public:
	VC1VideoDescription(unsigned char* pb,unsigned len) throw();

public:
	int GetType() { return VC1_VIDEO_DESC_TYPE; }
	bool GetProfile(void* profile) { return false; }
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	bool GetVideoDescription(VideoBasicDescription* desc);

private:
	void FlushDesc(unsigned char* pb,unsigned size);

private:
	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_size;

	VideoBasicDescription _basic_desc;
};

#endif //__VC1_VIDEO_DESCRIPTION_H