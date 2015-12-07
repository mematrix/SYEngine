#include "MKVParser.h"

using namespace MKVParser;

static int MKVDurationToSeconds(const char* duration)
{
	int hour = 0,minute = 0,seconds = 0;
	const char *h = nullptr,*m = nullptr,*s = nullptr;
	h = m = s = duration;
	m = strstr(duration,":");
	if (m == nullptr)
		return 0;
	s = strstr(m + 1,":");
	if (s == nullptr)
		return 0;

	m++;
	s++;
	hour = atoi(h);
	minute = atoi(m);
	seconds = atoi(s);
	return seconds + (minute * 60) + (hour * 60 * 60);
}

MKVFileParser::MKVFileParser(IMKVParserIO* io) : _io(io)
{
	memset(&_info,0,sizeof(_info));
	memset(&_frame,0,sizeof(_frame));
}

unsigned MKVFileParser::Open(bool ignore_duratin_zero,bool do_not_parse_seekhead)
{
	if (_io == nullptr)
		return PARSER_MKV_ERR_UNEXPECTED;
	if (_io->GetSize() > INT64_MAX) //那么大，不科学。
		return PARSER_MKV_ERR_IO;

	unsigned flag = 0;
	if (!_io->Seek())
		return PARSER_MKV_ERR_OPEN_SEEK_FAILED;
	if (_io->Read(&flag,4) == 0)
		return PARSER_MKV_ERR_IO;

	if (flag != 0xA3DF451A) //判断MKV文件头
		return PARSER_MKV_ERR_OPEN_HEAD_MISSING;
	if (!_io->Seek())
		return PARSER_MKV_ERR_OPEN_SEEK_FAILED;

	MKV::EBML::EbmlDocument ebmlDoc = {};
	ebmlDoc.Default();
	if (!ebmlDoc.Parse(this)) //先分析EBML文档
		return PARSER_MKV_ERR_OPEN_DOC_INVALID;
	if (!ebmlDoc.IsDocumentSupported()) //判断EBML文档是不是MKV
		return PARSER_MKV_ERR_OPEN_DOC_UNSUPPORTED;

	//取得MKV的段总长度
	if (ebmlDoc.GetSegmentSize() == 0 ||
		ebmlDoc.GetSegmentSize() > INT64_MAX)
		return PARSER_MKV_ERR_OPEN_HEAD_FAILED;

	//在文件中段的绝对偏移。
	_segmentOffset = (unsigned)_io->Tell();

	long long clusterNextSize = 0;
	auto mkv = std::make_shared<MKV::Internal::MatroskaSegment>(this);

	//分析MKV的段，返回第一个簇的大小。
	auto err = mkv->Parse(ebmlDoc.GetSegmentSize(),&clusterNextSize);
	if (err != MKV_ERR_OK)
		return err;

	if (!do_not_parse_seekhead)
	{
		//如果是网络MKV文件，执行这里因为SeekHead在文件尾，可能会导致MF下载整个文件
		err = mkv->ExecuteSeekHead(); //这里分析一下Cue索引
		if (err != MKV_ERR_OK)
			printf("Matroska Execute SeekHead FAILED!\n");
	}

	//必须有段信息和轨道信息。
	if ((mkv->StatusOfParse() & MKV_SEGMENT_PARSE_INFO) == 0 ||
		(mkv->StatusOfParse() & MKV_SEGMENT_PARSE_TRACKS) == 0)
		return PARSER_MKV_ERR_OPEN_HEAD_FAILED;

	if (strcmpi(ebmlDoc.DocType,MKV_DOC_TYPE_MAIN) == 0)
		_info.Type = MKVGlobalInfo::Matroska;
	else if (strcmpi(ebmlDoc.DocType,MKV_DOC_TYPE_WEBM) == 0)
		_info.Type = MKVGlobalInfo::WebM;
	else
		_info.Type = MKVGlobalInfo::Unknown;

	InitGlobalInfo(mkv); //初始化全局信息。
	if (_info.Duration == 0.0 && !ignore_duratin_zero)
		return PARSER_MKV_ERR_OPEN_TIME_UNSUPPORTED;
	if (_info.Duration == 0.0)
		_info.Duration = InitDurationFromTags(mkv); //mkvMerge会把DURATION写道tag里面

	_track_num_start_with_zero = false;
	if (!InitTracks(mkv)) //初始化所有轨道信息。
		return PARSER_MKV_ERR_OPEN_TRACK_UNSUPPORTED;

	if ((mkv->StatusOfParse() & MKV_SEGMENT_PARSE_CUES) != 0)
		if (!InitKeyFrameIndex(mkv))
			_info.NonKeyFrameIndex = true;

	_is_mka_file = CheckMatroskaAudioOnly();
	if (_is_mka_file)
		_default_seek_tnum = ConvertTrackNumber(_tracks.GetItem()->InternalEntry->TrackNumber);

	_resetPointer = _io->Tell();

	if (_info.NonKeyFrameIndex) //如果没有关键帧索引，设置需要动态计算ClusterPos以便手动建立
		mkv->SetShouldCalcClusterPosition();
	if (_info.Duration == 0.0 && !_info.NonKeyFrameIndex)
		_info.Duration = InitDurationFromKeyFrameIndex();

#ifdef _DEBUG
	mkv->SetShouldCalcClusterPosition();
	_seek_after_flag = false;
#endif

	_frame.nextClusterSize = _firstClusterSize = clusterNextSize;
	_mkv = std::move(mkv);
	return PARSER_MKV_OK;
}

