#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MKV_MEDIA_FORMAT_H
#define __MKV_MEDIA_FORMAT_H

#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include <IOReader/IOPoolReader.h>
#include <SimpleContainers/List.h>

#ifdef _SYENGINE_DEMUX
#include "Parser/MKVParser.h"
#include "Parser/MKVParserIO.h"
#else
#include <MKVParser.h>
#include <MKVParserIO.h>
#endif

#include "MKVMediaStream.h"

#define MEDIA_FORMAT_MKV_NAME "Matroska AV Container"
#define MEDIA_FORMAT_MKV_MIME "video/x-matroska"

#define MAX_MATROSKA_STREAM_COUNT 128

class MKVMediaFormat : 
	public IAVMediaFormatEx,
	public IAVMediaMetadata,
	public IAVMediaChapters,
	public IAVMediaResources,
	public MKVParser::IMKVParserIO {

public:
	MKVMediaFormat() :
		_av_io(nullptr), _stream_count(0), _read_flags(0), 
		_skip_av_others_streams(false), _force_avc1(false) { _force_io_pool_size = 0; }

public: //IAVMediaFormat
	AV_MEDIA_ERR Open(IAVMediaIO* io);
	AV_MEDIA_ERR Close();

	double GetDuration() { return _global_info.Duration; }
	double GetStartTime() { return 0.0; }
	unsigned GetBitRate() { return 0; }

	int GetStreamCount() { return (int)_stream_count; }
	IAVMediaStream* GetStream(int index)
	{ if (index >= (int)_stream_count) return nullptr; return _streams[index].get(); }

	unsigned GetFormatFlags()
	{ return MEDIA_FORMAT_CAN_SEEK_ALL|MEDIA_FORMAT_CAN_FLUSH; }
	const char* GetFormatName()
	{ return MEDIA_FORMAT_MKV_NAME; }
	const char* GetMimeType()
	{ return MEDIA_FORMAT_MKV_MIME; }

	void SetReadFlags(unsigned flags);
	void SetIoCacheSize(unsigned size)
	{ _force_io_pool_size = size; }

	AV_MEDIA_ERR Flush() { return AV_ERR_OK; }
	AV_MEDIA_ERR Reset();

	AV_MEDIA_ERR Seek(double seconds,bool key_frame,AVSeekDirection seek_direction);
	AV_MEDIA_ERR ReadPacket(AVMediaPacket* packet);
	AV_MEDIA_ERR OnNotifySeek();

	void* StaticCastToInterface(unsigned id);
	void Dispose() { delete this; }

public: //IAVMediaFormatEx
	int GetKeyFramesCount();
	unsigned CopyKeyFrames(double* copyTo,int copyCount);

public: //IAVMediaMetadata
	unsigned GetValueCount()
	{ return _parser->GetCore()->GetTags() != nullptr ? _parser->GetCore()->GetTags()->GetTagCount():0; }
	unsigned GetValue(const char* name,void* copyTo);
	const char* GetValueName(unsigned index);
	AVMetadataCoverImageType GetCoverImageType();
	unsigned GetCoverImage(unsigned char* copyTo);

public: //IAVMediaChapters
	unsigned GetChapterCount() { return _chaps.GetCount(); }
	AVMediaChapter* GetChapter(unsigned index)
	{ return index >= _chaps.GetCount() ? nullptr:_chaps.GetItem(index); }

public: //IAVMediaResources
	unsigned GetResourceCount() { return _resources.GetCount(); }
	AVMediaResource* GetResource(unsigned index)
	{ return index >= _resources.GetCount() ? nullptr:_resources.GetItem(index); }

public: //IMKVParserIO
	unsigned Read(void* buf,unsigned size);
	bool Seek(long long offset,int whence);
	long long Tell();
	long long GetSize();

private:
	bool MakeAllStreams(std::shared_ptr<MKVParser::MKVFileParser>& parser);
	unsigned FindMediaStreamFromTrackNumber(unsigned number);
	unsigned GetAllAudioStreamsBitrate(unsigned* first_video_index);

	void LoadMatroskaChapters(std::shared_ptr<MKVParser::MKVFileParser>& parser);
	void LoadMatroskaAttachFiles(std::shared_ptr<MKVParser::MKVFileParser>& parser);
	bool ProcessASS2SRT(MKVParser::MKVPacket* pkt);

private:
	IAVMediaIO* _av_io;
	std::shared_ptr<IOPoolReader> _io_pool;
	unsigned _force_io_pool_size;

	std::shared_ptr<MKVParser::MKVFileParser> _parser;
	MKVParser::MKVGlobalInfo _global_info;

	std::shared_ptr<MKVMediaStream> _streams[MAX_MATROSKA_STREAM_COUNT];
	unsigned _stream_count;

	unsigned _read_flags;
	bool _force_avc1;
	bool _skip_av_others_streams; //忽略字幕流
	MKV::Internal::Object::Cluster::Block::AutoBuffer _pktBuffer; //全局包缓冲区
	MKV::Internal::Object::Cluster::Block::AutoBuffer _assBuffer; //ASS2SRT 缓冲区

	AVUtils::List<AVMediaChapter> _chaps; //所有章节信息保存在这里
	AVUtils::List<AVMediaResource,1> _resources; //所有附件文件保存在这里
};

#endif //__MKV_MEDIA_FORMAT_H