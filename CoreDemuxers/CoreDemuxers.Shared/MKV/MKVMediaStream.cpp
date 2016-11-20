#include "MKVMediaStream.h"

MKVMediaStream::MKVMediaStream(MKVParser::MKVTrackInfo* info)
{
	memset(&_info,0,sizeof(_info));
	memcpy(&_info,info,sizeof(_info));

	_ssa_ass_format = false;
	_frame_duration = _info.InternalFrameDuration;
	_real_cook_audio = _real_rv40_video = false;
	_has_bFrame = false;
	_decode_timestamp = 0.0;
}
const char* MKVMediaStream::GetStreamName()
{
	if (_info.Name == nullptr ||
		strlen(_info.Name) == 0)
		return _info.Codec.CodecID;

	return _info.Name;
}

const char* MKVMediaStream::GetLanguageName()
{
	if (strlen(_info.LangID) == 0)
		return "und";
	return _info.LangID;
}

IAudioDescription* MKVMediaStream::GetAudioInfo()
{
	if (_main_type != MediaMainType::MEDIA_MAIN_TYPE_AUDIO)
		return nullptr;

	return _audio_desc.get();
}

IVideoDescription* MKVMediaStream::GetVideoInfo()
{
	if (_main_type != MediaMainType::MEDIA_MAIN_TYPE_VIDEO)
		return nullptr;

	return _video_desc.get();
}