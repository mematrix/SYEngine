#include "MP4MediaFormat.h"

#define _IO_BUFFER_SIZE_NETWORK 65536 //64KB.
#define _IO_BUFFER_SIZE_LOCAL (65536 * 4) //256KB.

unsigned ScanAllFrameNALUToAnnexB(unsigned char* source,unsigned len);
unsigned ScanAllFrameNALU(unsigned char* psrc,unsigned char* pdst,unsigned len,unsigned lengthSizeMinusOne);

AV_MEDIA_ERR MP4MediaFormat::Open(IAVMediaIO* io)
{
	if (io == nullptr)
		return AV_COMMON_ERR_INVALIDARG;
	if (_av_io != nullptr)
		return AV_COMMON_ERR_CHANGED_STATE;

	if (!RESET_AV_MEDIA_IO(io))
		return AV_IO_ERR_SEEK_FAILED;

	unsigned io_buf_size = _IO_BUFFER_SIZE_LOCAL;
	if (io->IsAliveStream())
		io_buf_size = _IO_BUFFER_SIZE_NETWORK;

	auto io_pool = std::make_shared<IOPoolReader>(io,io_buf_size);
	_av_io = io_pool.get();

	unsigned isom_err_code = 0;
	auto core = std::make_shared<Aka4Splitter>(this);
	bool result = core->Open(&isom_err_code,true);
	_av_io = nullptr;
	if (!result)
		return AV_OPEN_ERR_PROBE_FAILED;
	if (core->GetTracks()->GetCount() == 0)
		return AV_OPEN_ERR_HEADER_INVALID;

	_av_io = io_pool.get();
	_stream_count = 0;
	if (!MakeAllStreams(core) || _stream_count == 0)
	{
		_av_io = nullptr;
		return AV_OPEN_ERR_STREAM_INVALID;
	}

	if (core->GetTracks()->GetCount() == 1)
		if (core->GetTracks()->Get(0)->Type == Aka4Splitter::TrackInfo::TrackType_Audio)
			_is_m4a_file = true;

	_io_pool = std::move(io_pool);
	_core = std::move(core);

	LoadAllChapters();
	return AV_ERR_OK;
}

AV_MEDIA_ERR MP4MediaFormat::Close()
{
	if (_av_io == nullptr)
		AV_CLOSE_ERR_NOT_OPENED;

	if (_core)
		_core->Close();
	_core.reset();

	if (_io_pool)
		_io_pool.reset();

	for (unsigned i = 0;i < _stream_count;i++)
		_streams[i].reset();
	_stream_count = 0;

	_chaps.ClearItems();
	_pkt_buffer.Free();
	_av_io = nullptr;
	return AV_ERR_OK;
}

AV_MEDIA_ERR MP4MediaFormat::Seek(double seconds,bool key_frame,AVSeekDirection seek_direction)
{
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;

	if (seconds < 0.1)
	{
		if (!_core->Seek(0.0,Aka4Splitter::SeekMode::SeekMode_Reset,false))
			return AV_SEEK_ERR_PARSER_FAILED;
		return AV_ERR_OK;
	}

	if (!_core->Seek(seconds,
		seek_direction == AVSeekDirection::SeekDirection_Auto ? Aka4Splitter::SeekMode::SeekMode_Prev:
		seek_direction == AVSeekDirection::SeekDirection_Back ? Aka4Splitter::SeekMode::SeekMode_Prev:
		Aka4Splitter::SeekMode::SeekMode_Next,key_frame))
		return AV_SEEK_ERR_PARSER_FAILED;

	return AV_ERR_OK;
}

