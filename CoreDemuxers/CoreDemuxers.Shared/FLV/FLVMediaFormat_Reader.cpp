#include "FLVMediaFormat.h"

AV_MEDIA_ERR FLVMediaFormat::Flush()
{
	if (!_opened)
		return AV_FLUSH_ERR_NOT_OPENED;

	return AV_ERR_OK;
}

AV_MEDIA_ERR FLVMediaFormat::Reset()
{
	if (!_opened)
		return AV_FLUSH_ERR_NOT_OPENED;

	if (_parser->Reset() != PARSER_FLV_OK)
		return AV_SEEK_ERR_PARSER_FAILED;

	return AV_ERR_OK;
}

AV_MEDIA_ERR FLVMediaFormat::Seek(double seconds,bool key_frame,AVSeekDirection seek_direction)
{
	if (!_opened)
		return AV_READ_PACKET_ERR_NOT_OPENED;
	
	if (seconds > _stream_info.duration)
		return AV_COMMON_ERR_INVALID_OPERATION;

	if (seconds < 0.1)
		return Reset();

	auto ret = _parser->Seek(seconds,seek_direction == 
		AVSeekDirection::SeekDirection_Next ? true:false);
	if (ret != PARSER_FLV_OK && ret != PARSER_FLV_ERR_NON_KEY_FRAME_INDEX)
		return AV_SEEK_ERR_PARSER_FAILED;
	else
		return ret == PARSER_FLV_OK ? AV_ERR_OK:InternalSeek(seconds);

	return AV_ERR_OK;
}

AV_MEDIA_ERR FLVMediaFormat::ReadPacket(AVMediaPacket* packet)
{
	if (!_opened)
		return AV_READ_PACKET_ERR_NOT_OPENED;
	if (packet == nullptr)
		return AV_COMMON_ERR_INVALIDARG;

	FLVParser::FLV_STREAM_PACKET flv_pkt = {};
	while (1) {
		auto ret = _parser->ReadNextPacket(&flv_pkt);
		if (ret != PARSER_FLV_OK) {
			if (_skip_unknown_stream) {
				if (ret == PARSER_FLV_ERR_AUDIO_STREAM_UNSUPPORTED ||
					ret == PARSER_FLV_ERR_VIDEO_STREAM_UNSUPPORTED ||
					ret == PARSER_FLV_ERR_VIDEO_H263_STREAM_UNSUPPORTED)
					continue;
			}
			return AV_READ_PACKET_ERR_NON_MORE;
		}
		break;
	}

	if (flv_pkt.skip_this == 1 || flv_pkt.data_size == 0)
	{
		memset(packet,0,sizeof(AVMediaPacket));

		packet->flag = MEDIA_PACKET_CAN_TO_SKIP_FLAG|
			MEDIA_PACKET_BUFFER_NONE_FLAG;
		packet->pts = PACKET_NO_PTS;
		packet->stream_index = -1;

		return AV_ERR_OK;
	}

	packet->data.size = flv_pkt.data_size;
	packet->data.buf = flv_pkt.data_buf;
	/*
	if (AllocMediaPacketAndCopy(packet,flv_pkt.data_buf,flv_pkt.data_size) == 0)
		return AV_READ_PACKET_ERR_ALLOC_PKT;
	*/

	packet->position = -1;
	packet->duration = PACKET_NO_PTS;

	if (flv_pkt.type == FLVParser::PacketTypeAudio)
		packet->stream_index = 0;
	else
		packet->stream_index = 1;

	//packet->pts = (double)flv_pkt.timestamp / (double)1000.0;
	packet->pts = (double)flv_pkt.timestamp * 0.001; //1 / 1000 = 0.001
	packet->dts = (double)flv_pkt.dts * 0.001;
	packet->flag = flv_pkt.key_frame != 0 ? MEDIA_PACKET_KEY_FRAME_FLAG:0;
	packet->flag |= MEDIA_PACKET_FIXED_BUFFER_FLAG;

	if (flv_pkt.type == FLVParser::PacketTypeVideo &&
		_frame_duration != PACKET_NO_PTS && _frame_duration != 0.0)
		packet->duration = _frame_duration;
	else if (flv_pkt.type == FLVParser::PacketTypeAudio && _stream_info.audio_info.srate > 0 &&
		_sound_duration != PACKET_NO_PTS && _sound_duration != 0.0)
		packet->duration = _sound_duration;

	return AV_ERR_OK;
}

AV_MEDIA_ERR FLVMediaFormat::InternalSeek(double seconds)
{
	if (_keyframe_count == 0 && _keyframe_index == nullptr)
	{
		if (!FindAndMakeKeyFrames())
			return AV_SEEK_ERR_PARSER_FAILED;
	}

	long long file_pos = 0;
	for (unsigned i = 0;i < _keyframe_count;i++)
	{
		auto p = _keyframe_index + i;
		if (p->time >= seconds)
		{
			double next_time = p->time;
			double prev_time = 0.0;
			if (i > 0)
				prev_time = (_keyframe_index + (i - 1))->time;

			double next_offset = next_time - seconds;
			double prev_offset = seconds - prev_time;

			if (i == 0)
				file_pos = p->pos;
			else
				file_pos = (_keyframe_index + (i - 1))->pos;

			break;
		}
	}

	if (file_pos == 0 && _keyframe_count > 0)
	{
		if (seconds > (_keyframe_index + (_keyframe_count - 1))->time)
			file_pos = (_keyframe_index + (_keyframe_count - 1))->pos;
	}

	if (file_pos == 0)
		return AV_COMMON_ERR_IO_ERROR;
	if (!Seek(file_pos - 4))
		return AV_IO_ERR_SEEK_FAILED;

	return AV_ERR_OK;
}