/* ---------------------------------------------------------------
 -
 - The Aka4Splitter : Simple library to parses MP4 files.
 - Aka4Splitter.cpp

 - MIT License
 - Copyright (C) 2015 ShanYe
 - Rev: 1.0.0
 -
 --------------------------------------------------------------- */

#include "Aka4Splitter.h"

bool MovLangId2Iso639(unsigned code, char to[4]);
unsigned ReadDolbyAudioChannels(unsigned char* pb, unsigned size, bool ac3_ec3);

bool Aka4Splitter::Open(unsigned* isom_err_code, bool alloc_max_pkt_buf)
{
	memset(&_info, 0, sizeof(_info));
	_process_tracks_start_time_offset = nullptr;
	_track_num_start_with_zero = false;
	_sample_prev_filepos = 0;

	if (_stm == nullptr)
		return false;
	if (uint64_t(_stm->GetSize()) > INT64_MAX)
		return false;
	if (_parser.get() != nullptr)
		return false;

	_stm->Seek(0);
	auto parser = std::make_shared<Isom::Parser>(_stm);
	if (parser == nullptr)
		return false;

	unsigned err = parser->Parse();
	if (isom_err_code != nullptr)
		*isom_err_code = err;

	if (err != ISOM_ERR_OK)
		return false;
	if (!InitGlobalInfo(parser))
		return false;
	if (!InitTracks(parser))
		return false;

	if (_info.Duration < 0.1)
	{
		unsigned index = 0;
		auto dur = GetLongestTrackDuration(&index);
		if (dur > 0)
		{
			auto p = _tracks[index];
			if (p->InternalTrack->MediaInfo.TimeScale > 0)
				_info.Duration = double(dur) / double(p->InternalTrack->MediaInfo.TimeScale);
		}
	}

	_best_seek_track_index = FindBestSeekTrackIndex();
	_parser = std::move(parser);

	if (alloc_max_pkt_buf)
		if (!PreallocMaxPacketBuffer())
			return false;

	return true;
}

void Aka4Splitter::Close()
{
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
		_tracks.Get(i)->InternalFree();
	_tracks.Clear();

	if (_parser)
		_parser->Free();
	_parser.reset();

	if (_process_tracks_start_time_offset)
		free(_process_tracks_start_time_offset);
	_process_tracks_start_time_offset = nullptr;

	if (_pkt_buffer)
		free(_pkt_buffer);
	_pkt_buffer = nullptr;
}

bool Aka4Splitter::Seek(double seconds, SeekMode mode, bool keyframe)
{
	if (mode == SeekMode_Reset)
	{
		for (unsigned i = 0; i < _tracks.GetCount(); i++)
		{
			_tracks[i]->InternalCurrentSample = 0;
			_tracks[i]->InternalStartTimeOffset = _tracks[i]->InternalTrack->TimeStartOffset;
		}
		return true;
	}
	return SeekAllStreams(seconds, mode == SeekMode_Prev, keyframe);
}

bool Aka4Splitter::ReadPacket(DataPacket* pkt)
{
	if (pkt == nullptr)
		return false;

	unsigned track_index = 0, sample_index = 0;
	if (!FindNextSample(&track_index, &sample_index))
		return false;

	auto t = _tracks[track_index];
	auto core = t->InternalTrack;
	auto sample = &core->Samples[sample_index];
	t->InternalCurrentSample++;

	if (!_stm->Seek(sample->pos))
		return false;

	if (_extern_pkt_buf == nullptr)
	{
		if (!AllocPacketBuffer((unsigned)sample->size))
			return false;
		pkt->Data = _pkt_buffer;
	}else{
		pkt->Data = _extern_pkt_buf;
	}

	pkt->DataSize = _stm->Read(pkt->Data, sample->size);
	pkt->Id = track_index;
	pkt->InFilePosition = sample->pos;
	pkt->KeyFrame = sample->keyframe;
	if (!sample->IsInvalidTimestamp())
	{
		pkt->DTS = sample->GetDTS();
		pkt->PTS = sample->GetPTS();
		if (t->InternalCurrentSample < core->SampleCount && !t->Audio.QuickTimeFlag)
			pkt->Duration = (&core->Samples[t->InternalCurrentSample])->timestamp - pkt->DTS;
		if (_process_tracks_start_time_offset[track_index])
			pkt->PTS -= t->InternalStartTimeOffset;
	}else{
		pkt->PTS = pkt->DTS = Aka4_DataPacket_InvalidTimestamp;
	}
	if (pkt->DataSize == 0)
		return false;

	/*
#ifdef _DEBUG
	if (sample->pos < _sample_prev_filepos)
		printf("GoBack Sample: Index - %d, Size - %d, BackOffset - %d\n",
			pkt->Id,pkt->DataSize,unsigned(_sample_prev_filepos - sample->pos));
	_sample_prev_filepos = sample->pos;
#endif
	*/
	return true;
}

