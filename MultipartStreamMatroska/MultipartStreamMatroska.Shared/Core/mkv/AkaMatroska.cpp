/* ---------------------------------------------------------------
 -
 - The AkaMatroska : Simple library to generate matroska file.
 - AkaMatroska.cpp

 - MIT License
 - Copyright (C) 2015 ShanYe
 - Rev: 1.0.0
 -
 --------------------------------------------------------------- */

#include "AkaMatroska.h"

#define LIBRARY_NAME "akaMatroska 20150831"
#define MAX_TRACK_COUNT 123

#define SEGMENT_ID_AND_SIZE_OFFSET 12 // 4id + 8size.
#define SEEKHEAD_DEFAULT_RESERVED_SIZE (28 * 8) // 7 items.
#define CLUSTER_TOTAL_SIZE_MAX_LIMITED (1048576 * 7) //7MB
#define CLUSTER_TIMECODE_MAX_OFFSET 10.0 //10 seconds

static const unsigned char INFO_UUID[] = {0xFA,0x8F,0xE8,0xA2,0xD7,0x3A,0x49,0x88,0x29,0xE1,0x3B,0x87,0x43,0x8E,0x73,0xB6};
static const double NANO_SEC = 1000000000.0;

#pragma pack(1)
struct BlockHeader //4bytes
{
	unsigned char TrackNumber; //Ebml Coded.
	short BlockTimecode;
	unsigned char Flags;
};
#pragma pack(1)

using namespace AkaMatroska;

Matroska::Matroska(Core::IOCallback* cb) throw() :
	_cb(cb), _segment(nullptr), _cur_cluster(nullptr),
	_extern_tracks(&_tracks),
	_extern_chapters(&_chapters),
	_extern_tags(&_tags),
	_extern_files(&_files) {
	memset(&_cfg, 0, sizeof(_cfg));
}

bool Matroska::Begin(const Objects::Header& head, const Configs& cfg)
{
	if (_segment != nullptr)
		return false;

	_cfg = cfg;
	_offset_info = _offset_tracks = _offset_chapters = _offset_tags = _offset_files = _offset_cues = 0;
	_seginfo_duration_offset = 0;
	_cur_cluster_position = 0;
	_cur_cluster_datasize = 0;
	_cur_cluster_time_start = _cur_cluster_time_offset = 0.0;

	WriteEbmlDoc(cfg.DocTypeWebM ? "webm" : "matroska");
	_segment = new(std::nothrow) Ebml::Master(_cb, Consts::Matroska::Root_Segment);
	if (_segment == nullptr)
		return false;
	_segment_offset = _cb->GetPosition();
	if (_segment_offset <= 0)
		return false;

	//reserved spaces for SeekHead entry and Cues.
	if (!cfg.NonKeyFrameIndex)
		Ebml::WriteVoid(_cb, SEEKHEAD_DEFAULT_RESERVED_SIZE + cfg.ReservedEbmlVoidSize);

	WriteSegmentInfo(head.Title, head.Application, head.Duration);
	WriteTracks();
	WriteChapters();
	if (!_cfg.WriteTagsToEnded)
		WriteTags();
	if (!_cfg.WriteFilesToEnded)
		WriteFiles();

	return true;
}

bool Matroska::Ended(double new_duration)
{
	if (_segment == nullptr)
		return false;

	CloseCurCluster();
	if (!_cfg.NonKeyFrameIndex)
		if (_cues_index.GetCount() > 0)
			WriteCues();

	if (_cfg.WriteFilesToEnded)
		WriteFiles();
	if (_cfg.WriteTagsToEnded)
		WriteTags();

	if (new_duration > 0.0)
		if (!WriteNewDuration(new_duration * 1000.0))
			return false;

	if (!_cfg.NonKeyFrameIndex)
	{
		int64_t pos = _cb->GetPosition();
		if (_cb->SetPosition(_segment_offset) != _segment_offset)
			return false;
		WriteSeekHead();
		Ebml::WriteVoid(_cb,
			(SEEKHEAD_DEFAULT_RESERVED_SIZE + _cfg.ReservedEbmlVoidSize) -
			(unsigned)(_cb->GetPosition() - _segment_offset));
		_cb->SetPosition(pos);
	}

	delete _segment;
	_segment = nullptr;
	return true;
}

