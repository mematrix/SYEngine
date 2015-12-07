/*
 - Matroska File Parser -

 - Author: K.F Yang
 - Date: 2015-01-10
*/

#ifndef _MKV_PARSER_H
#define _MKV_PARSER_H

#include <cstdio>
#include <cstdint>
#include <memory>

#include "MKVParserIO.h"
#include "MKVParserErrSpec.h"

#include "Internal/MatroskaDataSource.h"
#include "Internal/MatroskaEbmlDoc.h"
#include "Internal/MatroskaInternal.h"
#include "Internal/MatroskaCodecIds.h"

#include "Duration/DurationCalcInterface.h"
#include "Duration/VorbisCalcEngine.h"
#include "Duration/OpusCalcEngine.h"
#include "Duration/FlacCalcEngine.h"
#include "Duration/AlacCalcEngine.h"
#include "Duration/PcmCalcEngine.h"

#include "WV4Help.h"
#include "TTA1Help.h"

#ifndef _MKV_PARSER_NOUSE_ZLIB
#include "zlib/zlib.h"
#endif

#define MKV_TIMESTAMP_UNIT_NANOSEC 1000000000.0

#define WAVPACK4_BLOCK_HEADER_SIZE 32
#define WAVPACK4_BLOCK_HEADER_OFFSET 12
#define WAVPACK4_BLOCK_FIXED_FRAME_SIZE (WAVPACK4_BLOCK_HEADER_SIZE - WAVPACK4_BLOCK_HEADER_OFFSET)

namespace MKVParser
{
	struct MKVGlobalInfo
	{
		enum MKVContainerType
		{
			Matroska,
			WebM,
			Unknown
		};
		MKVContainerType Type;
		double Duration;
		double TimecodeScale;
		char Application[256];
		bool NonKeyFrameIndex;
	};

	struct MKVTrackInfo
	{
		MKV::Internal::Object::Context::TrackEntry* InternalEntry;
		double InternalFrameDuration;
		bool InternalNonDefaultDuration;
		IDurationCalc* InternalDurationCalc;

		enum MKVTrackType
		{
			TrackTypeUnknown,
			TrackTypeAudio,
			TrackTypeVideo,
			TrackTypeSubtitle
		};
		MKVTrackType Type;
		unsigned Number;
		bool Default;
		char LangID[32];
		char* Name; //UTF8

		struct CodecInfo
		{
			MatroskaCodecIds CodecType;
			char CodecID[64];
			unsigned char* CodecPrivate;
			unsigned CodecPrivateSize;
			double CodecDelay, SeekPreroll; //seconds

			bool InternalCopyCodecPrivate(unsigned char* src,unsigned size) throw()
			{
				if (size > INT_MAX)
					return false;
				if (CodecPrivate)
					return false;

				CodecPrivate = (unsigned char*)malloc(MKV_ALLOC_ALIGNED(size));
				if (!CodecPrivate)
					return false;

				memcpy(CodecPrivate,src,size);
				CodecPrivateSize = size;
				return true;
			}

			void InternalFree() throw()
			{
				if (CodecPrivate)
				{
					free(CodecPrivate);
					CodecPrivate = nullptr;
					CodecPrivateSize = 0;
				}
			}
		};
		CodecInfo Codec;

		struct AudioInfo
		{
			unsigned Channels;
			unsigned SampleRate;
			unsigned BitDepth;
		};
		AudioInfo Audio;
		struct VideoInfo
		{
			unsigned Width;
			unsigned Height;
			unsigned DisplayWidth, DisplayHeight;
			float FrameRate;
			bool Interlaced;
		};
		VideoInfo Video;

		enum TrackCompressType
		{
			TrackCompressNone = 0,
			TrackCompressStripHeader,
			TrackCompressZLIB
		};
		TrackCompressType CompressType;
		bool IsZLibCompressed() const throw() { return CompressType == TrackCompressZLIB; }

		bool InternalChangeName(char* name) throw()
		{
			if (name == nullptr)
				return false;
			if (strlen(name) == 0)
				return true;

			if (Name)
				free(Name);
			Name = strdup(name);
			return Name != nullptr;
		}

		void InternalFree() throw()
		{
			Codec.InternalFree();
			if (Name)
			{
				free(Name);
				Name = nullptr;
			}

			if (InternalDurationCalc != nullptr)
			{
				InternalDurationCalc->DeleteMe();
				InternalDurationCalc = nullptr;
			}
		}
	};

	struct MKVKeyFrameIndex //for Internal.
	{
		unsigned TrackNumber;
		double Time; //seconds
		bool SubtitleTrack; //字幕轨道也可以用做关键帧索引
		long long ClusterPosition; //abs of file.
		unsigned BlockNumber; //unused
	};

	struct MKVPacket
	{
		unsigned Number; //MKVTrackInfo::Number.
		unsigned char* PacketData;
		unsigned PacketSize;
		double Timestamp; //seconds.
		double Duration;
		bool KeyFrame;
		bool SkipThis;
		bool NoTimestamp; //if to get Duration. (TrackDefaultDuration is none.)
		long long ClusterPosition; //当前Packet在哪个Cluster里面，绝对值offset by file。
	};