bool Aka4Splitter::FindNextSample(unsigned* track_index, unsigned* next_index)
{
	Isom::Object::Internal::IndexObject* sample = nullptr;
	double best_dts = double(INT64_MAX);
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		auto t = _tracks[i];
		auto core = t->InternalTrack;
		if (t->InternalCurrentSample >= core->SampleCount)
			continue;

		auto cur_sample = &core->Samples[t->InternalCurrentSample];
		if (core->MediaInfo.TimeScale <= 0 || cur_sample->stsd_index > 1)
		{
			t->InternalCurrentSample++;
			continue;
		}
		double dts = cur_sample->timestamp;

		if (sample == nullptr)
		{
			best_dts = dts;
			sample = cur_sample;
			*track_index = i;
			*next_index = t->InternalCurrentSample;
			continue;
		}
		if (!_stm->IsSeekable() && cur_sample->pos < sample->pos)
			continue;
		
		//优先级是pos小的优先，后是dts小的优先。
		//pos小的时候，上一个best_dts - 现在的dts，偏差必须在1秒内。
		//dts小的时候，上一个best_dts - 现在的dts，偏差必须大于1秒。
		if ((fabs(best_dts - dts) <= 1.0 && cur_sample->pos < sample->pos) ||
			(fabs(best_dts - dts) > 1.0 && dts < best_dts)) {
			best_dts = dts;
			sample = cur_sample;
			*track_index = i;
			*next_index = t->InternalCurrentSample;
		}
	}
	return sample != nullptr;
}

bool Aka4Splitter::SeekAllStreams(double seconds, bool prev_or_next, bool key_frame)
{
	if (!SeekSingleTrack(_best_seek_track_index, seconds, prev_or_next, key_frame))
		return false;

	auto t = _tracks.Get(_best_seek_track_index);
	if (t->InternalTrack->Samples == nullptr &&
		t->InternalCurrentSample >= t->InternalTrack->SampleCount)
		return false;

	if (t->InternalCurrentSample == 0)
		return Reset();

	double sec = t->InternalTrack->Samples[t->InternalCurrentSample].GetPTS();
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		if (i == _best_seek_track_index)
			continue;
		SeekSingleTrack(i, sec, prev_or_next, key_frame);
	}
	return true;
}

bool Aka4Splitter::SeekSingleTrack(unsigned index, double seconds, bool prev_or_next, bool key_frame)
{
	auto t = _tracks[index];
	double duration;
	if (t->InternalTimeScale > 0)
		duration = double(t->InternalTrack->TotalDuration) / t->InternalTimeScale;
	else
		duration = t->InternalTrack->Samples[t->InternalTrack->SampleCount - 1].GetPTS();

	bool found = false;
	unsigned found_index = 0, prev_index = 0;
	for (unsigned i = 0; i < t->InternalTrack->SampleCount; i++)
	{
		auto p = &t->InternalTrack->Samples[i];
		if (key_frame)
			if (!p->keyframe && t->InternalTrack->KeyFrameExists)
				continue;
		if (p->IsInvalidTimestamp())
			continue;
		if (p->GetPTS() >= seconds)
		{
			found_index = i;
			found = true;
			break;
		}
		prev_index = i;
	}
	if (!found)
	{
		//即便没有找到frame（比如seek比duration大的情况）。
		//也保障提交最后一个关键帧。
		if (prev_index == 0)
			return false;
		found_index = prev_index;
	}

	if (prev_or_next && found_index > 0 && t->InternalTrack->KeyFrameExists)
		found_index = prev_index;
	t->InternalCurrentSample = found_index;
	return true;
}

bool Aka4Splitter::AllocPacketBuffer(unsigned size)
{
	if (size <= _pkt_buf_size)
		return true;
	if (_pkt_buffer)
		free(_pkt_buffer);
	_pkt_buffer = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(size) + 64); //64 for bitreader edge aligned.
	if (_pkt_buffer)
		_pkt_buf_size = size;
	return _pkt_buffer != nullptr;
}

bool Aka4Splitter::PreallocMaxPacketBuffer()
{
	unsigned buf_size = InternalGetMaxPacketBufferSize();
	if (buf_size == 0)
		return false;
	if (buf_size >= INT32_MAX)
		return false;
	return AllocPacketBuffer(buf_size);
}

bool Aka4Splitter::InitGlobalInfo(const std::shared_ptr<Isom::Parser>& parser) throw()
{
	_info.Bands = parser->GetHeader()->File.CompBands;
	if (parser->GetHeader()->Movie.TimeScale > 0)
		_info.Duration = 
			double(parser->GetHeader()->Movie.Duration) /
			double(parser->GetHeader()->Movie.TimeScale);
	return true;
}