bool Matroska::NewSample(const Sample& sample, bool force_non_new_cluster)
{
	_cur_keyframe_index = nullptr;
	if (sample.KeyFrame)
	{
		_cur_keyframe_index = _cues_index.New();
		if (_cur_keyframe_index == nullptr)
			return false;
		_cur_keyframe_index->Timestamp = sample.Timestamp;
	}

	if (_cur_cluster == nullptr)
	{
		if (!StartNewCluster(sample.Timestamp))
			return false;
		return WriteFrame(sample);
	}

	if (CheckNeedNewCluster(sample.Timestamp - _cur_cluster_time_start) || sample.KeyFrame)
		if (!force_non_new_cluster)
			if (!StartNewCluster(sample.Timestamp))
				return false;

	return WriteFrame(sample);
}

///////////// Private Functions /////////////

template<typename T>
static void FreeListItemRes(T* list)
{
	unsigned count = list->GetCount();
	for (unsigned i = 0; i < count; i++)
		list->Get(i)->InternalFree();
}

void Matroska::FreeAllResources()
{
	FreeListItemRes<TrackList>(&_tracks);
	FreeListItemRes<ChapterList>(&_chapters);
	FreeListItemRes<TagList>(&_tags);
	FreeListItemRes<AttachList>(&_files);
}

void Matroska::WriteEbmlDoc(const char* doc_type)
{
	Ebml::Master doc(_cb, Consts::Ebml::Root_Header, 512);
	doc.PutElementUInt(Consts::Ebml::Element_Verson, 1);
	doc.PutElementUInt(Consts::Ebml::Element_ReadVersion, 1);
	doc.PutElementUInt(Consts::Ebml::Element_MaxIdLength, 4);
	doc.PutElementUInt(Consts::Ebml::Element_MaxSizeLength, 8);
	doc.PutElementString(Consts::Ebml::Element_DocType, doc_type);
	doc.PutElementUInt(Consts::Ebml::Element_DocType_Version, 4);
	doc.PutElementUInt(Consts::Ebml::Element_DocType_ReadVersion, 2);
}

void Matroska::WriteSegmentInfo(const char* title, const char* app_name, double duration)
{
	_offset_info = _cb->GetPosition() - _segment_offset;
	Ebml::Master info(_cb, Consts::Matroska::Master_Information, 16384);
	info.PutElementUInt(Consts::Matroska::Information::Element_TimecodeScale, 1000000);
	_seginfo_duration_offset = _cb->GetPosition();
	info.PutElementFloat(Consts::Matroska::Information::Element_Duration, duration * 1000.0);
	info.PutElementBinary(Consts::Matroska::Information::Element_UID, &INFO_UUID, 16);
	info.PutElementString(Consts::Matroska::Information::Element_MuxingApp, LIBRARY_NAME);
	info.PutElementString(Consts::Matroska::Information::Element_WritingApp, app_name[0] == 0 ? LIBRARY_NAME : app_name);
	if (title[0] != 0)
		info.PutElementString(Consts::Matroska::Information::Element_Title, title);
}

void Matroska::WriteTracks()
{
	if (_tracks.GetCount() == 0)
		return;

	unsigned count = _tracks.GetCount();
	if (count > MAX_TRACK_COUNT)
		count = MAX_TRACK_COUNT;

	_offset_tracks = _cb->GetPosition() - _segment_offset;
	Ebml::Master tracks(_cb, Consts::Matroska::Master_Tracks, INT32_MAX / 2);
	for (unsigned i = 0; i < count; i++)
	{
		auto t = _tracks.Get(i);
		Ebml::Master track(_cb, Consts::Matroska::Tracks::Master_Track_Entry, INT32_MAX / 4);
		WriteTrackGlobal(track, t, i);
		WriteTrackCodec(track, t);
		WriteTrackAVMedia(track, t);
	}
}

