#ifndef __X264_VIDEO_DESCRIPTION_H
#define __X264_VIDEO_DESCRIPTION_H

#include <memory.h>
#include <memory>

#include "H264Spec.h"
#include "H264_SPS_Parser.h"
#include "H264AnnexBParser.h"

#include "AVDescription.h"

#define X264_VIDEO_DESC_TYPE MAKE_AV_DESC_TYPE('X','2','6','4')

struct H264_PROFILE_SPEC
{
	int profile;
	int level;
	int chroma_format; //1 == 4:2:0
	int ref_frames;
	int luma_bitdepth;
	int chroma_bitdepth;
	int nalu_size;
	int variable_framerate;
	double framerate;
};

class X264VideoDescription : public IVideoDescription
{
public:
	X264VideoDescription(unsigned char* pNaluArray,unsigned len) throw();

public:
	int GetType();
	bool GetProfile(void* profile);
	bool GetExtradata(void*);
	unsigned GetExtradataSize();

	const unsigned char* GetPrivateData() { return _extradata.get(); }
	unsigned GetPrivateSize() { return GetExtradataSize(); }

	bool GetVideoDescription(VideoBasicDescription* desc);
	bool ExternalUpdateVideoDescription(VideoBasicDescription* desc)
	{ _basic_desc = *desc; return true; }

protected:
	bool ParseSPS();
	void InitFromSPS(H264_SPS* sps);

private:
	void InternalUpdateExtradata();
	void InternalCopyExtradata(H264AnnexBParser& parser);

private:
	std::unique_ptr<H264AnnexBParser> _parser;
	bool _init_ok;

	std::unique_ptr<unsigned char> _extradata;
	unsigned _extradata_offset;
	unsigned _extradata_len;
	bool _extradata_ok;

	H264_PROFILE_SPEC _profile;
	VideoBasicDescription _basic_desc;
};

#endif //__X264_VIDEO_DESCRIPTION_H