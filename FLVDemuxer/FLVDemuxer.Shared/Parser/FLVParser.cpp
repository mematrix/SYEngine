/*
 - FLV Stream Parser (Kernel) -
 - Support H264 + AAC and MP3 -

 - Author: K.F Yang
 - Date: 2014-11-26
*/

#include "FLVParser.h"

bool ReadFlvStreamTag(FLVParser::IFLVParserIO* pIo,unsigned& tag_type,unsigned& data_size,unsigned& timestamp);
unsigned VerifyFlvStreamHeader(FLVParser::IFLVParserIO* pIo,unsigned& av_flag);
unsigned AMFParseFindOnMetadata(unsigned char* pb,unsigned len);
unsigned AMFParseObjectAndInit(unsigned char* pb,unsigned len,char* key,FLVParser::FLV_STREAM_GLOBAL_INFO* global_info,FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX** kf_index,unsigned* kf_count);

using namespace FLVParser;

static inline unsigned GetFrameNALUSize(unsigned char* p,unsigned lengthSizeMinusOne)
{
	if (lengthSizeMinusOne == 4)
		return FLV_SWAP32(*(unsigned*)p);
	else if (lengthSizeMinusOne == 3)
		return ((FLV_SWAP32(*(unsigned*)p)) >> 8);
	else if (lengthSizeMinusOne == 2)
		return FLV_SWAP16(*(unsigned short*)p);
	else if (lengthSizeMinusOne == 1)
		return *p;

	return 0xFFFFFFFF;
}

FLVStreamParser::FLVStreamParser(IFLVParserIO* pIo) throw() : _flv_io(pIo), _keyframe_index(nullptr), _keyframe_count(0)
{
	_output_avc1 = false;
	_lengthSizeMinusOne = 0;

	memset(&_global_info,0,sizeof(_global_info));

	memset(&_pkt_buf_audio,0,sizeof(_pkt_buf_audio));
	memset(&_pkt_buf_video,0,sizeof(_pkt_buf_video));

	_global_info.audio_info.raw_codec_id = _global_info.video_info.raw_codec_id = -1;
}

FLVStreamParser::~FLVStreamParser() throw()
{
	if (_keyframe_index)
		free(_keyframe_index);

	if (_global_info.delay_flush_spec_info.aac_spec_info)
		free(_global_info.delay_flush_spec_info.aac_spec_info);
	if (_global_info.delay_flush_spec_info.avc_spec_info)
		free(_global_info.delay_flush_spec_info.avc_spec_info);

	if (_global_info.delay_flush_spec_info.avcc)
		free(_global_info.delay_flush_spec_info.avcc);
}