void Matroska::WriteTrackGlobal(Ebml::Master& track, Objects::Track* info, unsigned index)
{
	track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackNumber, index + 1);
	track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackUID, info->UniqueId);
	if (info->Name && info->Name[0] != 0)
		track.PutElementString(Consts::Matroska::Tracks::Element_Name, info->Name);
	if (info->LangId[0] != 0)
		track.PutElementString(Consts::Matroska::Tracks::Element_Language, info->LangId);
	if (info->FrameDefaultDuration != 0.0)
		track.PutElementUInt(Consts::Matroska::Tracks::Element_DefaultDuration, uint64_t(info->FrameDefaultDuration * NANO_SEC));

	if (info->Props.Default)
		track.PutElementUInt(Consts::Matroska::Tracks::Element_FlagDefault, 1);
	if (info->Props.Enabled)
		track.PutElementUInt(Consts::Matroska::Tracks::Element_FlagEnabled, 1);
	if (info->Props.Forced)
		track.PutElementUInt(Consts::Matroska::Tracks::Element_FlagForced, 1);

	track.PutElementUInt(Consts::Matroska::Tracks::Element_FlagLacing, 0);
	track.PutElementUInt(Consts::Matroska::Tracks::Element_MinCache, 1);
	switch (info->Type)
	{
	case Objects::Track::TrackType::TrackType_Audio:
		track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackType, Consts::Container::TrackType_Audio);
		break;
	case Objects::Track::TrackType::TrackType_Video:
		track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackType, Consts::Container::TrackType_Video);
		break;
	case Objects::Track::TrackType::TrackType_Subtitle:
		track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackType, Consts::Container::TrackType_Subtitle);
		break;
	default:
		track.PutElementUInt(Consts::Matroska::Tracks::Element_TrackType, 0);
	}
}

void Matroska::WriteTrackCodec(Ebml::Master& track, Objects::Track* info)
{
	if (info->Codec.CodecId[0] != 0)
		track.PutElementString(Consts::Matroska::Tracks::Element_CodecId, info->Codec.CodecId);
	if (info->Codec.CodecName[0] != 0)
		track.PutElementString(Consts::Matroska::Tracks::Element_CodecName, info->Codec.CodecName);

	if (info->Codec.CodecPrivateSize > 0 && info->Codec.CodecPrivate != nullptr)
		track.PutElementBinary(Consts::Matroska::Tracks::Element_CodecPrivate, info->Codec.CodecPrivate, info->Codec.CodecPrivateSize);
	if (info->Codec.CodecDelay > 0)
		track.PutElementUInt(Consts::Matroska::Tracks::Element_CodecDelay, info->Codec.CodecDelay);
	track.PutElementUInt(Consts::Matroska::Tracks::Element_CodecDecodeAll, info->Codec.CodecDecodeAll ? 1 : 0);
}

void Matroska::WriteTrackAVMedia(Ebml::Master& track, Objects::Track* info)
{
	if (info->Type == Objects::Track::TrackType::TrackType_Unknown ||
		info->Type == Objects::Track::TrackType::TrackType_Subtitle)
		return;

	Ebml::Master media(_cb, 
		info->Type == Objects::Track::TrackType::TrackType_Audio ? 
		Consts::Matroska::Tracks::Master_Audio : Consts::Matroska::Tracks::Master_Video, 64);

	if (info->Type == Objects::Track::TrackType::TrackType_Audio)
	{
		media.PutElementFloat(Consts::Matroska::Tracks::Audio::Element_SamplingFrequency, double(info->Audio.SampleRate));
		if (info->Audio.Channels > 0)
			media.PutElementUInt(Consts::Matroska::Tracks::Audio::Element_Channels, info->Audio.Channels);
		if (info->Audio.BitDepth > 0)
			media.PutElementUInt(Consts::Matroska::Tracks::Audio::Element_BitDepth, info->Audio.BitDepth);
	}else if (info->Type == Objects::Track::TrackType::TrackType_Video)
	{
		media.PutElementUInt(Consts::Matroska::Tracks::Video::Element_PixelWidth, info->Video.Width);
		media.PutElementUInt(Consts::Matroska::Tracks::Video::Element_PixelHeight, info->Video.Height);
		if (info->Video.DisplayWidth > 0)
			media.PutElementUInt(Consts::Matroska::Tracks::Video::Element_DisplayWidth, info->Video.DisplayWidth);
		if (info->Video.DisplayHeight > 0)
			media.PutElementUInt(Consts::Matroska::Tracks::Video::Element_DisplayHeight, info->Video.DisplayHeight);

		if (info->Video.Interlaced)
			media.PutElementUInt(Consts::Matroska::Tracks::Video::Element_FlagInterlaced, 1);
	}
}

