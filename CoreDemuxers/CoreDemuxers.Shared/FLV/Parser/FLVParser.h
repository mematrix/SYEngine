/*
 - FLV Stream Parser (Packet Reader) -

 - Author: K.F Yang
 - Date: 2014-11-25
*/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __FLV_PARSER_H
#define __FLV_PARSER_H

#include "FLVSpec.h"
#include "FLVMetaSpec.h"

#include "FLVParserErrSpec.h"
#include "FLVParserIO.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <utility>

#define FLV_SWAP16(x) ((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))
#define FLV_SWAP32(x) ((((x) & 0xFF000000UL) >> 24) | (((x) & 0x00FF0000UL) >> 8) | (((x) & 0x0000FF00UL) << 8) | (((x) & 0x000000FFUL) << 24))

namespace FLVParser{

#define FLV_ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 4) //4bytes.

enum SupportAudioStreamType
{
	AudioStreamType_None = 0,
	AudioStreamType_MP3,
	AudioStreamType_AAC,
	AudioStreamType_PCM
};

enum SupportVideoStreamType
{
	VideoStreamType_None = 0,
	VideoStreamType_AVC, //H264
	VideoStreamType_VP6
};

enum SupportPacketType
{
	PacketTypeAudio = 0,
	PacketTypeVideo = 1
};

struct AAC_AVC_SPEC_INFO
{
	unsigned char* aac_spec_info; //AudioSpecificConfig. (2bytes)
	unsigned aac_info_size;
	unsigned char* avc_spec_info; //SPS and PPS. (AnnexB)
	unsigned avc_info_size;
	unsigned avc_profile; //H264 Profile
	unsigned avc_level;
	unsigned aac_profile; //audioObjectType
	unsigned char* avcc;
	unsigned avcc_size;
};

struct FLV_AUDIO_STREAM_INFO
{
	int raw_codec_id; //unknown is -1;
	int srate; //unknown is 0.
	int bits; //unknown is 0.
	int nch; //unknown is 0.
	int bitrate; //unknown is 0
};

struct FLV_VIDEO_STREAM_INFO
{
	int raw_codec_id; //unknown is -1;
	int width;
	int height;
	double fps;
	int bitrate; //unknown is 0
};

struct FLV_STREAM_GLOBAL_INFO
{
	double duration; //sec.
	SupportAudioStreamType audio_type;
	SupportVideoStreamType video_type;
	FLV_AUDIO_STREAM_INFO audio_info;
	FLV_VIDEO_STREAM_INFO video_info;
	AAC_AVC_SPEC_INFO delay_flush_spec_info; //Flush in ReadNextPacket.
	int no_keyframe_index;
	int no_audio_stream;
};

struct FLV_STREAM_PACKET
{
	SupportPacketType type;
	unsigned char* data_buf;
	unsigned data_size;
	unsigned timestamp; //ms. (pts)
	unsigned dts;
	int key_frame;
	int skip_this;
};

struct FLV_INTERNAL_KEY_FRAME_INDEX
{
	long long pos;
	double time;
};

#pragma pack(1)
struct FLV_TAG
{
	unsigned char type; //5bits
	unsigned char size[3]; //24bits
	unsigned char timestamp[4]; //24 + 8bits
	unsigned char sid[3]; //24bits
};

struct FLV_BODY_TAG
{
	unsigned prev_tag_size;
	FLV_TAG tag;
};
#pragma pack()

//****************************************

class FLVStreamParser final
{
public:
	FLVStreamParser(IFLVParserIO* pIo) throw();
	~FLVStreamParser() throw();

public:
	unsigned Open(bool update_duration_from_fileend = false);
	unsigned GetStreamInfo(FLV_STREAM_GLOBAL_INFO* info);

	unsigned Reset();
	unsigned Seek(double seconds,bool direction_next);
	unsigned ReadNextPacket(FLV_STREAM_PACKET* packet);
	unsigned ReadNextPacketFast(FLV_STREAM_PACKET* packet); //for Seek...

	void UpdateGlobalInfoH264();
	std::pair<FLV_INTERNAL_KEY_FRAME_INDEX*,unsigned> InternalGetKeyFrames()
	{ return {_keyframe_index,_keyframe_count}; }

	inline void SetOutputRawAVC1() throw()
	{ _output_avc1 = true; }
	inline bool IsOutputRawAVC1() const throw()
	{ return _output_avc1; }

private:
	unsigned ScanAllFrameNALU(unsigned char* psrc,unsigned char* pdst,unsigned len);

	bool ProcessAVCDecoderConfigurationRecord(unsigned char* pb);
	bool ProcessAACAudioSpecificConfig(unsigned char* pb,unsigned size);

	double SearchDurationFromLastTimestamp();

private:
	IFLVParserIO* _flv_io;

	unsigned _flv_head_offset,_flv_data_offset;

	FLV_STREAM_GLOBAL_INFO _global_info;

	FLV_STREAM_PACKET _pkt_buf_audio;
	FLV_STREAM_PACKET _pkt_buf_video;

	FLV_INTERNAL_KEY_FRAME_INDEX* _keyframe_index; //use free to Free memory.
	unsigned _keyframe_count;

	unsigned _lengthSizeMinusOne;
	bool _output_avc1;

private:
	struct AutoBuffer
	{
		void* ptr;
		unsigned alloc_size;
		unsigned true_size;

		bool Alloc(unsigned size) throw()
		{
			if (size > alloc_size)
			{
				Free();

				ptr = malloc(FLV_ALLOC_ALIGNED(size) + 8);
				alloc_size = true_size = size;
			}else{
				true_size = size;
			}

			if (ptr)
				memset(ptr,0,true_size);

			return ptr != nullptr;
		}

		void Free() throw()
		{
			if (ptr)
				free(ptr);

			ptr = nullptr;
			alloc_size = true_size = 0;
		}

		inline void ResetSize() throw() { true_size = 0; }
		inline unsigned Size() throw() { return true_size; }

		template<typename T = void>
		T* Get() const throw()
		{
			return (T*)ptr;
		}

		AutoBuffer() throw() : ptr(nullptr) { Free(); }
		AutoBuffer(unsigned size) throw() : ptr(nullptr) { Free(); Alloc(size); }
		~AutoBuffer() throw() { Free(); }
	};

	AutoBuffer _bodyBuffer;
	AutoBuffer _pktAudioBuffer,_pktVideoBuffer;
};

} //namespace FLVParser.

#endif //__FLV_PARSER_H