#include "FLVMediaFormat.h"

unsigned FLVMediaFormat::InternalInitStreams(std::shared_ptr<FLVParser::FLVStreamParser>& parser)
{
	unsigned result = PARSER_FLV_OK;
	int fail_count = 0;
	int max_count = 1;

	FLVParser::FLV_STREAM_GLOBAL_INFO info = {};
	parser->GetStreamInfo(&info);
	if (info.video_info.fps < 120.0)
		max_count = (int)info.video_info.fps;

	_skip_unknown_stream = false;
	while (1)
	{
		FLVParser::FLV_STREAM_PACKET packet = {};
		auto ret = parser->ReadNextPacket(&packet);
		if (ret == PARSER_FLV_ERR_VIDEO_STREAM_UNSUPPORTED ||
			ret == PARSER_FLV_ERR_AUDIO_STREAM_UNSUPPORTED ||
			ret == PARSER_FLV_ERR_VIDEO_H263_STREAM_UNSUPPORTED) {
			if (fail_count > max_count) {
				result = ret;
				break;
			}
			fail_count++;
			continue;
		}
		if (ret != PARSER_FLV_OK)
		{
			result = ret;
			break;
		}
		if (fail_count > 0)
			_skip_unknown_stream = true;

		parser->GetStreamInfo(&_stream_info);
		if (_stream_count == 1)
		{
			if (_stream_info.video_type == FLVParser::VideoStreamType_AVC)
				if (_stream_info.delay_flush_spec_info.avc_spec_info != nullptr)
					break;
			if (_stream_info.video_type == FLVParser::VideoStreamType_VP6)
				break;

			continue;
		}

		if (_stream_info.audio_type == FLVParser::AudioStreamType_None)
			continue;
		if (_stream_info.video_type == FLVParser::VideoStreamType_None)
			continue;

		if (_stream_info.audio_type == FLVParser::AudioStreamType_AAC)
		{
			if (_stream_info.delay_flush_spec_info.aac_spec_info == nullptr)
				continue;
		}
		if (_stream_info.video_type == FLVParser::VideoStreamType_AVC)
		{
			if (_stream_info.delay_flush_spec_info.avc_spec_info == nullptr)
				continue;
		}

		break;
	}

	return result;
}