void Matroska::WriteChapters()
{
	if (_chapters.GetCount() == 0)
		return;

	_offset_chapters = _cb->GetPosition() - _segment_offset;
	Ebml::Master chapters(_cb, Consts::Matroska::Master_Chapters, INT32_MAX / 2);
	{
		Ebml::Master chapter(_cb, Consts::Matroska::Chapters::Master_Edition_Entry, 65536);
		chapter.PutElementUInt(Consts::Matroska::Chapters::Element_EditionFlagDefault, 0);
		chapter.PutElementUInt(Consts::Matroska::Chapters::Element_EditionFlagHidden, 0);
		chapter.PutElementUInt(Consts::Matroska::Chapters::Element_EditionUID, 1);
		for (unsigned i = 0; i < _chapters.GetCount(); i++)
		{
			auto t = _chapters.Get(i);
			WriteChapterAtom(chapter, t, i + 1);
		}
	}
}

void Matroska::WriteChapterAtom(Ebml::Master& chapter, Objects::Chapter* info, unsigned uid)
{
	Ebml::Master atom(_cb, Consts::Matroska::Chapters::Master_ChapterAtom, 65536);
	atom.PutElementUInt(Consts::Matroska::Chapters::Element_ChapterTimeStart, uint64_t(info->Timestamp * NANO_SEC));
	atom.PutElementUInt(Consts::Matroska::Chapters::Element_ChapterFlagEnabled, 1);
	atom.PutElementUInt(Consts::Matroska::Chapters::Element_ChapterFlagHidden, 0);
	atom.PutElementUInt(Consts::Matroska::Chapters::Element_ChapterUID, uid);
	if (info->Title != nullptr && info->Title[0] != 0)
	{
		Ebml::Master display(_cb, Consts::Matroska::Chapters::Master_ChapterDisplay, 65536);
		display.PutElementString(Consts::Matroska::Chapters::Element_ChapString, info->Title);
		if (info->LangId[0] != 0)
			display.PutElementString(Consts::Matroska::Chapters::Element_ChapLanguage, info->LangId);
	}
}

void Matroska::WriteTags()
{
	if (_tags.GetCount() == 0)
		return;

	_offset_tags = _cb->GetPosition() - _segment_offset;
	Ebml::Master tags(_cb, Consts::Matroska::Master_Tags, INT32_MAX / 2);
	for (unsigned i = 0; i < _tags.GetCount(); i++)
	{
		auto t = _tags.Get(i);
		Ebml::Master tag(_cb, Consts::Matroska::Tags::Master_Tag_Entry, 65536);
		{
			Ebml::Master targets(_cb, Consts::Matroska::Tags::Master_Tag_Targets, 1);
		}
		WriteSimpleTag(tag, t);
	}
}

void Matroska::WriteSimpleTag(Ebml::Master& tag, Objects::Tag* info)
{
	Ebml::Master stag(_cb, Consts::Matroska::Tags::Master_SimpleTag_Entry, 65536);
	if (info->Name && info->Name[0] != 0)
		stag.PutElementString(Consts::Matroska::Tags::Element_TagName, info->Name);
	if (info->String && info->String[0] != 0)
		stag.PutElementString(Consts::Matroska::Tags::Element_TagString, info->String);
	if (info->LangId[0] != 0)
		stag.PutElementString(Consts::Matroska::Tags::Element_TagLanguage, info->LangId);
}

void Matroska::WriteFiles()
{
	if (_files.GetCount() == 0)
		return;

	_offset_files = _cb->GetPosition() - _segment_offset;
	Ebml::Master files(_cb, Consts::Matroska::Master_Attachments);
	for (unsigned i = 0; i < _files.GetCount(); i++)
	{
		auto t = _files.Get(i);
		Ebml::Master file(_cb, Consts::Matroska::Attachments::Master_AttachedFile_Entry);
		WriteFile(file, t);
	}
}

