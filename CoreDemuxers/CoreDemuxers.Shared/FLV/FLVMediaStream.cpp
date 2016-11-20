#include "FLVMediaStream.h"

FLVMediaStream::FLVMediaStream(std::shared_ptr<IAudioDescription>& audio_desc,MediaCodecType codec_type) throw()
{
	_stream_index = 0;

	_main_type = MediaMainType::MEDIA_MAIN_TYPE_AUDIO;
	_codec_type = codec_type;

	_audio_desc = audio_desc;
}

FLVMediaStream::FLVMediaStream(std::shared_ptr<IVideoDescription>& video_desc,MediaCodecType codec_type,float fps) throw()
{
	_stream_index = 1;

	_main_type = MediaMainType::MEDIA_MAIN_TYPE_VIDEO;
	_codec_type = codec_type;

	_video_desc = video_desc;
	_fps = fps;
}

MediaMainType FLVMediaStream::GetMainType()
{
	return _main_type;
}

MediaCodecType FLVMediaStream::GetCodecType()
{
	return _codec_type;
}

int FLVMediaStream::GetStreamIndex()
{
	return _stream_index;
}

const char* FLVMediaStream::GetStreamName()
{
	return _main_type == MEDIA_MAIN_TYPE_AUDIO ?
		"FLV Audio Stream":"FLV Video Stream";
}

IAudioDescription* FLVMediaStream::GetAudioInfo()
{
	if (_main_type != MediaMainType::MEDIA_MAIN_TYPE_AUDIO)
		return nullptr;

	return _audio_desc.get();
}

IVideoDescription* FLVMediaStream::GetVideoInfo()
{
	if (_main_type != MediaMainType::MEDIA_MAIN_TYPE_VIDEO)
		return nullptr;

	return _video_desc.get();
}