#include "FLVMediaFormat.h"

#define _128KB 131072
#define _64KB 65536
#define _32KB 32768
#define _10MINUTE 600

AV_MEDIA_ERR FLVMediaFormat::Open(IAVMediaIO* io)
{
	if (io == nullptr)
		return AV_COMMON_ERR_INVALIDARG;
	if (_opened)
		return AV_COMMON_ERR_CHANGED_STATE;
	if (_av_io != nullptr)
		return AV_COMMON_ERR_ABORT;

	if (!RESET_AV_MEDIA_IO(io))
		return AV_IO_ERR_SEEK_FAILED;

	memset(&_stream_info,0,sizeof(_stream_info));

	auto parser = std::make_shared<FLVParser::FLVStreamParser>(
		static_cast<FLVParser::IFLVParserIO*>(this));

	_av_io = io;
	auto ret = parser->Open();
	if (ret != PARSER_FLV_OK)
		return AV_OPEN_ERR_PROBE_FAILED;

	FLVParser::FLV_STREAM_GLOBAL_INFO info = {};
	parser->GetStreamInfo(&info);
	if (info.duration == 0.0)
		return AV_OPEN_ERR_HEADER_INVALID;

	if (_force_avc1)
		parser->SetOutputRawAVC1();

	_stream_count = 2;
	if (info.no_audio_stream)
		_stream_count = 1;

	if (info.audio_type == FLVParser::SupportAudioStreamType::AudioStreamType_None ||
		info.video_type == FLVParser::SupportVideoStreamType::VideoStreamType_None)
		ret = InternalInitStreams(parser);

	if (ret != PARSER_FLV_OK)
		return AV_OPEN_ERR_STREAM_INVALID;

	if (!MakeAllStreams(parser))
		return AV_OPEN_ERR_STREAM_INVALID;

	unsigned io_buf_size = _128KB;
	if (info.no_keyframe_index)
	{
		io_buf_size = _128KB * 4;
		// If io_buf_size larger than bitrate of live stream, may cause more lantency
		// and more bandwidth pressure for media server 
		if (io->IsAliveStream())
			io_buf_size = _32KB;
		else if (info.duration > (_10MINUTE * 6))
			io_buf_size = _128KB * 10;
		else if (info.duration > (_10MINUTE * 3))
			io_buf_size = _128KB * 8;
		else if (info.duration > _10MINUTE)
			io_buf_size = _128KB * 6;
	}

	_io_pool = std::make_shared<IOPoolReader>(io,_force_io_pool_size == 0 ? io_buf_size:_force_io_pool_size);
	_av_io = _io_pool.get();

	if (!io->IsAliveStream()) {
		ret = parser->Reset();
		if (ret != PARSER_FLV_OK)
			return AV_COMMON_ERR_IO_ERROR;
	}

	if (_stream_info.video_info.fps != 0.0)
		_frame_duration = 1.0 / _stream_info.video_info.fps;
	if (_stream_info.audio_info.srate != 0)
	{
		if (_stream_info.audio_type == FLVParser::SupportAudioStreamType::AudioStreamType_MP3)
			_sound_duration = 1152.0 / (double)_stream_info.audio_info.srate;
		else if (_stream_info.audio_type == FLVParser::SupportAudioStreamType::AudioStreamType_AAC)
			_sound_duration = 1024.0 / (double)_stream_info.audio_info.srate;
		else
			_sound_duration = PACKET_NO_PTS;
	}

	if (_video_stream.get() && _video_stream->GetCodecType() == MEDIA_CODEC_VIDEO_H264) {
		VideoBasicDescription desc = {};
		_video_stream->GetVideoInfo()->GetVideoDescription(&desc);
		if (desc.scan_mode != VideoScanMode::VideoScanModeProgressive)
			_frame_duration = PACKET_NO_PTS;
		H264_PROFILE_SPEC profile = {};
		_video_stream->GetVideoInfo()->GetProfile(&profile);
		if (profile.variable_framerate)
			_frame_duration = PACKET_NO_PTS;
	}else{
		_frame_duration = PACKET_NO_PTS;
	}

	_parser = parser;
	_opened = true;
	return AV_ERR_OK;
}

AV_MEDIA_ERR FLVMediaFormat::Close()
{
	if (!_opened)
		return AV_CLOSE_ERR_NOT_OPENED;

	memset(&_stream_info,0,sizeof(_stream_info));

	if (_keyframe_index)
		free(_keyframe_index);
	_keyframe_index = nullptr;
	_keyframe_count = 0;

	_opened = false;
	return AV_ERR_OK;
}

double FLVMediaFormat::GetDuration()
{
	if (_opened)
		return _stream_info.duration;
	else
		return 0.0;
}

double FLVMediaFormat::GetStartTime()
{
	return 0.0;
}

unsigned FLVMediaFormat::GetBitRate()
{
	return 0;
}

int FLVMediaFormat::GetStreamCount()
{
	if (_opened)
		return _stream_count;
	else
		return -1;
}

IAVMediaStream* FLVMediaFormat::GetStream(int index)
{
	if (index >= _stream_count)
		return nullptr;

	if (_stream_count == 2)
	{
		if (index == 0)
			return _audio_stream.get();
		else
			return _video_stream.get();
	}else{
		return _video_stream.get();
	}
}

unsigned FLVMediaFormat::GetFormatFlags()
{
	return MEDIA_FORMAT_CAN_SEEK_ALL|MEDIA_FORMAT_CAN_FLUSH;
}

const char* FLVMediaFormat::GetFormatName()
{
	return MEDIA_FORMAT_FLV_NAME;
}

const char* FLVMediaFormat::GetMimeType()
{
	return MEDIA_FORMAT_FLV_MIME;
}

int FLVMediaFormat::GetKeyFramesCount()
{
	if (_parser == nullptr)
		return -1;

	auto kf = _parser->InternalGetKeyFrames();
	if (kf.first == nullptr || kf.second == 0)
		return -1;
	return kf.second;
}

unsigned FLVMediaFormat::CopyKeyFrames(double* copyTo,int copyCount)
{
	if (GetKeyFramesCount() < 1)
		return 0;
	if (copyTo == nullptr)
		return 0;
	if (copyCount != -1)
		return 0;

	unsigned result = 0;
	auto kf = _parser->InternalGetKeyFrames();
	auto time = copyTo;
	for (unsigned i = 0;i < kf.second;i++)
	{
		if ((kf.first + i)->time == 0.0)
			continue;
		
		*time = (kf.first + i)->time;
		time++;
		result++;
	}
	return result;
}