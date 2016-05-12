#ifndef __MEDIA_AV_STREAM_H
#define __MEDIA_AV_STREAM_H

#include "AVDescription.h"
#include "MediaCodecType.h"

struct MediaSubtitleStreamInfo
{
	MediaSubtitleTextType type;
	unsigned char* head_info;
	unsigned head_info_size;
	bool same_matroska;
};

class IAVMediaStream
{
public:
	virtual MediaMainType GetMainType() = 0;
	virtual MediaCodecType GetCodecType() = 0;

	virtual MediaSubtitleStreamInfo* GetSubtitleInfo() { return nullptr; }

	virtual int GetStreamIndex() = 0;

	virtual const char* GetStreamName() { return nullptr; }
	virtual const char* GetLanguageName() { return "und"; }

	virtual bool CopyAttachedData(void* copy_to) { return false; }
	virtual unsigned GetAttachedDataSize() { return 0; }

	virtual double GetDuration() { return 0; }
	virtual unsigned GetBitrate() { return 0; }
	virtual long long GetSamples() { return 0; }

	//for Isom\Mpeg4 format...
	virtual int GetRotation() { return 0; }
	virtual int GetRawTimescale() { return 0; }
	virtual float GetContainerFps() { return 0.0f; }

	virtual IAudioDescription* GetAudioInfo() { return nullptr; }
	virtual IVideoDescription* GetVideoInfo() { return nullptr; }

	virtual ~IAVMediaStream() {}
};

#endif //__MEDIA_AV_STREAM_H