AV_MEDIA_ERR MP4MediaFormat::ReadPacket(AVMediaPacket* packet)
{
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;
	if (packet == nullptr)
		return AV_COMMON_ERR_INVALIDARG;

	Aka4Splitter::DataPacket pkt;
	MP4MediaStream* stm = nullptr;
	while (1)
	{
		pkt = {};
		if (!_core->ReadPacket(&pkt))
			return AV_READ_PACKET_ERR_NON_MORE;

		unsigned index = FindStreamIndexFromId(pkt.Id);
		if (index != 0xFF)
		{
			stm = _streams[index].get();
			break;
		}
	}
	if (stm == nullptr)
		return AV_COMMON_ERR_UNEXPECTED;

	bool no_fixed = false;
	auto ct = stm->GetCodecType();
	if ((ct == MediaCodecType::MEDIA_CODEC_VIDEO_H264 ||
		ct == MediaCodecType::MEDIA_CODEC_VIDEO_HEVC) && !_force_avc1) {
		unsigned nal_size = stm->GetNaluSize();
		if (nal_size == 0)
			return AV_COMMON_ERR_UNEXPECTED;

		if (nal_size == 4)
		{
			//如果NAL头是4字节，不需要Copy，直接修改为AnnexB格式
			packet->data.buf = pkt.Data;
			packet->data.size = ScanAllFrameNALUToAnnexB(pkt.Data,pkt.DataSize);
		}else{
			if (!_pkt_buffer.Alloc(pkt.DataSize << 1,true))
				return AV_COMMON_ERR_OUTOFMEMORY;

			packet->data.buf = _pkt_buffer.Get<unsigned char>();
			packet->data.size = ScanAllFrameNALU(pkt.Data,packet->data.buf,pkt.DataSize,nal_size);
		}
	}else{
		//if (AllocMediaPacketAndCopy(packet,pkt.Data,pkt.DataSize) == 0)
			//return AV_READ_PACKET_ERR_ALLOC_PKT;
		packet->data.buf = pkt.Data;
		packet->data.size = pkt.DataSize;

		if (stm->IsAppleMovText())
		{
			if (pkt.DataSize <= 2)
			{
				packet->data.size = 0;
				packet->data.buf = nullptr;
				packet->flag = MEDIA_PACKET_CAN_TO_SKIP_FLAG;
			}else{
				uint16_t len = *(uint16_t*)pkt.Data;
				len = ISOM_SWAP16(len);
				if (len > pkt.DataSize - 2)
					len = pkt.DataSize - 2;
				if (len > 0)
					if (AllocMediaPacketAndCopy(packet,pkt.Data + 2,len) == 0)
						return AV_READ_PACKET_ERR_ALLOC_PKT;
				if (len > 0)
					no_fixed = true;
			}
		}
	}

	if (!no_fixed)
		packet->flag |= MEDIA_PACKET_FIXED_BUFFER_FLAG;
	packet->duration = PACKET_NO_PTS;
	packet->stream_index = stm->GetStreamIndex();
	packet->position = pkt.InFilePosition;
	packet->dts = pkt.DTS;
	packet->pts = pkt.PTS;
	if (pkt.Duration > 0)
		packet->duration = pkt.Duration;
	if (pkt.KeyFrame)
		packet->flag |= MEDIA_PACKET_KEY_FRAME_FLAG;

	if (packet->data.size == 0)
		packet->flag |= MEDIA_PACKET_BUFFER_NONE_FLAG;
	return AV_ERR_OK;
}

void* MP4MediaFormat::StaticCastToInterface(unsigned id)
{
	if (id == AV_MEDIA_INTERFACE_ID_CAST_CHAPTER)
		return static_cast<IAVMediaChapters*>(this);
	else if (id == AV_MEDIA_INTERFACE_ID_CAST_METADATA)
		return static_cast<IAVMediaMetadata*>(this);
	else if (id == AV_MEDIA_INTERFACE_ID_CASE_EX)
		return static_cast<IAVMediaFormatEx*>(this);
	return nullptr;
}