void Matroska::WriteFile(Ebml::Master& file, Objects::Attachment* info)
{
	if (info->FileName[0] != 0)
		file.PutElementString(Consts::Matroska::Attachments::Element_FileName, info->FileName);
	if (info->FileDescription[0] != 0)
		file.PutElementString(Consts::Matroska::Attachments::Element_FileDescription, info->FileDescription);
	if (info->FileMimeType[0] != 0)
		file.PutElementString(Consts::Matroska::Attachments::Element_FileMimeType, info->FileMimeType);

	if (info->DataSize > 0 && info->Data != nullptr)
		file.PutElementBinary(Consts::Matroska::Attachments::Element_FileData, info->Data, info->DataSize);
}

void Matroska::WriteCues()
{
	_offset_cues = _cb->GetPosition() - _segment_offset;
	Ebml::Master cues(_cb, Consts::Matroska::Master_Cues, UINT32_MAX);
	for (unsigned i = 0; i < _cues_index.GetCount(); i++)
	{
		auto t = _cues_index.Get(i);
		Ebml::Master cue(_cb, Consts::Matroska::Cues::Master_CuePoint_Entry, 512);
		cue.PutElementUInt(Consts::Matroska::Cues::Element_CueTime, uint64_t(t->Timestamp * 1000.0));
		{
			Ebml::Master pos(_cb, Consts::Matroska::Cues::Master_CueTrackPositions, 512);
			pos.PutElementUInt(Consts::Matroska::Cues::Element_CueTrack, t->TrackNumber);
			pos.PutElementUInt(Consts::Matroska::Cues::Element_CueClusterPosition, t->ClusterPosition);
		}
	}
}

void Matroska::WriteSeekHead()
{
	unsigned char temp[SEEKHEAD_DEFAULT_RESERVED_SIZE] = {};
	memset(&temp, 0, SEEKHEAD_DEFAULT_RESERVED_SIZE);
	_cb->Write(temp, SEEKHEAD_DEFAULT_RESERVED_SIZE);
	_cb->SetPosition(_segment_offset);

	Ebml::Master seekhead(_cb, Consts::Matroska::Master_SeekHead, 16384);
	unsigned id;
	if (_offset_info > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Information);
		WriteSeekEntry(&id, _offset_info);
	}
	if (_offset_tracks > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Tracks);
		WriteSeekEntry(&id, _offset_tracks);
	}
	if (_offset_chapters > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Chapters);
		WriteSeekEntry(&id, _offset_chapters);
	}
	if (_offset_tags > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Tags);
		WriteSeekEntry(&id, _offset_tags);
	}
	if (_offset_files > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Attachments);
		WriteSeekEntry(&id, _offset_files);
	}
	if (_offset_cues > 0)
	{
		id = EBML_SWAP32(Consts::Matroska::Master_Cues);
		WriteSeekEntry(&id, _offset_cues);
	}
}

void Matroska::WriteSeekEntry(const unsigned* id, int64_t pos)
{
	Ebml::Master entry(_cb, Consts::Matroska::SeekHead::Master_Seek_Entry, 1);
	entry.PutElementBinary(Consts::Matroska::SeekHead::Element_SeekId, id, 4);
	entry.PutElementUInt(Consts::Matroska::SeekHead::Element_SeekPos, pos);
}

bool Matroska::WriteNewDuration(double duration)
{
	uint64_t temp = *(uint64_t*)&duration;
	temp = EBML_SWAP64(temp);

	int64_t old_pos = _cb->GetPosition();
	if (_cb->SetPosition(_seginfo_duration_offset) != _seginfo_duration_offset)
		return false;
	if (!Ebml::WriteId(_cb, Consts::Matroska::Information::Element_Duration) || !Ebml::WriteSize(_cb, 8))
		return false;
	if (_cb->Write(&temp, 8) != 8)
		return false;
	return _cb->SetPosition(old_pos) == old_pos;
}