bool Aka4Splitter::InitTracks(const std::shared_ptr<Isom::Parser>& parser) throw()
{
	auto p = parser->GetTracks();
	if (p->GetCount() == 0)
		return false;

	_process_tracks_start_time_offset = (bool*)malloc(p->GetCount() * 4);
	if (_process_tracks_start_time_offset == nullptr)
		return false;
	memset(_process_tracks_start_time_offset, 0, p->GetCount() * 4);

	for (unsigned i = 0; i < p->GetCount(); i++)
	{
		auto t = p->Get(i);
		if (t->Header.Id == 0)
			_track_num_start_with_zero = true;

		TrackInfo ti = {};
		if (!ConvertIsomTrackToNative(t, &ti))
			continue;

		//check start time offset.
		_process_tracks_start_time_offset[_tracks.GetCount()] = 
			CheckApplyStartTimeOffset(&ti);

		ti.Id = _tracks.GetCount();
		if (!_tracks.Add(&ti))
			return false;
	}
	return true;
}

bool Aka4Splitter::CheckApplyStartTimeOffset(TrackInfo* ti) throw()
{
	if (ti->Type != Aka4Splitter::TrackInfo::TrackType::TrackType_Video)
		return false;
	if (ti->InternalStartTimeOffset <= 0.0)
		return false;
	auto s = &ti->InternalTrack->Samples[0];
	if (s->IsInvalidTimestamp())
		return false;
	auto pts = s->GetPTS();
	pts -= ti->InternalStartTimeOffset;
	if (pts == 0.0)
		return true;
	if (pts < 0.0)
		return false;
	if (pts > s->GetPTS())
		return false;
	return true;
}

bool Aka4Splitter::ConvertIsomTrackToNative(Isom::Parser::Track* isom, TrackInfo* ti) throw()
{
	if (isom->SampleCount == 0 || isom->Samples == nullptr)
		return false;

	ti->Type = (decltype(ti->Type))isom->Type;
	ti->Id = ConvertTrackId((unsigned)isom->Header.Id);
	if ((isom->Header.Flags & Isom::Object::TrackHeader::Enabled) != 0)
		ti->Enabled = true;

	char language[4] = {};
	MovLangId2Iso639(isom->MediaInfo.LangId, language);
	memcpy(ti->LangId, language, 4);
	if (strlen(ti->LangId) == 0)
		strcpy(ti->LangId, "und");

	if (isom->HandlerName)
		ti->InternalChangeName(isom->HandlerName);

	ti->InternalTrack = isom;
	ti->InternalTimeScale = double(isom->MediaInfo.TimeScale);
	ti->InternalStartTimeOffset = isom->TimeStartOffset;
	auto ci = isom->CodecInfo->GetCodecInfo();
	if (ci)
	{
		//Codec...
		ti->Codec.StoreType = TrackInfo::CodecInfo::StoreType_Default;
		ti->Codec.CodecId.CodecFcc = isom->CodecTag;
		if (ci->CodecId == ISOM_FCC('esds'))
			ti->Codec.StoreType = TrackInfo::CodecInfo::StoreType_Esds,
			ti->Codec.CodecId.EsdsObjType = ci->esds.ObjectTypeIndication;

		if (ci->CodecId == ISOM_FCC('esds'))
			ti->Codec.InternalCopyUserdata(ci->esds.DecoderConfig, ci->esds.DecoderConfigSize);
		else
			ti->Codec.InternalCopyUserdata(ci->CodecPrivate, ci->CodecPrivateSize);
		if (ti->Codec.UserdataSize == 0)
			ti->Codec.StoreType = TrackInfo::CodecInfo::StoreType_SampleEntry,
			ti->Codec.InternalCopyUserdata(isom->CodecSampleEntry, isom->CodecSampleEntrySize);

		if (ti->Codec.StoreType == TrackInfo::CodecInfo::StoreType_SampleEntry &&
			ci->CodecId == ISOM_FCC('esds'))
			ti->Codec.StoreType = TrackInfo::CodecInfo::StoreType_SampleEntryWithEsds;

		//Audio...
		ti->Audio.Channels = ci->Audio.ChannelCount;
		ti->Audio.SampleRate = ci->Audio.SampleRate;
		ti->Audio.BitDepth = ci->Audio.SampleSize;
		//Process AC-3 Channels...
		if (ci->CodecId == ISOM_FCC('dac3') || ci->CodecId == ISOM_FCC('dec3'))
		{
			unsigned nch = 0;
			if (ci->CodecPrivateSize > 2 && ci->CodecPrivate)
			{
				nch = ReadDolbyAudioChannels(ci->CodecPrivate, ci->CodecPrivateSize, ci->CodecId != ISOM_FCC('dec3'));
				if (nch > 0)
					ti->Audio.Channels = nch;
			}
		}
		if (ci->Audio.QT_Version > 0)
			ti->Audio.QuickTimeFlag = true;

		//Video...
		ti->Video.Width = ci->Video.Width;
		ti->Video.Height = ci->Video.Height;
		ti->Video.DisplayWidth = isom->Header.Width;
		ti->Video.DisplayHeight = isom->Header.Height;
		ti->Video.Rotation = isom->VideoRotation;
		//fps
		if (ti->InternalTrack->TotalFrames > 0 && ti->InternalTrack->TotalDuration > 0 && ti->InternalTimeScale > 0.1)
			ti->Video.FrameRate = 1.0f / float((double)ti->InternalTrack->TotalDuration /
				(double)ti->InternalTrack->TotalFrames / ti->InternalTimeScale);
	}

	//birate...
	double total_time = 0;
	if (ti->InternalTimeScale > 0)
		total_time = double(isom->TotalDuration) / ti->InternalTimeScale;
	if (total_time == 0)
		total_time = _info.Duration;

	if (total_time > 1.0 && isom->TotalStreamSize > 128)
		ti->BitratePerSec = unsigned(double(isom->TotalStreamSize) / total_time * 8.0);

	return true;
}