bool MKVFileParser::Close()
{
	if (_io)
		_io->Seek();

	memset(&_info,0,sizeof(_info));
	memset(&_frame,0,sizeof(_frame));

	if (!_tracks.IsEmpty())
	{
		for (unsigned i = 0;i < _tracks.GetCount();i++)
			_tracks.GetItem(i)->InternalFree();
	}

	_tracks.ClearItems();
	_keyframe_index.ClearItems();
	if (_mkv)
		_mkv.reset();

	_resetPointer = 0;
	_segmentOffset = 0;

	_frameBuffer.Free();
	return true;
}

bool MKVFileParser::GetGlobalInfo(MKVGlobalInfo* info)
{
	if (info == nullptr)
		return false;

	memcpy(info,&_info,sizeof(_info));
	return true;
}

bool MKVFileParser::GetTrackInfo(unsigned index,MKVTrackInfo* info)
{
	if (index >= GetTrackCount())
		return false;
	if (info == nullptr)
		return false;

	memcpy(info,_tracks.GetItem(index),sizeof(MKVTrackInfo));
	return true;
}

void MKVFileParser::InitGlobalInfo(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv)
{
	auto p = mkv->GetSegmentInfo();
	if (p == nullptr)
		return;

	if (p->GetWritingApp())
	{
		if (strlen(p->GetWritingApp()) > 0 &&
			strlen(p->GetWritingApp()) < 255)
			strcpy(_info.Application,p->GetWritingApp());
	}

	if (mkv->GetCues() == nullptr)
		_info.NonKeyFrameIndex = true;

	_info.TimecodeScale = (double)p->GetTimecodeScale();
	if (p->IsTimecodeMilliseconds())
		_info.Duration = p->GetDuration() / 1000.0;
	else
		_info.Duration = ((double)p->GetTimecodeScale() / MKV_TIMESTAMP_UNIT_NANOSEC) * p->GetDuration();
}

bool MKVFileParser::InitTracks(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv)
{
	auto p = mkv->GetTracks();
	if (p == nullptr)
		return false;
	if (p->GetTrackCount() == 0)
		return false;

	if (p->GetTrackEntry(0)->TrackNumber == 0) //LEAD MKV Multiplexer...
		_track_num_start_with_zero = true;

	unsigned count = p->GetTrackCount();
	for (unsigned i = 0;i < count;i++)
	{
		auto entry = p->GetTrackEntry(i);
		MKVTrackInfo info = {};
		if (!ConvertTrackEntry2MKVTrackInfo(entry,&info))
			continue;

		if (info.Type == MKVTrackInfo::TrackTypeUnknown)
		{
			if (entry->Track.CodecID[0] == 'V')
				info.Type = MKVTrackInfo::TrackTypeVideo;
			else if (entry->Track.CodecID[0] == 'A')
				info.Type = MKVTrackInfo::TrackTypeAudio;
			else
				continue;
		}

		if (info.Codec.CodecType == MatroskaCodecIds::MKV_Audio_TTA1)
		{
			//build tta head.
			TTA1_FILE_HEADER head = {};
			if (info.Audio.SampleRate > 0)
				BuildTTA1FileHeader(info.Audio.Channels,info.Audio.BitDepth,info.Audio.SampleRate,
					_info.Duration,&head);
			info.Codec.InternalFree();
			info.Codec.InternalCopyCodecPrivate((unsigned char*)&head,sizeof(head));
		}

		if (info.Type == MKVTrackInfo::TrackTypeAudio &&
			info.InternalNonDefaultDuration) {
			//如果音频没有Duration，如果可能，比如AAC\MP3则设置默认的Duration上去
			info.InternalFrameDuration = CalcAudioTrackDefaultDuration(&info);
			if (info.InternalFrameDuration != 0.0)
				info.InternalNonDefaultDuration = false;
		}

		if (info.InternalNonDefaultDuration)
			InitDurationCalcEngine(&info);

		_tracks.AddItem(&info);
	}

	return _tracks.GetCount() != 0;
}