bool Matroska::ProcessBlockHeader(const Sample& sample, void* header, bool key_frame)
{
	unsigned char num = (unsigned char)FindTrackNumberByUniqueId(sample.UniqueId);
	if (num == 0)
		return false;

	BlockHeader* head = (BlockHeader*)header;
	head->Flags = 0;
	if (key_frame) {
		head->Flags = 0x80;
		if (_cur_keyframe_index)
			_cur_keyframe_index->TrackNumber = num;
	}
	head->TrackNumber = 0x80 | num;

	double offset = sample.Timestamp - _cur_cluster_time_start;
	_cur_cluster_time_offset += offset;
	short timecode = short(offset * 1000.0);
	head->BlockTimecode = EBML_SWAP16(timecode);
	return true;
}

bool Matroska::WriteSimpleBlock(const Sample& sample)
{
	BlockHeader head;
	if (!ProcessBlockHeader(sample, &head, sample.KeyFrame))
		return false;

	const void* data[2] = {&head, sample.Data};
	unsigned size[2] = {4, sample.DataSize};
	return _cur_cluster->PutElementBArray(Consts::Matroska::Cluster::Element_SimpleBlock, data, size, 2);
}

bool Matroska::WriteBlockGroup(const Sample& sample)
{
	Ebml::Master group(_cb, Consts::Matroska::Cluster::Master_BlockGroup, sample.DataSize * 2);

	BlockHeader head;
	if (!ProcessBlockHeader(sample, &head, false))
		return false;

	const void* data[2] = {&head, sample.Data};
	unsigned size[2] = {4, sample.DataSize};
	if (!group.PutElementBArray(Consts::Matroska::Cluster::Element_Block, data, size, 2))
		return false;

	group.PutElementUInt(Consts::Matroska::Cluster::Element_BlockDuration, uint64_t(sample.Duration * 1000.0));
	if (sample.KeyFrame)
		group.PutElementUInt(Consts::Matroska::Cluster::Element_ReferenceBlock, 1);
	return true;
}

bool Matroska::WriteFrame(const Sample& sample)
{
	bool result = sample.Duration > 0.0 ? WriteBlockGroup(sample) : WriteSimpleBlock(sample);
	if (!result)
		return false;
	_cur_cluster_datasize += sample.DataSize + 4 + 1 + 1;

	if (_cur_keyframe_index)
		_cur_keyframe_index->ClusterPosition = _cur_cluster_position;
	return true;
}

bool Matroska::CheckNeedNewCluster(double offset_with_start)
{
	if (_cfg.MaxClusterTotalSize > 0 &&
		(unsigned)_cur_cluster_datasize > _cfg.MaxClusterTotalSize)
		return true;
	if (_cur_cluster_datasize >= CLUSTER_TOTAL_SIZE_MAX_LIMITED)
		return true;
	if (offset_with_start > CLUSTER_TIMECODE_MAX_OFFSET)
		return true;
	return false;
}

bool Matroska::StartNewCluster(double time)
{
	if (_cur_cluster)
		CloseCurCluster();

	int64_t old_pos = _cur_cluster_position;
	_cur_cluster_position = _cb->GetPosition() - _segment_offset;
	_cur_cluster = new(std::nothrow) Ebml::Master(_cb, Consts::Matroska::Master_Cluster, UINT32_MAX);
	if (_cur_cluster == nullptr)
		return false;

	_cur_cluster_time_start = time < 0.0 ? 0.0 : time;
	_cur_cluster->PutElementUInt(Consts::Matroska::Cluster::Element_Timecode, uint64_t(_cur_cluster_time_start * 1000.0));
	if (!_cfg.NonPrevClusterSize && old_pos > 0)
		_cur_cluster->PutElementUInt(Consts::Matroska::Cluster::Element_PrevSize, _cur_cluster_position - old_pos);
	return true;
}

void Matroska::CloseCurCluster()
{
	if (_cur_cluster == nullptr)
		return;
	delete _cur_cluster;
	_cur_cluster = nullptr;

	_cur_cluster_datasize = 0;
	_cur_cluster_time_start = _cur_cluster_time_offset = 0.0;
}

unsigned Matroska::FindTrackNumberByUniqueId(unsigned uid)
{
	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		if (_tracks.Get(i)->UniqueId == uid)
			return i + 1;
	}
	return 0;
}