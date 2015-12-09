#include "MKVMediaFormat.h"

bool MKVMediaFormat::MakeAllStreams(std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	unsigned count = parser->GetTrackCount();
	if (count > MAX_MATROSKA_STREAM_COUNT)
		count = MAX_MATROSKA_STREAM_COUNT;

	for (unsigned i = 0;i < count;i++)
	{
		MKVParser::MKVTrackInfo info = {};
		if (!parser->GetTrackInfo(i,&info))
			continue;

		if (info.Type != MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeAudio &&
			info.Type != MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeVideo &&
			info.Type != MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeSubtitle)
			continue;

		auto s = std::make_shared<MKVMediaStream>(&info);
		switch (info.Type)
		{
		case MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeAudio:
			if (!s->ProbeAudio(parser))
				continue;
			break;
		case MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeVideo:
			if (!s->ProbeVideo(parser,_force_avc1))
				continue;
			if (info.Codec.CodecType == MatroskaCodecIds::MKV_Video_H264 ||
				info.Codec.CodecType == MatroskaCodecIds::MKV_Video_HEVC) {
				s->SetDecodeTimestamp(PACKET_NO_PTS);
				if (info.Codec.CodecType == MatroskaCodecIds::MKV_Video_HEVC)
				{
					s->SetBFrameFlag();
				}else{
					H264_PROFILE_SPEC profile = {};
					if (s->GetVideoInfo()->GetProfile(&profile))
						if (profile.profile > H264_PROFILE_BASELINE)
							s->SetBFrameFlag();
				}
			}
			break;
		case MKVParser::MKVTrackInfo::MKVTrackType::TrackTypeSubtitle:
			if (!_skip_av_others_streams)
			{
				if (!s->ProbeSubtitle(parser))
					continue;
				if ((_read_flags & MEDIA_FORMAT_READER_MATROSKA_ASS2SRT) != 0)
					if (s->GetCodecType() != MediaCodecType::MEDIA_CODEC_SUBTITLE_TEXT)
						s->ForceChangeCodecType(MEDIA_CODEC_SUBTITLE_TEXT);
				break;
			}else{
				continue;
			}
		default:
			continue;
		}

		_streams[_stream_count] = s;
		_stream_count++;
	}

	return true;
}

unsigned MKVMediaFormat::GetAllAudioStreamsBitrate(unsigned* first_video_index)
{
	unsigned result = 0;
	unsigned video_stream_count = 0;

	for (unsigned i = 0;i < _stream_count;i++)
	{
		if (_streams[i]->GetMainType() == MediaMainType::MEDIA_MAIN_TYPE_VIDEO)
		{
			if (first_video_index)
				*first_video_index = i;
			video_stream_count++;
		}
		if (_streams[i]->GetMainType() != MediaMainType::MEDIA_MAIN_TYPE_AUDIO)
			continue;

		auto p = _streams[i]->GetAudioInfo();
		if (p == nullptr)
			continue;

		AudioBasicDescription desc = {};
		if (!p->GetAudioDescription(&desc))
			return 0;
		if (desc.bitrate == 0)
			return 0;

		result += desc.bitrate;
	}
	if (video_stream_count != 1)
		return 0;

	return result;
}

void MKVMediaFormat::LoadMatroskaChapters(std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	auto list = parser->GetCore()->GetChapters();
	if (parser->GetCore()->GetSegmentInfo()->GetTimecodeScale() == 0)
		return;
	if (list == nullptr)
		return;
	if (list->GetChapterCount() == 0)
		return;

	for (unsigned i = 0;i < list->GetChapterCount();i++)
	{
		auto p = list->GetChapter(i);
		if (p == nullptr)
			break;
		if (p->fHidden || !p->fEnabled)
			continue;
		if (p->Display.ChapString == nullptr)
			continue;
		if (strlen(p->Display.ChapString) > 255)
			continue;
		if (p->ChapterTimeStart == -1)
			continue;

		AVMediaChapter chap = {};
		chap.type = AVChapterType::ChapterType_Marker;
		chap.str_enc_type = AVChapterEncodingType::ChapterStringEnc_UTF8;
		chap.index = (double)i;
		chap.end_time = chap.duration = PACKET_NO_PTS;
		chap.start_time = (double)p->ChapterTimeStart /
			(double)parser->GetCore()->GetSegmentInfo()->GetTimecodeScale() / 1000.0;
		if (p->ChapterTimeEnd > 0)
			chap.end_time = (double)p->ChapterTimeEnd /
				(double)parser->GetCore()->GetSegmentInfo()->GetTimecodeScale() / 1000.0;
		if (chap.end_time > chap.start_time)
			chap.duration = chap.end_time - chap.start_time;
		strcpy(chap.title,p->Display.ChapString);
		memcpy(chap.lang,p->Display.ChapLanguage,MKV_ARRAY_COUNT(chap.lang));

		_chaps.AddItem(&chap);
	}
}

void MKVMediaFormat::LoadMatroskaAttachFiles(std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	auto list = parser->GetCore()->GetAttachments();
	if (list == nullptr)
		return;
	if (list->GetFileCount() == 0)
		return;

	for (unsigned i = 0;i < list->GetFileCount();i++)
	{
		auto p = list->GetFile(i);
		if (p == nullptr)
			continue;
		if (p->FileName == nullptr ||
			p->FileDataSize < 1)
			continue;
		if (strlen(p->FileName) >= MAX_AV_RES_FILENAME_LEN)
			continue;
		if (p->FileName[0] == 0)
			continue;

		AVMediaResource res = {};
		strcpy(res.file_name,p->FileName);
		strcpy(res.mime_type,p->FileMimeType);
		res.data_size = p->FileDataSize;
		res.seek_pos = p->OnAbsolutePosition;

		_resources.AddItem(&res);
	}
}