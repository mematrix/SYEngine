#ifndef __ISOM_PARSER_H_
#define __ISOM_PARSER_H_

#include "IsomGlobal.h"
#include "IsomObject.h"
#include "IsomTypes.h"
#include "IsomError.h"
#include "IsomCodec.h"
#include "IsomStreamSource.h"
#include "StructTable.h"

namespace Isom {

class Parser
{
public:
	explicit Parser(IStreamSource* stm) throw() : _stream(stm) {}
	~Parser() throw() { Free(); }

public:
	struct Header
	{
		Object::FileHeader File;
		Object::MovieHeader Movie;
	};
	struct Track
	{
		enum TrackType
		{
			TrackType_Unknown = 0,
			TrackType_Audio,
			TrackType_Video,
			TrackType_Subtitle
		};
		TrackType Type;
		char HandlerName[512];
		Object::TrackHeader Header;
		Object::MediaHeader MediaInfo;
		char* LocationUrl; //null is local file.

		int16_t AudioBalance; //smhd
		int16_t VideoGraphicsMode; //vmhd
		int16_t VideoOpColor[3]; //vmhd
		unsigned VideoRotation; //tkhd

		bool MultiStsdDesc;
		bool KeyFrameExists;
		Object::Internal::IndexObject* Samples;
		unsigned SampleCount;
		float TimeStartOffset; //dts, frm elst, encoder delay.

		int64_t TotalStreamSize;
		int64_t TotalDuration;
		unsigned TotalFrames;

		CodecSampleEntryParser* CodecInfo;
		unsigned CodecTag;
		unsigned char* CodecSampleEntry;
		unsigned CodecSampleEntrySize;

		void FreeResources() throw()
		{
			Temp.FreeResources();
			if (CodecInfo)
				delete CodecInfo;
			CodecInfo = nullptr;

			if (CodecSampleEntry)
				free(CodecSampleEntry);
			CodecSampleEntry = nullptr;
			CodecSampleEntrySize = 0;

			if (LocationUrl)
				free(LocationUrl);
			LocationUrl = nullptr;

			if (Samples)
				free(Samples);
			Samples = nullptr;
			SampleCount = 0;
		}

		struct Internal
		{
			int64_t* chunkOffsets; //chunk in file table.
			unsigned chunkCount;
			Object::Internal::sttsObject* stts; //dts to sample table.
			unsigned sttsCount;
			Object::Internal::sttsObject* ctts; //dts to pts table.
			unsigned cttsCount;
			Object::Internal::stscObject* stsc; //sample to chunk table.
			unsigned stscCount;
			unsigned* stss; //key frames index table.
			unsigned* stps; //key frames for mpeg2 open gop.
			unsigned stssCount, stpsCount;
			unsigned* stsz; //sample size table.
			unsigned stszCount;
			unsigned stszSampleSize;
			
			Object::Internal::elstObject* elst;
			unsigned elstCount;

			void FreeResources() throw()
			{
				if (chunkOffsets)
					free(chunkOffsets);
				chunkOffsets = nullptr;
				chunkCount = 0;

				if (stts)
					free(stts);
				stts = nullptr;
				sttsCount = 0;

				if (ctts)
					free(ctts);
				ctts = nullptr;
				cttsCount = 0;

				if (stsc)
					free(stsc);
				stsc = nullptr;
				stscCount = 0;

				if (stss)
					free(stss);
				if (stsz)
					free(stsz);
				if (stps)
					free(stps);
				stss = stsz = stps = nullptr;
				stssCount = stszCount = stpsCount = 0;

				if (elst)
					free(elst);
				elst = nullptr;
				elstCount = 0;
			}
		};
		Internal Temp;
	};
	typedef StructTable<Track> TrackList;

	struct Chapter
	{
		double Timestamp;
		char* Title;

		void FreeResources() throw()
		{
			if (Title)
				free(Title);
			Title = nullptr;
		}
	};
	typedef StructTable<Chapter> ChapList;
	struct Tag
	{
		unsigned Type;
		const char* Name;
		unsigned char* DataPointer;
		char* StringPointer; //if this != null, tag is string.
		unsigned Length;

		void FreeResources() throw()
		{
			if (DataPointer)
				free(DataPointer);
			DataPointer = nullptr;
			StringPointer = nullptr;
			Length = 0;
		}
	};
	typedef StructTable<Tag> TagList;

public:
	unsigned Parse() throw();
	void Free() throw();

	inline Header* GetHeader() throw() { return &_header; }
	inline TrackList* GetTracks() throw() { return &_tracks; }
	inline ChapList* GetChapters() throw() { return &_chaps; }
	inline TagList* GetTags() throw() { return &_tags; }

private:
	bool IsContainerBox(unsigned box_type);
	unsigned ParseContainer(Object::Internal::BoxObject* parent, int depth, unsigned head_offset_size = 0);

	unsigned OnProcessContainer(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned OnProcessContainerEnded(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);

	unsigned OnProcessContent(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned OnProcessSkipped(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);

	unsigned OnMetadataBox(Object::Internal::BoxObject* box, bool itunes);

	double CalcStartTimeOffsetFromElst();
	unsigned CalcSampleCountFromChunks(int stsd_index = -1);
	unsigned BuildTrackIndexTable();
	unsigned SaveTrackSampleEntry(Object::Internal::BoxObject* box);
	bool GetBoxVersionAndFlags(int* version, int* flags);

public:
	unsigned Internal_Process_ftyp(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_mvhd(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_tkhd(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_mdhd(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_hdlr(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_stts(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_stss(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_stsz(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_stco(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_stsc(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_ctts(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_elst(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_chpl(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_url(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_smhd(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_vmhd(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_uuid(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_wide(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);
	unsigned Internal_Process_trak(Object::Internal::BoxObject* box, Object::Internal::BoxObject* parent);

private:
	bool StmReadUI64(uint64_t* value);
	bool StmReadUI32(uint32_t* value);
	bool StmReadUI24(uint32_t* value);
	bool StmReadUI16(uint32_t* value);
	bool StmReadUI08(uint32_t* value);
	bool StmReadSkip(unsigned size);

private:
	IStreamSource* _stream;
	bool _found_moov,  _found_mdat;

	Track* _current_track;
	Header _header;
	TrackList _tracks;
	ChapList _chaps;
	TagList _tags;
};

}

#endif //__ISOM_PARSER_H_