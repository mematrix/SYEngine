#include "MatroskaMerge.h"
#include "parser/AACParser.h"
#include "parser/AVCParser.h"
#include "parser/FLACParser.h"

bool MatroskaMerge::CreateTracks()
{
	//处理H264和AAC
	AVCParser avc = {};
	AACParser aac = {};
	FLACParser flac = {};

	DemuxProxy* demux = GetDemuxObject();
	if (demux == NULL)
		return false;
	if (demux->GetTrackCount() < 2)
		return false;

	DemuxProxy::Track audio = {}, video = {};
	demux->GetTrack(0, &video);
	demux->GetTrack(1, &audio);
	if (strcmp(video.CodecName, "h264") != 0)
		return false;
	if (strcmp(audio.CodecName, "aac") != 0 &&
		strcmp(audio.CodecName, "flac") != 0)
		return false;
	if (video.CodecPrivateSize == 0 ||
		audio.CodecPrivateSize < 2)
		return false;

	bool is_flac = (strcmp(audio.CodecName, "flac") == 0);
	if (is_flac)
		flac.Parse((unsigned char*)audio.CodecPrivate, audio.CodecPrivateSize);
	else
		aac.Parse((unsigned char*)audio.CodecPrivate);
	avc.Parse((unsigned char*)video.CodecPrivate, video.CodecPrivateSize);

	unsigned nch = is_flac ? flac.nch : aac.channels;
	unsigned rate = is_flac ? flac.rate : aac.samplerate;
	unsigned bps = is_flac ? flac.bps : 16;
	if (nch == 0 ||
		rate == 0)
		return false;
	if (avc.width == 0 ||
		avc.height == 0)
		return false;

	AkaMatroska::Objects::Track ta = {}, tv = {};
	if (!InitMatroskaTrack(&audio, &ta))
		return false;
	if (!InitMatroskaTrack(&video, &tv))
		return false;

	_audio_extradata.Alloc(audio.CodecPrivateSize);
	_video_extradata.Alloc(video.CodecPrivateSize);
	memcpy(_audio_extradata.Get<void*>(), audio.CodecPrivate, audio.CodecPrivateSize);
	memcpy(_video_extradata.Get<void*>(), video.CodecPrivate, video.CodecPrivateSize);

	//Audio...
	ta.Audio.Channels = nch;
	ta.Audio.SampleRate = rate;
	ta.Audio.BitDepth = bps;
	//Video...
	tv.Video.Width = avc.width;
	tv.Video.Height = avc.height;

	if (!_muxer->Tracks()->Add(&tv) ||
		!_muxer->Tracks()->Add(&ta))
		return false;
	return true;
}

bool MatroskaMerge::InitMatroskaTrack(DemuxProxy::Track* st, AkaMatroska::Objects::Track* dt)
{
	if (st->CodecPrivateSize == 0)
		return false;
	
	dt->UniqueId = st->Id;
	dt->Type = (st->Type == DemuxProxy::Track::TrackType::Video ?
		AkaMatroska::Objects::Track::TrackType::TrackType_Video:
		AkaMatroska::Objects::Track::TrackType::TrackType_Audio);

	unsigned char flac_codecprivate[128] = {};
	strcpy((char*)&flac_codecprivate, "fLaC");
	flac_codecprivate[7] = 0x22;
	if (st->CodecPrivateSize < 64)
		memcpy(&flac_codecprivate[8], st->CodecPrivate, st->CodecPrivateSize);

	char codec_id[64] = {};
	if (strcmp(st->CodecName, "h264") == 0)
		strcpy(codec_id, "V_MPEG4/ISO/AVC");
	else if (strcmp(st->CodecName, "hevc") == 0)
		strcpy(codec_id, "V_MPEGH/ISO/HEVC");
	else if (strcmp(st->CodecName, "aac") == 0)
		strcpy(codec_id, "A_AAC"); //mp4
	else if (strcmp(st->CodecName, "flac") == 0)
		strcpy(codec_id, "A_FLAC"); //测试特殊mkv。。
	else if (strcmp(st->CodecName, "alac") == 0)
		strcpy(codec_id, "A_ALAC"); //测试特殊mp4。。
	else if (strcmp(st->CodecName, "vorbis") == 0)
		strcpy(codec_id, "A_VORBIS"); //webm
	dt->Codec.SetCodecId(codec_id);

	if (strcmp(st->CodecName, "flac") == 0)
		dt->Codec.SetCodecPrivate(flac_codecprivate, st->CodecPrivateSize + 8);
	else
		dt->Codec.SetCodecPrivate(st->CodecPrivate, st->CodecPrivateSize);
	return true;
}

