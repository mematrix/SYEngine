#include "FLACParser.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/flac.h>
}

void FLACParser::Parse(unsigned char* buf, unsigned size) throw()
{
	unsigned char extradata[64] = {};
	if (size > 64)
		return;
	memcpy(extradata, buf, size);

	AVCodecContext* codec = avcodec_alloc_context3(NULL);
	if (codec == NULL)
		return;
	codec->extradata = &extradata[0];
	codec->extradata_size = size;
	FLACExtradataFormat format;
	uint8_t* head = NULL;
	if (ff_flac_is_extradata_valid(codec, &format, &head)) {
		FLACStreaminfo info = {};
		ff_flac_parse_streaminfo(codec, &info, head);
		nch = info.channels;
		rate = info.samplerate;
		bps = info.bps;
		max_blocksize = info.max_blocksize;
		max_framesize = info.max_framesize;
	}

	av_free(codec);
}