unsigned FLVStreamParser::Open(bool update_duration_from_fileend)
{
	unsigned av_flag = 0;
	_flv_head_offset = VerifyFlvStreamHeader(_flv_io,av_flag);
	if (_flv_head_offset == 0)
		return PARSER_FLV_ERR_HEAD_INVALID;

	if ((av_flag & FLV_HEAD_HAS_VIDEO_FLAG) == 0)
		return PARSER_FLV_ERR_NON_VIDEO_STREAM;
	//if ((av_flag & FLV_HEAD_HAS_AUDIO_FLAG) == 0)
		//return PARSER_FLV_ERR_NON_AUDIO_STREAM;

	//如果FLV头不是9字节，跳过垃圾部分
	if (_flv_head_offset > FLV_HEAD_LENGTH)
	{
		if (!_flv_io->Seek(_flv_head_offset))
			return PARSER_FLV_ERR_IO_SEEK_FAILED;
	}

	unsigned tag_type = 0,tag_size = 0,tag_time = 0;
	if (!ReadFlvStreamTag(_flv_io,
		tag_type,tag_size,tag_time))
		return PARSER_FLV_ERR_METADATA_MISS;

	bool searchDurationFromFileEnd = false;
	if (tag_type == FLV_TAG_TYPE_META && tag_size > 0)
	{
		AutoBuffer metadata(tag_size);
		if (metadata.Get<int>() == nullptr)
			return PARSER_FLV_ERR_BAD_ALLOC;

		auto rs = _flv_io->Read(metadata.Get<void>(),tag_size);
		if (rs != tag_size)
			return PARSER_FLV_ERR_IO_READ_FAILED;

		unsigned offset = AMFParseFindOnMetadata(metadata.Get<unsigned char>(),tag_size);
		if (offset == 0)
			return PARSER_FLV_ERR_METADATA_INVALID;

		offset = AMFParseObjectAndInit(metadata.Get<unsigned char>() + offset,tag_size - offset,
			nullptr,&_global_info,&_keyframe_index,&_keyframe_count);

		if (offset == 0)
			return PARSER_FLV_ERR_PARSE_FAILED;

		if (_global_info.duration == 0.0)
			_global_info.duration = 0.001;
	}else{
		searchDurationFromFileEnd = true;
		_global_info.duration = SearchDurationFromLastTimestamp();
		if (_global_info.duration == 0.0)
			_global_info.duration = 0.001;

		_global_info.duration += 0.5;
		if (!_flv_io->Seek(_flv_head_offset))
			return PARSER_FLV_ERR_IO_SEEK_FAILED;
	}

	if (update_duration_from_fileend && !searchDurationFromFileEnd)
	{
		//修正后黑
		auto duration = SearchDurationFromLastTimestamp();
		if (duration > 0.1)
		{
			if (_global_info.duration == 0.001) {
				_global_info.duration = duration;
			}else{
				if (_global_info.duration - duration > 60)
					_global_info.duration = duration;
			}
		}
		
		_global_info.duration += 0.5;
		if (!_flv_io->Seek(_flv_head_offset))
			return PARSER_FLV_ERR_IO_SEEK_FAILED;
	}

	if (_global_info.audio_info.bits != 8 &&
		_global_info.audio_info.bits != 16)
		_global_info.audio_info.bits = 0;
	if (_global_info.audio_info.nch > 2)
		_global_info.audio_info.nch = 0;
	if (_global_info.audio_info.srate != 11025 &&
		_global_info.audio_info.srate != 22050 &&
		_global_info.audio_info.srate != 44100 &&
		_global_info.audio_info.srate != 48000)
		_global_info.audio_info.srate = 0;

	if (_keyframe_count == 0)
		_global_info.no_keyframe_index = 1;

	_flv_data_offset = _flv_head_offset + sizeof(FLV_BODY_TAG) + tag_size;
	if ((av_flag & FLV_HEAD_HAS_AUDIO_FLAG) == 0)
		_global_info.no_audio_stream = 1;

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::GetStreamInfo(FLV_STREAM_GLOBAL_INFO* info)
{
	if (info == nullptr)
		return PARSER_FLV_ERR_UNEXPECTED;

	if (_flv_data_offset == 0)
		return PARSER_FLV_ERR_NOT_OPENED;

	*info = _global_info;

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::Reset()
{
	if (_flv_data_offset == 0)
		return PARSER_FLV_ERR_NOT_OPENED;

	auto r = _flv_io->Seek(_flv_data_offset);
	if (!r)
		return PARSER_FLV_ERR_IO_SEEK_FAILED;

	_bodyBuffer.ResetSize();

	_pktAudioBuffer.ResetSize();
	_pktVideoBuffer.ResetSize();

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::Seek(double seconds,bool direction_next)
{
	if (_flv_data_offset == 0)
		return PARSER_FLV_ERR_NOT_OPENED;

	if (seconds <= 0.5)
		return Reset();

	if (seconds > _global_info.duration)
		return Reset();

	if (_keyframe_count == 0)
		return PARSER_FLV_ERR_NON_KEY_FRAME_INDEX;

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

			if ((next_offset < prev_offset) && (i != 0) && (direction_next))
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
		return PARSER_FLV_ERR_UNEXPECTED;

	if (!_flv_io->Seek(file_pos - 4))
		return PARSER_FLV_ERR_IO_SEEK_FAILED;

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::ReadNextPacket(FLV_STREAM_PACKET* packet)
{
	if (_flv_data_offset == 0)
		return PARSER_FLV_ERR_NOT_OPENED;
	if (packet == nullptr)
		return PARSER_FLV_ERR_UNEXPECTED;

	packet->skip_this = 0;

	unsigned tag_type = 0,tag_size = 0,tag_time = 0;
	if (!ReadFlvStreamTag(_flv_io,tag_type,tag_size,tag_time))
		return PARSER_FLV_ERR_NON_MORE_PACKET;

	if (tag_size < 2)
		return PARSER_FLV_ERR_IO_DATA_INVALID;

	if (tag_type == FLV_TAG_TYPE_AUDIO)
	{
		if (_pkt_buf_audio.data_buf)
		{
			_pktAudioBuffer.ResetSize();
			memset(&_pkt_buf_audio,0,sizeof(_pkt_buf_audio));
		}
	}else if (tag_type == FLV_TAG_TYPE_VIDEO)
	{
		if (_pkt_buf_video.data_buf)
		{
			_pktVideoBuffer.ResetSize();
			memset(&_pkt_buf_video,0,sizeof(_pkt_buf_video));
		}
	}else if (tag_type == FLV_TAG_TYPE_META)
	{
		packet->skip_this = 1;
		_flv_io->SeekByCurrent(tag_size);
		return PARSER_FLV_OK;
	}else{
		return PARSER_FLV_ERR_PARSE_FAILED;
	}

	_bodyBuffer.Alloc(tag_size);
	if (_bodyBuffer.Get() == nullptr)
		return PARSER_FLV_ERR_BAD_ALLOC;

	auto rs = _flv_io->Read(_bodyBuffer.Get(),tag_size);
	if (rs != tag_size)
		return PARSER_FLV_ERR_IO_READ_FAILED;

	unsigned char* pdata = _bodyBuffer.Get<unsigned char>();

	unsigned v_frame_type = (pdata[0] & 0xF0) >> 4;
	unsigned a_sample_rate = (pdata[0] & 0x0C) >> 2;
	unsigned a_sample_size = (pdata[0] & 0x02) >> 1;
	unsigned a_ch_type = pdata[0] & 0x01;

	static const unsigned audio_sample_rate_index[] = {8000,11025,22050,44100,48000};

	if (tag_type == FLV_TAG_TYPE_AUDIO)
	{
		if (_global_info.audio_info.nch == 0)
			_global_info.audio_info.nch = a_ch_type + 1;
		if (_global_info.audio_info.bits == 0 && a_sample_size != 0)
			_global_info.audio_info.bits = (a_sample_size + 1) * 8;
		if (_global_info.audio_info.srate == 0 && a_sample_rate != 0 && a_sample_rate < 4)
			_global_info.audio_info.srate = audio_sample_rate_index[a_sample_rate];
	}

	unsigned codec_id = 0;
	if (tag_type == FLV_TAG_TYPE_VIDEO)
		codec_id = pdata[0] & 0x0F;
	else if (tag_type == FLV_TAG_TYPE_AUDIO)
		codec_id = (pdata[0] & 0xF0) >> 4;

	if (tag_type == FLV_TAG_TYPE_AUDIO)
	{
		if (codec_id != FLV_AUDIO_CODEC_MP3 &&
			codec_id != FLV_AUDIO_CODEC_AAC &&
			codec_id != FLV_AUDIO_CODEC_PCM_LE &&
			_global_info.audio_info.raw_codec_id != FLV_AUDIO_CODEC_AAC &&
			_global_info.audio_info.raw_codec_id != FLV_AUDIO_CODEC_MP3 &&
			_global_info.audio_info.raw_codec_id != FLV_AUDIO_CODEC_PCM_LE &&
			_global_info.audio_info.raw_codec_id != -1)
			return PARSER_FLV_ERR_AUDIO_STREAM_UNSUPPORTED;
	}else if (tag_type == FLV_TAG_TYPE_VIDEO)
	{
		if (codec_id != FLV_VIDEO_CODEC_H264 && 
			_global_info.video_info.raw_codec_id != FLV_VIDEO_CODEC_H264 &&
			_global_info.video_info.raw_codec_id != -1) {
			if (_global_info.video_info.raw_codec_id == FLV_VIDEO_CODEC_VP6 ||
				_global_info.video_info.raw_codec_id == FLV_VIDEO_CODEC_VP6A)
				return PARSER_FLV_ERR_VIDEO_VP6_STREAM_UNSUPPORTED;
			else if (_global_info.video_info.raw_codec_id == FLV_VIDEO_CODEC_H263)
				return PARSER_FLV_ERR_VIDEO_H263_STREAM_UNSUPPORTED;
			else
				return PARSER_FLV_ERR_VIDEO_STREAM_UNSUPPORTED;
		}
		if (codec_id != FLV_VIDEO_CODEC_H264)
		{
			memset(packet,0,sizeof(FLV_STREAM_PACKET));
			packet->type = SupportPacketType::PacketTypeVideo;
			packet->skip_this = 1;
			return PARSER_FLV_OK;
		}
	}

	if (tag_type == FLV_TAG_TYPE_AUDIO && _global_info.audio_type == AudioStreamType_None)
	{
		if (codec_id == FLV_AUDIO_CODEC_MP3)
			_global_info.audio_type = AudioStreamType_MP3;
		else if (codec_id == FLV_AUDIO_CODEC_AAC)
			_global_info.audio_type = AudioStreamType_AAC;
		else if (codec_id == FLV_AUDIO_CODEC_PCM_LE)
			_global_info.audio_type = AudioStreamType_PCM;
	}else if (tag_type == FLV_TAG_TYPE_VIDEO && _global_info.video_type == VideoStreamType_None)
	{
		_global_info.video_type = VideoStreamType_AVC;
	}

	if (tag_type == FLV_TAG_TYPE_VIDEO &&
		_global_info.delay_flush_spec_info.avc_info_size == 0) {
		if (codec_id == FLV_VIDEO_CODEC_H264 && pdata[1] == 0) //AVCDecoderConfigurationRecord.
		{
			if (!ProcessAVCDecoderConfigurationRecord(&pdata[5])) //convert to AnnexB format.
			{
				return PARSER_FLV_ERR_PARSE_FAILED;
			}else{
				memset(packet,0,sizeof(FLV_STREAM_PACKET));
				packet->type = SupportPacketType::PacketTypeVideo;
				packet->skip_this = 1;

				if (_global_info.video_info.width == 0 ||
					_global_info.video_info.height == 0)
					UpdateGlobalInfoH264();

				if (_output_avc1)
				{
					if (_global_info.delay_flush_spec_info.avcc == nullptr)
					{
						_global_info.delay_flush_spec_info.avcc = (unsigned char*)
							malloc(FLV_ALLOC_ALIGNED(tag_size));
						_global_info.delay_flush_spec_info.avcc_size = tag_size - 5;
						memcpy(_global_info.delay_flush_spec_info.avcc,&pdata[5],
							_global_info.delay_flush_spec_info.avcc_size);
					}
				}
			}
		}
	}

	if (tag_type == FLV_TAG_TYPE_AUDIO &&
		_global_info.delay_flush_spec_info.aac_spec_info == nullptr) {
		if (codec_id == FLV_AUDIO_CODEC_AAC && pdata[1] == 0) //AudioSpecificConfig
		{
			if (!ProcessAACAudioSpecificConfig(&pdata[2])) // copy AudioSpecificConfig Header.
			{
				return PARSER_FLV_ERR_PARSE_FAILED;
			}else{
				memset(packet,0,sizeof(FLV_STREAM_PACKET));
				packet->type = SupportPacketType::PacketTypeAudio;
				packet->skip_this = 1;
			}
		}
	}
	if (tag_size > 2 && pdata[1] == 0)
		packet->skip_this = 1;

	if (packet->skip_this != 1)
	{
		int len = 0;
		if (tag_type == FLV_TAG_TYPE_VIDEO)
		{
			len = tag_size - FLV_H264_VIDEO_TAG_HEAD_LEN;
		}else if (tag_type == FLV_TAG_TYPE_AUDIO)
		{
			if (codec_id == FLV_AUDIO_CODEC_AAC)
				len = tag_size - FLV_AAC_AUDIO_TAG_HEAD_LEN;
			else if (codec_id == FLV_AUDIO_CODEC_MP3)
				len = tag_size - FLV_MP3_AUDIO_TAG_HEAD_LEN;
			else
				len = tag_size - 1;
		}

		if (len > 0)
		{
			if (tag_type == FLV_TAG_TYPE_AUDIO)
			{
				_pkt_buf_audio.timestamp = tag_time;

				_pkt_buf_audio.type = SupportPacketType::PacketTypeAudio;
				_pkt_buf_audio.data_size = len;
				_pkt_buf_audio.key_frame = 1;

				_pktAudioBuffer.Alloc(len);
				_pkt_buf_audio.data_buf = _pktAudioBuffer.Get<unsigned char>();
				if (_pkt_buf_audio.data_buf == nullptr)
					return PARSER_FLV_ERR_BAD_ALLOC;

				memcpy(_pkt_buf_audio.data_buf,pdata + (tag_size - len),len);

				*packet = _pkt_buf_audio;
			}else{
				_pkt_buf_video.timestamp = _pkt_buf_video.dts = tag_time;

				_pkt_buf_video.type = SupportPacketType::PacketTypeVideo;
				_pkt_buf_video.key_frame = (v_frame_type == FLV_FRAME_TYPE_KEY ? 1:0);

				unsigned cts = 0;
				unsigned char* p = (unsigned char*)&cts;
				p[0] = pdata[4];
				p[1] = pdata[3];
				p[2] = pdata[2];

				_pkt_buf_video.timestamp += cts;

				_pktVideoBuffer.Alloc(len);
				_pkt_buf_video.data_buf = _pktVideoBuffer.Get<unsigned char>();
				if (_pkt_buf_video.data_buf == nullptr)
					return PARSER_FLV_ERR_BAD_ALLOC;

				if (!_output_avc1)
				{
					_pkt_buf_video.data_size = ScanAllFrameNALU(pdata + FLV_H264_VIDEO_TAG_HEAD_LEN,_pkt_buf_video.data_buf,len);
					if (_pkt_buf_video.data_size == 0)
						_pkt_buf_video.skip_this = 1;
				}else{
					_pkt_buf_video.data_size = len;
					memcpy(_pkt_buf_video.data_buf,pdata + FLV_H264_VIDEO_TAG_HEAD_LEN,len);
				}

				*packet = _pkt_buf_video;
			}
		}
	}

	if (packet->type == SupportPacketType::PacketTypeVideo &&
		packet->data_size == 0 && packet->skip_this == 0)
		return PARSER_FLV_ERR_NON_MORE_PACKET;

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::ReadNextPacketFast(FLV_STREAM_PACKET* packet)
{
	if (_flv_data_offset == 0)
		return PARSER_FLV_ERR_NOT_OPENED;
	if (packet == nullptr)
		return PARSER_FLV_ERR_UNEXPECTED;

	packet->skip_this = 1;

	unsigned tag_type = 0,tag_size = 0,tag_time = 0;
	if (!ReadFlvStreamTag(_flv_io,tag_type,tag_size,tag_time))
		return PARSER_FLV_ERR_NON_MORE_PACKET;

	if (tag_size == 0)
		return PARSER_FLV_ERR_IO_DATA_INVALID;

	packet->data_buf = nullptr;
	packet->data_size = 0;
	
	packet->type = tag_type == FLV_TAG_TYPE_AUDIO ? SupportPacketType::PacketTypeAudio:
		SupportPacketType::PacketTypeVideo;

	unsigned offset = 0;
	unsigned char buffer[10] = {};

	if (tag_type == FLV_TAG_TYPE_VIDEO &&
		tag_size < FLV_H264_VIDEO_TAG_HEAD_LEN)
		return PARSER_FLV_ERR_IO_DATA_INVALID;

	if (tag_type == FLV_TAG_TYPE_VIDEO)
	{
		offset = FLV_H264_VIDEO_TAG_HEAD_LEN;
		if (!_flv_io->Read(&buffer,offset))
			return PARSER_FLV_ERR_IO_READ_FAILED;
	}

	if (tag_size - offset > 0)
	{
		if (!_flv_io->SeekByCurrent(tag_size - offset))
			return PARSER_FLV_ERR_IO_SEEK_FAILED;
	}

	packet->timestamp = tag_time;
	packet->data_size = tag_size;
	if (tag_type == FLV_TAG_TYPE_VIDEO) //h264
	{
		unsigned frame_type = (buffer[0] & 0xF0) >> 4;
		packet->key_frame = (frame_type == FLV_FRAME_TYPE_KEY ? 1:0);

		if (buffer[1] == 1)
		{
			//cts..
			unsigned cts = 0;
			unsigned char* p = (unsigned char*)&cts;
			p[0] = buffer[4];
			p[1] = buffer[3];
			p[2] = buffer[2];

			packet->timestamp += cts;
		}
	}

	return PARSER_FLV_OK;
}

unsigned FLVStreamParser::ScanAllFrameNALU(unsigned char* psrc,unsigned char* pdst,unsigned len)
{
	if (len < _lengthSizeMinusOne)
		return 0;

	decltype(psrc) ps = psrc;
	decltype(pdst) pd = pdst;

	unsigned size = 0,size2 = 0;
	unsigned count = 0;
	while (1)
	{
		unsigned nal_size = GetFrameNALUSize(ps,_lengthSizeMinusOne);
		if (nal_size == 0)
		{
			ps += _lengthSizeMinusOne;
			continue;
		}
		
		if (nal_size > len)
		{
			printf("Fuck Bilibili.\n");
			return 0;
		}

		pd[0] = 0;
		pd[1] = 0;
		pd[2] = 0;
		pd[3] = 1;

		memcpy(&pd[4],ps + _lengthSizeMinusOne,nal_size);

		size = size + _lengthSizeMinusOne + nal_size;
		size2 = size2 + 4 + nal_size;

		pd = pdst + size2;
		ps = psrc + size;

		if (size >= len)
			break;
	}

	return size2;
}