bool MKVFileParser::ConvertTrackEntry2MKVTrackInfo(MKV::Internal::Object::Context::TrackEntry* entry,MKVTrackInfo* info)
{
	//只支持视频、音频、字幕
	if (!IsTrackTypeSupported(entry->TrackType))
	{
		char t = entry->Track.CodecID[0];
		if (t != 'V' && t != 'A')
			return false;
	}

	if (!entry->fEnabled)
		return false;
	if (entry->Track.CodecID[0] == 0)
		return false;

	if (entry->Encoding.fEncryptionExists)
		return false; //加密的轨道不支持。
	if (entry->Encoding.fCompressionExists)
	{
		//压缩的轨道只支持“去头”and Zlib。
		if (entry->Encoding.ContentEncodingScope > MKV_TRACK_ENCODE_SCOPE_FULL)
			return false;
		if (entry->Encoding.Compression.ContentCompAlgo != MKV_TRACK_COMPRESS_TYPE_HEAD &&
			entry->Encoding.Compression.ContentCompAlgo != MKV_TRACK_COMPRESS_TYPE_ZLIB)
			return false;

		info->CompressType = MKVTrackInfo::TrackCompressNone;
		if (entry->Encoding.Compression.ContentCompAlgo == MKV_TRACK_COMPRESS_TYPE_HEAD)
			info->CompressType = MKVTrackInfo::TrackCompressStripHeader;
		else if (entry->Encoding.Compression.ContentCompAlgo == MKV_TRACK_COMPRESS_TYPE_ZLIB)
			info->CompressType = MKVTrackInfo::TrackCompressZLIB;
	}
	
	info->InternalEntry = entry;
	info->Number = ConvertTrackNumber(entry->TrackNumber);
	info->Default = entry->fDefault;
	strcpy(info->LangID,entry->Track.Language);
	if (entry->Track.Name && (strlen(entry->Track.Name) > 0))
		info->InternalChangeName(entry->Track.Name);

	//取得编码器ID。
	info->Codec.CodecType = MatroskaFindCodecId(entry->Track.CodecID);
	if (info->Codec.CodecType == MKV_Error_CodecId)
		return false;

	strncpy(info->Codec.CodecID,entry->Track.CodecID,MKV_ARRAY_COUNT(info->Codec.CodecID));
	if (entry->Track.CodecPrivate)
	{
		unsigned size = entry->Track.CodecPrivateSize;
		unsigned offset = 0;
		if (!entry->Encoding.fCompressionExists ||
			(entry->Encoding.ContentEncodingScope & MKV_TRACK_ENCODE_SCOPE_PRIVATE_DATA) == 0) {
			if (!info->Codec.InternalCopyCodecPrivate(entry->Track.CodecPrivate,size))
				return false;
		}else{
			//如果是去头压缩，需要合并头。
			if (!info->IsZLibCompressed())
			{
				size += entry->Encoding.Compression.ContentCompSettingsSize;
				offset = entry->Encoding.Compression.ContentCompSettingsSize;

				auto p = (unsigned char*)malloc(MKV_ALLOC_ALIGNED(size));
				memcpy(p,entry->Encoding.Compression.ContentCompSettings,offset);
				memcpy(p + offset,entry->Track.CodecPrivate,entry->Track.CodecPrivateSize);
				if (!info->Codec.InternalCopyCodecPrivate(p,size))
				{
					free(p);
					return false;
				}
				free(p);
			}else{
				//ZLIB压缩，解压。
				unsigned char* pd = nullptr;
				unsigned dsize = 0;
				if (!zlibDecompress(entry->Track.CodecPrivate,entry->Track.CodecPrivateSize,&pd,&dsize))
					return false;

				if (!info->Codec.InternalCopyCodecPrivate(pd,dsize))
				{
					free(pd);
					return false;
				}
				free(pd);
			}
		}
	}

	//such Opus Audio...
	if (entry->Track.CodecDelay > 0)
		info->Codec.CodecDelay = (double)entry->Track.CodecDelay / MKV_TIMESTAMP_UNIT_NANOSEC;
	if (entry->Track.SeekPreroll > 0)
		info->Codec.SeekPreroll = (double)entry->Track.SeekPreroll / MKV_TIMESTAMP_UNIT_NANOSEC;

	if (entry->DefaultDuration > 0) //一个Frame的长度（秒）
		info->InternalFrameDuration = (double)entry->DefaultDuration / MKV_TIMESTAMP_UNIT_NANOSEC;
	if (info->InternalFrameDuration == 0.0)
		info->InternalNonDefaultDuration = true;

	switch (entry->TrackType)
	{
	case MKV_TRACK_TYPE_VIDEO:
		info->Type = MKVTrackInfo::TrackTypeVideo;
		info->Video.Width = entry->Video.PixelWidth;
		info->Video.Height = entry->Video.PixelHeight;
		if (entry->DefaultDuration != 0.0)
			info->Video.FrameRate = (float)(1000000000.0 / (double)entry->DefaultDuration);
		info->Video.Interlaced = entry->Video.fInterlaced;
		if (entry->Video.DisplayWidth > 32 && entry->Video.DisplayHeight > 32)
		{
			info->Video.DisplayWidth = entry->Video.DisplayWidth;
			info->Video.DisplayHeight = entry->Video.DisplayHeight;
		}
		break;
	case MKV_TRACK_TYPE_AUDIO:
		info->Type = MKVTrackInfo::TrackTypeAudio;
		info->Audio.BitDepth = entry->Audio.BitDepth;
		info->Audio.Channels = entry->Audio.Channels;
		info->Audio.SampleRate = (unsigned)entry->Audio.SamplingFrequency;
		break;
	case MKV_TRACK_TYPE_SUBTITLE:
		info->Type = MKVTrackInfo::TrackTypeSubtitle;
		break;
	default:
		info->Type = MKVTrackInfo::TrackTypeUnknown;
	}

	return true;
}