int64_t Aka4Splitter::GetLongestTrackDuration(unsigned* index) throw()
{
	int64_t result = 0;
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		if (_tracks[i]->InternalTrack->TotalDuration > result)
		{
			*index = i;
			result = _tracks[i]->InternalTrack->TotalDuration;
		}
	}
	return result;
}

unsigned Aka4Splitter::FindBestSeekTrackIndex() throw()
{
	bool found = false;
	unsigned index = 0;
	//pass1
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		auto p = _tracks[i]->InternalTrack;
		if (p->KeyFrameExists && p->Type == Isom::Parser::Track::TrackType_Video)
		{
			found = true;
			index = i;
			break;
		}
	}
	//pass2
	if (!found)
	{
		for (unsigned i = 0; i < _tracks.GetCount(); i++)
		{
			auto p = _tracks[i]->InternalTrack;
			if (p->KeyFrameExists)
			{
				found = true;
				index = i;
				break;
			}
		}
	}
	//pass3
	if (!found)
	{
		for (unsigned i = 0; i < _tracks.GetCount(); i++)
		{
			auto p = _tracks[i]->InternalTrack;
			if (p->Type == Isom::Parser::Track::TrackType_Video)
			{
				found = true;
				index = i;
				break;
			}
		}
	}
	return index;
}

unsigned Aka4Splitter::InternalGetMaxPacketBufferSize() throw()
{
	unsigned buf_size = 0;
	for (unsigned i = 0; i < _parser->GetTracks()->GetCount(); i++)
	{
		auto track = _parser->GetTracks()->Get(i);
		for (unsigned j = 0; j < track->SampleCount; j++)
		{
			auto sample = &track->Samples[j];
			if (sample->size > (int)buf_size)
				buf_size = (unsigned)sample->size;
		}
	}
	return buf_size;
}

int Aka4Splitter::QueryDefaultAudioTrackIndex(Aka4Splitter* splitter) throw()
{
	int result = -1;
	if (splitter->GetTracks()->GetCount() == 1)
		if (splitter->GetTracks()->Get()->Type != Aka4Splitter::TrackInfo::TrackType::TrackType_Audio)
			return result;

	for (unsigned i = 0; i < splitter->GetTracks()->GetCount(); i++)
	{
		auto p = splitter->GetTracks()->Get(i);
		if (p->Type != Aka4Splitter::TrackInfo::TrackType::TrackType_Audio)
			continue;
		if (result == -1)
			result = (int)i;
		if (p->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::CodecStoreType::StoreType_Esds &&
			p->Codec.CodecId.EsdsObjType == Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC &&
			p->Codec.UserdataSize > 1) {
			result = (int)i;
			break;
		}
	}

	return result;
}

int Aka4Splitter::QueryDefaultVideoTrackIndex(Aka4Splitter* splitter) throw()
{
	int result = -1;
	if (splitter->GetTracks()->GetCount() == 1)
		if (splitter->GetTracks()->Get()->Type != Aka4Splitter::TrackInfo::TrackType::TrackType_Video)
			return result;

	for (unsigned i = 0; i < splitter->GetTracks()->GetCount(); i++)
	{
		auto p = splitter->GetTracks()->Get(i);
		if (p->Type != Aka4Splitter::TrackInfo::TrackType::TrackType_Video)
			continue;
		if (result == -1)
			result = (int)i;
		if (p->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::CodecStoreType::StoreType_Default &&
			p->Codec.CodecId.CodecFcc == ISOM_FCC('avc1') &&
			p->Codec.UserdataSize > 8) {
			result = (int)i;
			break;
		}
	}

	return result;
}