bool MatroskaMerge::OnNewInput(MergeManager::IOCallback* old_io, MergeManager::IOCallback* new_io)
{
	//验证新的分段跟上一分段的codec信息是否完全相同
	DemuxProxy::Track audio = {}, video = {};
	GetDemuxObject()->GetTrack(1, &audio);
	GetDemuxObject()->GetTrack(0, &video);
	if (audio.CodecPrivateSize != _audio_extradata.Length())
		return false;
	if (video.CodecPrivateSize != _video_extradata.Length())
		return false;

	//验证
	if (memcmp(audio.CodecPrivate, _audio_extradata.Get<void*>(), audio.CodecPrivateSize) != 0)
		return false;
	if (memcmp(video.CodecPrivate, _video_extradata.Get<void*>(), video.CodecPrivateSize) != 0)
		return false;
	return true;
}

bool MatroskaMerge::DoProcessHeader(double total_duration)
{
	if (_muxer != NULL)
		return false;

	_muxer = new(std::nothrow) AkaMatroska::Matroska(this);
	if (_muxer == NULL)
		return false;

	//生成所有的轨道信息
	if (!CreateTracks())
		return false;
	_duration = total_duration;

	AkaMatroska::Objects::Header head = {};
	head.Duration = total_duration;
	AkaMatroska::Matroska::Configs cfg = {};
	cfg.NonKeyFrameIndex = _no_key_index; //live流性播放不使用关键帧索引
	cfg.NonPrevClusterSize = true; //不写入上一个cluster大小
	if (_no_key_index)
		cfg.MaxClusterTotalSize = 1; //每个frame都是一个新的cluster

	_head_stream = GetStreamHeader(); //头信息写到一个虚拟的stream
	if (!_muxer->Begin(head, cfg))
		return false;
	_head_stream = NULL; //写到真正的IO
	return true;
}

bool MatroskaMerge::DoProcessComplete(double total_duration)
{
	if (_muxer == NULL)
		return false;
	_duration = total_duration;

	if (_no_key_index)
		_muxer->ForceCloseCurrentCluster();

	_now_process_complete = true;
	if (!_no_key_index)
		_muxer->Ended(total_duration);
	delete _muxer;
	_muxer = NULL;
	_now_process_complete = false;
	return true;
}

bool MatroskaMerge::DoProcessPacketOnce(Packet* pkt)
{
	if (_muxer == NULL)
		return false;
	
	AkaMatroska::Matroska::Sample sample = {};
	sample.UniqueId = pkt->Id;
	sample.Data = pkt->Data;
	sample.DataSize = pkt->DataSize;
	sample.KeyFrame = pkt->KeyFrame;
	sample.Timestamp = pkt->ScalePTS;
	if (pkt->PTS == InvalidTimestamp())
		sample.Timestamp = pkt->ScaleDTS;

	return _muxer->NewSample(sample);
}

unsigned MatroskaMerge::OnWrite(const void* buf, unsigned size)
{
	if (_head_stream)
		return _head_stream->Write(buf, size);
	return (unsigned)GetOutputStream()->Write(buf, size);
}

bool MatroskaMerge::OnSeek(int64_t offset)
{
	if (_now_process_complete && _no_key_index)
		return false;

	if (_head_stream)
		return _head_stream->SetOffset((unsigned)offset);
	return GetOutputStream()->Seek(offset);
}

int64_t MatroskaMerge::OnTell()
{
	if (_head_stream)
		return _head_stream->GetOffset();
	return GetOutputStream()->Tell();
}