bool MKVFileParser::InitKeyFrameIndex(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv)
{
	if (mkv->GetCues() == nullptr ||
		mkv->GetCues()->GetCuePointCount() == 0)
		return false;

	for (unsigned i = 0;i < mkv->GetCues()->GetCuePointCount();i++)
	{
		auto p = mkv->GetCues()->GetCuePoint(i);
		if (p == nullptr)
			continue;

		MKVKeyFrameIndex index = {};
		index.Time = CalcTimestamp(p->CueTime);
		index.TrackNumber = p->Positions.CueTrack;
		index.ClusterPosition = p->Positions.CueClusterPosition;
		index.BlockNumber = (unsigned)p->Positions.CueBlockNumber;
		if (index.TrackNumber == 0 ||
			index.ClusterPosition == 0)
			continue;
		auto t = FindTrackInfo(p->Positions.CueTrack);
		if (t) {
			if (t->Codec.CodecID[0] == 'S')
				index.SubtitleTrack = true;
		}

		index.ClusterPosition += _segmentOffset;
		_keyframe_index.AddItem(&index);
	}

	_default_seek_tnum = ConvertTrackNumber(_keyframe_index.GetItem(0)->TrackNumber);
	for (unsigned i = 0;i < _tracks.GetCount();i++)
	{
		if (_tracks.GetItem(i)->InternalEntry->Track.CodecID[0] != 'V')
			continue;

		bool found = false;
		for (unsigned j = 0;j < _keyframe_index.GetCount();j++)
			if (_keyframe_index.GetItem(j)->TrackNumber == _tracks.GetItem(i)->InternalEntry->TrackNumber) {
				_default_seek_tnum = ConvertTrackNumber(_keyframe_index.GetItem(j)->TrackNumber);
				found = true;
				break;
			}

		if (found)
			break;
	}

	return true;
}

