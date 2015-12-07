/* ---------------------------------------------------------------
 -
 - The AkaMatroska : Simple library to generate matroska file.
 - AkaMatroska.h

 - MIT License
 - Copyright (C) 2015 ShanYe
 - Rev: 1.0.0
 -
 --------------------------------------------------------------- */

#ifndef __AKA_MATROSKA_H
#define __AKA_MATROSKA_H

#include "AkaMatroskaEbml.h"
#include "AkaMatroskaConst.h"
#include "AkaMatroskaObject.h"
#include "AkaMatroskaStructTable.h"

namespace AkaMatroska {
class Matroska
{
public:
	typedef Core::StructTable<Objects::Track> TrackList;
	typedef Core::StructTable<Objects::Chapter> ChapterList;
	typedef Core::StructTable<Objects::Tag> TagList;
	typedef Core::StructTable<Objects::Attachment> AttachList;

	typedef Core::WriteableStructTable<Objects::Track> ExternTrackList;
	typedef Core::WriteableStructTable<Objects::Chapter> ExternChapterList;
	typedef Core::WriteableStructTable<Objects::Tag> ExternTagList;
	typedef Core::WriteableStructTable<Objects::Attachment> ExternAttachList;

	struct Configs
	{
		unsigned MaxClusterTotalSize;
		unsigned ReservedEbmlVoidSize;
		bool WriteTagsToEnded;
		bool WriteFilesToEnded;
		bool DocTypeWebM;
		bool NonKeyFrameIndex;
		bool NonPrevClusterSize;
	};

	struct Sample
	{
		unsigned UniqueId;
		const unsigned char* Data;
		unsigned DataSize;
		double Timestamp; //PTS
		double Duration;
		bool KeyFrame;
	};
	struct KeyFrameIndex
	{
		double Timestamp; //PTS
		unsigned TrackNumber;
		int64_t ClusterPosition;
	};

public:
	explicit Matroska(Core::IOCallback* cb) throw();
	~Matroska() throw() { if (_segment) delete _segment; FreeAllResources(); }

	bool Begin(const Objects::Header& head, const Configs& cfg);
	bool NewSample(const Sample& sample, bool force_non_new_cluster = false);
	bool Ended(double new_duration = 0.0);

	void ForceCloseCurrentCluster()
	{ CloseCurCluster(); }

	inline ExternTrackList* Tracks() throw() { return &_extern_tracks; }
	inline ExternChapterList* Chapters() throw() { return &_extern_chapters; }
	inline ExternTagList* Tags() throw() { return &_extern_tags; }
	inline ExternAttachList* Files() throw() { return &_extern_files; }

private:
	void FreeAllResources();

	void WriteEbmlDoc(const char* doc_type);
	void WriteSegmentInfo(const char* title, const char* app_name, double duration);

	void WriteTracks();
	void WriteTrackGlobal(Ebml::Master& track, Objects::Track* info, unsigned index);
	void WriteTrackCodec(Ebml::Master& track, Objects::Track* info);
	void WriteTrackAVMedia(Ebml::Master& track, Objects::Track* info);

	void WriteChapters();
	void WriteChapterAtom(Ebml::Master& chapter, Objects::Chapter* info, unsigned uid);

	void WriteTags();
	void WriteSimpleTag(Ebml::Master& tag, Objects::Tag* info);

	void WriteFiles();
	void WriteFile(Ebml::Master& file, Objects::Attachment* info);

	void WriteCues();
	void WriteSeekHead();
	void WriteSeekEntry(const unsigned* id, int64_t pos);

	bool WriteNewDuration(double duration);

	bool ProcessBlockHeader(const Sample& sample, void* header, bool key_frame);
	bool WriteSimpleBlock(const Sample& sample);
	bool WriteBlockGroup(const Sample& sample);
	bool WriteFrame(const Sample& sample);

	bool CheckNeedNewCluster(double offset_with_start);
	bool StartNewCluster(double time);
	void CloseCurCluster();

	unsigned FindTrackNumberByUniqueId(unsigned uid);

private:
	Core::IOCallback* _cb;
	Configs _cfg;

	Ebml::Master* _segment;
	int64_t _segment_offset;
	int64_t _seginfo_duration_offset;

	int64_t _offset_info;
	int64_t _offset_tracks;
	int64_t _offset_chapters;
	int64_t _offset_tags;
	int64_t _offset_files;
	int64_t _offset_cues;

	Ebml::Master* _cur_cluster;
	int64_t _cur_cluster_position;
	int32_t _cur_cluster_datasize;
	double _cur_cluster_time_start, _cur_cluster_time_offset;
	KeyFrameIndex* _cur_keyframe_index;

	TrackList _tracks;
	ChapterList _chapters;
	TagList _tags;
	AttachList _files;

	ExternTrackList _extern_tracks;
	ExternChapterList _extern_chapters;
	ExternTagList _extern_tags;
	ExternAttachList _extern_files;

	Core::StructTable<KeyFrameIndex, 10> _cues_index;
}; }

#endif //__AKA_MATROSKA_H