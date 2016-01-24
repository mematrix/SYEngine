#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __FLV_MEDIA_FORMAT_H
#define __FLV_MEDIA_FORMAT_H

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <memory>

#include <shared_memory_ptr.h>
#include <MediaAVFormat.h>
#include <IOReader/IOPoolReader.h>

#include <AAC/AACAudioDescription.h>
#include <MP3/MPEGAudioDescription.h>
#include <H264/X264VideoDescription.h>
#include <H264/AVC1VideoDescription.h>
#include <CommonAudioDescription.h>
#include <CommonVideoDescription.h>

#include "Parser/FLVParser.h"

#include "FLVMediaStream.h"

#define MEDIA_FORMAT_FLV_NAME "Adobe Flash Video"
#define MEDIA_FORMAT_FLV_MIME "video/x-flv"

class FLVMediaFormat : public IAVMediaFormatEx, public FLVParser::IFLVParserIO
{
public:
	FLVMediaFormat() : _av_io(nullptr), _opened(false), _force_avc1(false)
	{
		_force_io_pool_size = 0;

		_keyframe_index = nullptr;
		_keyframe_count = 0;

		_frame_duration = PACKET_NO_PTS;
		_sound_duration = PACKET_NO_PTS;
	}

public: //IAVMediaFormat
	AV_MEDIA_ERR Open(IAVMediaIO* io);
	AV_MEDIA_ERR Close();

	double GetDuration();
	double GetStartTime();
	unsigned GetBitRate();

	int GetStreamCount();
	IAVMediaStream* GetStream(int index);

	unsigned GetFormatFlags();
	char* GetFormatName();
	char* GetMimeType();

	AV_MEDIA_ERR Flush();
	AV_MEDIA_ERR Reset();

	AV_MEDIA_ERR Seek(double seconds,bool key_frame,AVSeekDirection seek_direction);
	AV_MEDIA_ERR ReadPacket(AVMediaPacket* packet);

	void SetReadFlags(unsigned flags)
	{ _force_avc1 = (flags == MEDIA_FORMAT_READER_H264_FORCE_AVC1); }
	void SetIoCacheSize(unsigned size)
	{ _force_io_pool_size = size; }

	void* StaticCastToInterface(unsigned id)
	{ if (id == AV_MEDIA_INTERFACE_ID_CASE_EX) return static_cast<IAVMediaFormatEx*>(this); return nullptr; }
	void Dispose() { delete this; }

public: //IAVMediaFormatEx
	int GetKeyFramesCount();
	unsigned CopyKeyFrames(double* copyTo,int copyCount);

public: //IFLVParserIO
	int Read(void*,int);
	bool Seek(long long);
	bool SeekByCurrent(long long);
	long long GetSize();

private:
	unsigned InternalInitStreams(std::shared_ptr<FLVParser::FLVStreamParser>& parser);
	bool MakeAllStreams(std::shared_ptr<FLVParser::FLVStreamParser>& parser);

	bool FindAndMakeKeyFrames();
	AV_MEDIA_ERR InternalSeek(double seconds);

private:
	bool _opened;
	IAVMediaIO* _av_io;
	std::unique_ptr<IOPoolReader> _io_pool;
	unsigned _force_io_pool_size;

	std::shared_ptr<FLVParser::FLVStreamParser> _parser;
	FLVParser::FLV_STREAM_GLOBAL_INFO _stream_info;

	std::shared_ptr<IAVMediaStream> _audio_stream;
	std::shared_ptr<IAVMediaStream> _video_stream;
	int _stream_count;

	//如果Parser没有这个索引，那在这儿自己建立
	FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX* _keyframe_index;
	unsigned _keyframe_count;

	bool _skip_unknown_stream;
	bool _force_avc1;
	double _frame_duration,_sound_duration;
};

#endif //__FLV_MEDIA_FORMAT_H