double MKVFileParser::InitDurationFromTags(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv)
{
	if (mkv->GetTags() == nullptr)
		return 0.0;
	if (mkv->GetTags()->GetTagCount() == 0)
		return 0.0;

	double result = 0.0;
	for (unsigned i = 0;i < mkv->GetTags()->GetTagCount();i++)
	{
		auto tag = mkv->GetTags()->GetTag(i);
		if (tag == nullptr || tag->TagName == nullptr || tag->TagString == nullptr)
			continue;
		if (strcmpi(tag->TagName,"DURATION") != 0)
			continue;
		if (strstr(tag->TagString,":") == nullptr)
			continue;

		result = (double)MKVDurationToSeconds(tag->TagString);
		break;
	}
	return result;
}

double MKVFileParser::InitDurationFromKeyFrameIndex()
{
	if (_info.NonKeyFrameIndex)
		return 0.0;
	if (_keyframe_index.GetCount() <= 1)
		return 0.0;

	auto p = _keyframe_index.GetItem(_keyframe_index.GetCount() - 1);
	return p->Time + 15;
}

double MKVFileParser::CalcAudioTrackDefaultDuration(MKVTrackInfo* info)
{
	double result = 0.0;
	if (info->Type != MKVTrackInfo::TrackTypeAudio)
		return result;
	if (info->InternalFrameDuration != 0.0)
		return result;
	if (info->Audio.SampleRate == 0)
		return result;
	
	switch (info->Codec.CodecType)
	{
	case MatroskaCodecIds::MKV_Audio_AAC:
		result = 1024.0 / (double)info->Audio.SampleRate;
		break;
	case MatroskaCodecIds::MKV_Audio_MP2:
	case MatroskaCodecIds::MKV_Audio_MP3:
		result = 1152.0 / (double)info->Audio.SampleRate;
		break;
	case MatroskaCodecIds::MKV_Audio_AC3:
		result = 0.032; //32ms
		break;
	case MatroskaCodecIds::MKV_Audio_DTS:
		result = 0.010; //10ms
		break;
	case MatroskaCodecIds::MKV_Audio_TTA1:
		result = 1.04489795918367346939;
		break;
	}

	return result;
}

void MKVFileParser::InitDurationCalcEngine(MKVTrackInfo* info)
{
	if (info->InternalDurationCalc)
		return;

	switch (info->Codec.CodecType)
	{
	case MatroskaCodecIds::MKV_Audio_PCM:
	case MatroskaCodecIds::MKV_Audio_PCMBE:
	case MatroskaCodecIds::MKV_Audio_FLOAT:
		info->InternalDurationCalc = 
			CreatePcmDurationCalcEngine(info->Audio.Channels,
				info->Audio.SampleRate,info->Audio.BitDepth,
				info->Codec.CodecType == MKV_Audio_FLOAT);
		break;
	case MatroskaCodecIds::MKV_Audio_ALAC:
		info->InternalDurationCalc = CreateAlacDurationCalcEngine();
		break;
	case MatroskaCodecIds::MKV_Audio_FLAC:
		info->InternalDurationCalc = CreateFlacDurationCalcEngine();
		break;
	case MatroskaCodecIds::MKV_Audio_VORBIS:
		info->InternalDurationCalc = CreateVorbisDurationCalcEngine();
		break;
	case MatroskaCodecIds::MKV_Audio_OPUS:
	case MatroskaCodecIds::MKV_Audio_OPUSE:
		info->InternalDurationCalc = CreateOpusDurationCalcEngine();
		break;
	}

	if (info->InternalDurationCalc)
		info->InternalDurationCalc->Initialize(
			info->Codec.CodecPrivate,info->Codec.CodecPrivateSize);
}

bool MKVFileParser::CheckMatroskaAudioOnly()
{
	if (_tracks.GetCount() == 1)
	{
		if (_tracks.GetItem()->Codec.CodecID[0] == 'A')
			return true;
	}
	return false;
}