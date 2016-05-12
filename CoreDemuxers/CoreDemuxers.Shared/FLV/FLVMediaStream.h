#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __FLV_MEDIA_STREAM_H
#define __FLV_MEDIA_STREAM_H

#include "FLVMediaFormat.h"

class FLVMediaStream : public IAVMediaStream
{
public:
	FLVMediaStream
		(std::shared_ptr<IAudioDescription>& audio_desc,MediaCodecType codec_type) throw();
	FLVMediaStream
		(std::shared_ptr<IVideoDescription>& video_desc,MediaCodecType codec_type,float fps) throw();

public:
	MediaMainType GetMainType();
	MediaCodecType GetCodecType();

	int GetStreamIndex();
	const char* GetStreamName();

	float GetContainerFps() { return _fps; }

	IAudioDescription* GetAudioInfo();
	IVideoDescription* GetVideoInfo();

private:
	MediaMainType _main_type;
	MediaCodecType _codec_type;

	int _stream_index;
	float _fps;

	std::shared_ptr<IAudioDescription> _audio_desc;
	std::shared_ptr<IVideoDescription> _video_desc;
};

#endif //__FLV_MEDIA_STREAM_H