bool MP4MediaFormat::MakeAllStreams(std::shared_ptr<Aka4Splitter>& core)
{
	auto p = core->GetTracks();
	if (p->GetCount() > MAX_MP4_STREAM_COUNT)
		return false;

	for (unsigned i = 0;i < p->GetCount();i++)
	{
		auto t = p->Get(i);
		if (t->Type != Aka4Splitter::TrackInfo::TrackType_Audio &&
			t->Type != Aka4Splitter::TrackInfo::TrackType_Video &&
			t->Type != Aka4Splitter::TrackInfo::TrackType_Subtitle)
			continue;

		auto s = std::make_shared<MP4MediaStream>(t,_stream_count);
		switch (t->Type)
		{
		case Aka4Splitter::TrackInfo::TrackType_Audio:
			if (!s->ProbeAudio(core))
				continue;
			break;
		case Aka4Splitter::TrackInfo::TrackType_Video:
			if (!s->ProbeVideo(core,_force_avc1))
				continue;
			break;
		case Aka4Splitter::TrackInfo::TrackType_Subtitle:
			if (!s->ProbeText(core,
					t->Codec.CodecId.CodecFcc != ISOM_FCC('tx3g') &&
					t->Codec.CodecId.CodecFcc != ISOM_FCC('text')))
				continue;
			break;
		default:
			continue;
		}

		//bitrate
		if (t->BitratePerSec > 128)
		{
			if (s->GetMainType() == MEDIA_MAIN_TYPE_AUDIO)
			{
				AudioBasicDescription audio = {};
				if (s->GetAudioInfo()->GetAudioDescription(&audio))
				{
					if (audio.bitrate == 0)
						UpdateAudioDescriptionBitrate(s->GetAudioInfo(),t->BitratePerSec);
				}
			}else if (s->GetMainType() == MEDIA_MAIN_TYPE_VIDEO)
			{
				VideoBasicDescription video = {};
				if (s->GetVideoInfo()->GetVideoDescription(&video))
				{
					if (video.bitrate == 0)
					{
						video.bitrate = t->BitratePerSec;
						s->GetVideoInfo()->ExternalUpdateVideoDescription(&video);
					}
				}
			}
		}

		_streams[_stream_count] = std::move(s);
		_stream_count++;
	}
	return true;
}

unsigned MP4MediaFormat::FindStreamIndexFromId(unsigned id)
{
	for (unsigned i = 0;i < _stream_count;i++)
	{
		if (_streams[i]->GetTrackInfo()->Id == id)
			return i;
	}
	return 0xFF;
}

int MP4MediaFormat::GetKeyFramesCount()
{
	if (_core == nullptr)
		return -1;

	if (_kf_count == 0)
	{
		if (_core->GetTracks()->GetCount() == 0)
			return false;
		if (_is_m4a_file)
			return false;

		auto t = _core->GetTracks()->Get(_core->InternalGetBestSeekTrackIndex());
		auto isom = t->InternalTrack;
		if (isom->SampleCount == 0 || isom->Samples == nullptr)
			return false;
		for (unsigned i = 0;i < isom->SampleCount;i++)
		{
			auto p = &(isom->Samples[i]);
			if (!p->keyframe || p->IsInvalidTimestamp() || p->stsd_index > 1)
				continue;
			_kf_count++;
		}
	}
	return _kf_count;
}

unsigned MP4MediaFormat::CopyKeyFrames(double* copyTo,int copyCount)
{
	if (_kf_count == 0)
		return 0;

	unsigned count = copyCount == -1 ? _kf_count:(unsigned)copyCount;
	if (count > _kf_count || count == 0)
		return 0;

	auto time = copyTo;
	auto isom = _core->GetTracks()->Get(_core->InternalGetBestSeekTrackIndex())->InternalTrack;
	unsigned result = 0;
	for (unsigned i = 0;i < isom->SampleCount;i++)
	{
		auto p = &(isom->Samples[i]);
			if (!p->keyframe || p->IsInvalidTimestamp() || p->stsd_index > 1)
				continue;

		*time = p->GetPTS();
		time++;
		result++;

		if (result == count)
			break;
	}
	return result;
}

void MP4MediaFormat::LoadAllChapters()
{
	auto list = _core->InternalGetCore()->GetChapters();
	if (list == nullptr)
		return;
	if (list->GetCount() == 0)
		return;

	for (unsigned i = 0;i < list->GetCount();i++)
	{
		auto item = list->Get(i);
		if (item->Title == nullptr)
			continue;
		if (strlen(item->Title) > 255)
			continue;

		AVMediaChapter chap = {};
		chap.type = AVChapterType::ChapterType_Marker;
		chap.str_enc_type = AVChapterEncodingType::ChapterStringEnc_UTF8;
		chap.index = (double)i;
		chap.end_time = chap.duration = PACKET_NO_PTS;
		chap.start_time = item->Timestamp;
		strcpy(chap.title,item->Title);

		_chaps.AddItem(&chap);
	}
}