	class MKVFileParser final : public MKV::DataSource::IDataSource
	{
	public:
		typedef MatroskaList<MKVTrackInfo> MKVTrackList;
		typedef MatroskaList<MKVKeyFrameIndex,100> MKVKeyFramesList;

		explicit MKVFileParser(IMKVParserIO* io) throw();
		~MKVFileParser() throw() {}

	public:
		unsigned Open(bool ignore_duratin_zero = false,bool do_not_parse_seekhead = false) throw();
		bool Close() throw();

		bool GetGlobalInfo(MKVGlobalInfo* info) throw();

		unsigned GetTrackCount() throw() { return _tracks.GetCount(); }
		bool GetTrackInfo(unsigned index,MKVTrackInfo* info) throw();

		unsigned Reset() throw();
		unsigned Seek(double seconds,bool fuzzy = true,unsigned base_on_track_number = 0xFFFFFFFF) throw();
		unsigned ReadPacket(MKVPacket* packet,bool fast = false) throw();

		unsigned OnExternalSeek() throw();
		unsigned ReadSinglePacket(MKVPacket* packet,unsigned number,bool key_frame = true,bool auto_reset = true) throw();

		bool IsMatroskaAudioOnly() const throw() { return _is_mka_file; }

		const MKV::Internal::MatroskaSegment* GetCore() const throw() { return _mkv.get(); }
		inline MKVTrackList* InternalGetTracks() throw() { return &_tracks; }
		inline MKVKeyFramesList* InternalGetKeyFrames(unsigned* defaultSeekTrackNum = nullptr) throw()
		{ if (defaultSeekTrackNum) *defaultSeekTrackNum = _default_seek_tnum;  return &_keyframe_index; }

	public: //IDataSource
		int Read(void* buffer,unsigned size);
		int Seek(long long offset,int whence);
		long long Tell();
		long long Size();

	private:
		void InitGlobalInfo(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw();
		bool InitTracks(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw();
		bool InitKeyFrameIndex(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw();
		double InitDurationFromTags(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw();
		double InitDurationFromKeyFrameIndex() throw();

		bool CheckMatroskaAudioOnly() throw();
		void MakeKeyFramesIndex() throw();

		bool ConvertTrackEntry2MKVTrackInfo(MKV::Internal::Object::Context::TrackEntry* entry,MKVTrackInfo* info) throw();
		double CalcAudioTrackDefaultDuration(MKVTrackInfo* info) throw();
		void InitDurationCalcEngine(MKVTrackInfo* info) throw();

		inline bool IsTrackTypeSupported(unsigned type) const throw()
		{
			if (type == MKV_TRACK_TYPE_VIDEO ||
				type == MKV_TRACK_TYPE_AUDIO ||
				type == MKV_TRACK_TYPE_SUBTITLE)
				return true;
			return false;
		}

		bool ParseNext() throw();
		MKVTrackInfo* FindTrackInfo(unsigned TrackNumber) throw();

		inline double CalcTimestamp(long long timecode) throw()
		{
			//MKV的单位是纳秒。
			return (double)timecode * _info.TimecodeScale * 0.000000001;
		}

		inline unsigned ConvertTrackNumber(unsigned trackNum) const throw()
		{
			//某些MKV的Track号码不是从1开始的。。。
			if (!_track_num_start_with_zero)
				return trackNum - 1;
			return trackNum;
		}

	private:
		MKVParser::IMKVParserIO* _io;
		std::shared_ptr<MKV::Internal::MatroskaSegment> _mkv;

		MKVGlobalInfo _info;
		MKVTrackList _tracks;
		MKVKeyFramesList _keyframe_index;

		long long _firstClusterSize;
		unsigned _default_seek_tnum;

		long long _resetPointer;
		unsigned _segmentOffset;

		bool _is_mka_file;
		bool _track_num_start_with_zero;
		bool _seek_after_flag;

		struct BlockInfo
		{
			MKV::Internal::Object::Cluster::Block* block;
			unsigned count;
			unsigned now;
			long long nextClusterSize;
			double timestampLaced;

			inline void Empty(long long size = 0) throw()
			{
				timestampLaced = 0.0;
				nextClusterSize = size;
				count = now = 0;
				block = nullptr;
			}

			inline bool IsEmpty() const throw() { return block == nullptr; }
			inline bool IsFull() const throw() { return count == now; }
			inline void SetFull() throw() { count = now = 0; } //如果又分帧又没默认长度，只能copy全部帧
			//如果是Lace的Block，但是又没有Duration信息，则通过Duration分析器分析出
			//一帧的Duration后，使用timestampLaced变量进行递增，packet的Timestamp+=timestampLaced
			inline double GetIncrementDuration() const throw() { return timestampLaced; }
			inline void AddTimestampDuration(double duration) throw() { timestampLaced += duration; }

			inline decltype(block) GetBlock() const throw() { return block; }
			void Next(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw();
		};
		BlockInfo _frame;
		MKV::Internal::Object::Cluster::Block::AutoBuffer _frameBuffer;

	private:
		bool static zlibDecompress(unsigned char* inputData,unsigned inputSize,unsigned char** outputData,unsigned* outputSize);
	};
}

#endif //_MKV_PARSER_H