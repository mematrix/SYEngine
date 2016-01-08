#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MP4_MEDIA_FORMAT_H
#define __MP4_MEDIA_FORMAT_H

#include <shared_memory_ptr.h>
#include <MediaAVFormat.h>
#include <IOReader/IOPoolReader.h>
#include <SimpleContainers/List.h>

#include "Core/Aka4Splitter.h"
#include "MP4MediaStream.h"

#define MEDIA_FORMAT_MP4_NAME "MPEG4 ISO Base Media"
#define MEDIA_FORMAT_MP4_MIME "video/mp4"

#define MAX_MP4_STREAM_COUNT 64

class MP4MediaFormat : 
	public IAVMediaFormatEx,
	public IAVMediaChapters,
	public IAVMediaMetadata,
	public Isom::IStreamSource {

public:
	MP4MediaFormat() : _av_io(nullptr), _stream_count(0),
		_kf_count(0), _force_avc1(false), _is_m4a_file(false) { _force_io_pool_size = 0; }

public: //IAVMediaFormat
	AV_MEDIA_ERR Open(IAVMediaIO* io);
	AV_MEDIA_ERR Close();

	double GetDuration()
	{ return _core == nullptr ? 0.0:_core->GetGlobalInfo()->Duration; }
	double GetStartTime() { return 0.0; }
	unsigned GetBitRate() { return 0; }

	int GetStreamCount() { return _stream_count; }
	IAVMediaStream* GetStream(int index)
	{ if (index >= (int)_stream_count) return nullptr; return _streams[index].get(); }

	unsigned GetFormatFlags()
	{ return MEDIA_FORMAT_CAN_SEEK_ALL|MEDIA_FORMAT_CAN_FLUSH; }
	char* GetFormatName()
	{ return MEDIA_FORMAT_MP4_NAME; }
	char* GetMimeType()
	{ return MEDIA_FORMAT_MP4_MIME; }

	AV_MEDIA_ERR Flush() { return AV_ERR_OK; }
	AV_MEDIA_ERR Reset() { return Seek(0.0,false,SeekDirection_Auto); }

	AV_MEDIA_ERR Seek(double seconds,bool key_frame,AVSeekDirection seek_direction);
	AV_MEDIA_ERR ReadPacket(AVMediaPacket* packet);

	void SetReadFlags(unsigned flags)
	{ _force_avc1 = (flags == MEDIA_FORMAT_READER_H264_FORCE_AVC1); }
	void SetIoCacheSize(unsigned size)
	{ _force_io_pool_size = size; }

	void* StaticCastToInterface(unsigned id);
	void Dispose() { delete this; }

public: //IAVMediaFormatEx
	int GetKeyFramesCount();
	unsigned CopyKeyFrames(double* copyTo,int copyCount);

public: //IAVMediaMetadata
	unsigned GetValueCount()
	{ return _core != nullptr ? _core->InternalGetCore()->GetTags()->GetCount():0; }
	unsigned GetValue(const char* name,void* copyTo);
	const char* GetValueName(unsigned index);
	AVMetadataCoverImageType GetCoverImageType();
	unsigned GetCoverImage(unsigned char* copyTo);

public: //IAVMediaChapters
	unsigned GetChapterCount() { return _chaps.GetCount(); }
	AVMediaChapter* GetChapter(unsigned index)
	{ return index >= _chaps.GetCount() ? nullptr:_chaps.GetItem(index); }

public: //Isom::IStreamSource
	unsigned Read(void* buf,unsigned size)
	{ return (unsigned)_av_io->Read(buf,size); }
	bool Seek(long long pos)
	{ return _av_io->Seek(pos,SEEK_SET); }
	long long Tell()
	{ return _av_io->Tell(); }
	long long GetSize()
	{ return _av_io->GetSize(); }
	bool IsFromNetwork()
	{ return _av_io->IsAliveStream(); }

private:
	bool MakeAllStreams(std::shared_ptr<Aka4Splitter>& core);
	unsigned FindStreamIndexFromId(unsigned id);

	void LoadAllChapters();

private:
	IAVMediaIO* _av_io;
	std::shared_ptr<IOPoolReader> _io_pool;
	unsigned _force_io_pool_size;

	std::shared_ptr<Aka4Splitter> _core;
	std::shared_ptr<MP4MediaStream> _streams[MAX_MP4_STREAM_COUNT];
	unsigned _stream_count;

	AVUtils::Buffer _pkt_buffer;
	AVUtils::List<AVMediaChapter> _chaps;

	unsigned _kf_count;
	bool _is_m4a_file;

	bool _force_avc1;
};

#endif //__MP4_MEDIA_FORMAT_H