bool FLVMediaFormat::MakeAllStreams(std::shared_ptr<FLVParser::FLVStreamParser>& parser)
{
	if (_stream_info.video_type != FLVParser::VideoStreamType_AVC &&
		_stream_info.video_type != FLVParser::VideoStreamType_VP6)
		return false;
	if (_stream_info.audio_type != FLVParser::AudioStreamType_AAC &&
		_stream_info.audio_type != FLVParser::AudioStreamType_MP3 &&
		_stream_info.audio_type != FLVParser::AudioStreamType_PCM &&
		_stream_info.no_audio_stream == 0)
		return false;

	if (_stream_info.video_type == FLVParser::VideoStreamType_AVC) {
		std::shared_ptr<IVideoDescription> h264;
		if (!_force_avc1)
			h264 = std::make_shared<X264VideoDescription>
				(_stream_info.delay_flush_spec_info.avc_spec_info,_stream_info.delay_flush_spec_info.avc_info_size);
		else
			h264 = std::make_shared<AVC1VideoDescription>
				(_stream_info.delay_flush_spec_info.avcc,_stream_info.delay_flush_spec_info.avcc_size,
				_stream_info.video_info.width,_stream_info.video_info.height);

		H264_PROFILE_SPEC profile = {};
		h264->GetProfile(&profile);
		if (profile.profile == 0 && !_force_avc1)
			return false;

		VideoBasicDescription vdesc = {};
		h264->GetVideoDescription(&vdesc);
		vdesc.bitrate = _stream_info.video_info.bitrate * 1000;

		if (profile.variable_framerate && _stream_info.video_info.fps != 0.0)
		{
			//process variable_framerate.
			vdesc.frame_rate.den = 10000000;
			vdesc.frame_rate.num = (int)(_stream_info.video_info.fps * 10000000.0);
		}
		h264->ExternalUpdateVideoDescription(&vdesc);

		_video_stream = std::make_shared<FLVMediaStream>(h264,
			MEDIA_CODEC_VIDEO_H264,
			float(_stream_info.video_info.fps));
	}else{
		if (_stream_info.video_info.width == 0 ||
			_stream_info.video_info.height == 0)
			return false;

		parser->Reset();
		unsigned char mask = 0;
		while (1)
		{
			FLVParser::FLV_STREAM_PACKET packet = {};
			if (parser->ReadNextPacket(&packet) != PARSER_FLV_OK)
				return false;
			if (packet.type != FLVParser::PacketTypeVideo || packet.skip_this == 1)
				continue;

			mask = packet.data_buf[0];
			break;
		}
		parser->Reset();

		CommonVideoCore comm = {};
		comm.type = -1;
		comm.desc.aspect_ratio.num = comm.desc.aspect_ratio.den = 1;
		comm.desc.scan_mode = VideoScanModeMixedInterlaceOrProgressive;
		comm.desc.width = _stream_info.video_info.width;
		comm.desc.height = _stream_info.video_info.height;
		comm.desc.frame_rate.num = (int)(_stream_info.video_info.fps * 10000000.0);
		comm.desc.frame_rate.den = 10000000;
		comm.desc.compressed = true;
		comm.extradata = &mask;
		comm.extradata_size = 1;

		std::shared_ptr<IVideoDescription> vp6 = std::make_shared<CommonVideoDescription>(comm);
		_video_stream = std::make_shared<FLVMediaStream>(vp6,
			MEDIA_CODEC_VIDEO_VP6F,
			float(_stream_info.video_info.fps));
	}

	if (_stream_info.audio_type == FLVParser::AudioStreamType_AAC)
	{
		std::shared_ptr<IAudioDescription> aac = std::make_shared<ADTSAudioDescription>(
			(unsigned*)_stream_info.delay_flush_spec_info.aac_spec_info,true,_stream_info.delay_flush_spec_info.aac_info_size);

		AudioBasicDescription desc = {};
		aac->GetAudioDescription(&desc);
		if (desc.nch == 0)
			return false;

		if (_stream_info.audio_info.bitrate > 0)
			UpdateAudioDescriptionBitrate(aac.get(),_stream_info.audio_info.bitrate * 1000);
		_audio_stream = std::make_shared<FLVMediaStream>(aac,MEDIA_CODEC_AUDIO_AAC);
	}else if (_stream_info.audio_type == FLVParser::AudioStreamType_MP3)
	{
		parser->Reset();
		unsigned mp3_head = 0;
		while (1)
		{
			FLVParser::FLV_STREAM_PACKET packet = {};
			auto ret = parser->ReadNextPacket(&packet);
			if (ret != PARSER_FLV_OK)
				return false;

			if (packet.type != FLVParser::PacketTypeAudio ||
				packet.data_size < 8 || packet.skip_this == 1)
				continue;

			mp3_head = *(unsigned*)packet.data_buf;
			break;
		}
		parser->Reset();

		if (mp3_head == 0)
			return false;

		std::shared_ptr<IAudioDescription> mp3 = std::make_shared<MPEGAudioDescription>(
			&mp3_head);

		AudioBasicDescription desc = {};
		mp3->GetAudioDescription(&desc);
		if (desc.nch == 0)
			return false;

		_audio_stream = std::make_shared<FLVMediaStream>(mp3,MEDIA_CODEC_AUDIO_MP3);
	}else if (_stream_info.audio_type == FLVParser::AudioStreamType_PCM)
	{
		CommonAudioCore comm = {};
		comm.type = 3;
		comm.desc.bits = _stream_info.audio_info.bits;
		comm.desc.nch = _stream_info.audio_info.nch;
		comm.desc.srate = _stream_info.audio_info.srate;
		comm.desc.wav_block_align = comm.desc.nch * (comm.desc.bits / 8);
		comm.desc.wav_avg_bytes = comm.desc.wav_block_align * comm.desc.srate;

		std::shared_ptr<IAudioDescription> pcm = std::make_shared<CommonAudioDescription>(comm);
		_audio_stream = std::make_shared<FLVMediaStream>(pcm,MEDIA_CODEC_PCM_SINT_LE);
	}

	return true;
}