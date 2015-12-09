/* ---------------------------------------------------------------
 -
 - The Aka4Splitter : Simple library to parses MP4 files.
 - Aka4Splitter.h

 - MIT License
 - Copyright (C) 2015 ShanYe
 - Rev: 1.0.0
 -
 --------------------------------------------------------------- */

#ifndef _AKA4_SPLITTER_H
#define _AKA4_SPLITTER_H

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

#include "Internal/GlobalPrehead.h"
#include "Internal/IsomParser.h"

static const double Aka4_DataPacket_InvalidTimestamp = -1.0;

class Aka4Splitter
{
public:
	struct GlobalInfo
	{
		const char* Bands;
		double Duration; //seconds.
	};

	struct TrackInfo
	{
		enum TrackType
		{
			TrackType_Unknown = 0,
			TrackType_Audio,
			TrackType_Video,
			TrackType_Subtitle
		};
		TrackType Type;
		unsigned Id;
		bool Enabled;
		char LangId[16];
		char* Name; //UTF8
		unsigned BitratePerSec;

		struct CodecInfo
		{
			enum CodecStoreType
			{
				StoreType_Default, //such as avc1, hvc1, alac...
				StoreType_Esds, //such as mp4a, mp4v...
				StoreType_SampleEntry, //unknown sample-entry.
				StoreType_SampleEntryWithEsds //unknown esds sample-entry.
			};
			CodecStoreType StoreType;
			union u
			{
				unsigned CodecFcc; // -> Default, == ISOM_FCC('alac')...
				unsigned EsdsObjType; // -> Esds -> EsdsObjTypes, map table with EsdsObjTypes.
			};
			u CodecId;

			unsigned char* Userdata; //avcc, hvcc, adts...
			unsigned UserdataSize;

			bool InternalCopyUserdata(const unsigned char* data, unsigned size) throw()
			{
				if (data == nullptr || size == 0 || size > INT32_MAX)
					return false;
				if (Userdata)
					return false;
				Userdata = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(size));
				if (!Userdata)
					return false;
				memcpy(Userdata, data, size);
				UserdataSize = size;
				return true;
			}

			void InternalFree() throw()
			{
				if (Userdata)
					free(Userdata);
				Userdata = nullptr;
				UserdataSize = 0;
			}
		};
		CodecInfo Codec;

		struct AudioInfo
		{
			unsigned Channels;
			unsigned SampleRate;
			unsigned BitDepth;
			bool QuickTimeFlag;
		};
		AudioInfo Audio;
		struct VideoInfo
		{
			unsigned Width, Height;
			unsigned DisplayWidth, DisplayHeight;
			unsigned Rotation;
			float FrameRate;
		};
		VideoInfo Video;

		Isom::Parser::Track* InternalTrack;
		double InternalTimeScale;
		double InternalStartTimeOffset;
		unsigned InternalCurrentSample;

		bool InternalChangeName(const char* name) throw()
		{
			if (name == nullptr)
				return false;
			if (strlen(name) == 0)
				return false;
			if (Name)
				free(Name);
			Name = strdup(name);
			return Name != nullptr;
		}

		void InternalFree() throw()
		{
			Codec.InternalFree();
			if (Name)
				free(Name);
			Name = nullptr;
		}
	};

	struct DataPacket
	{
		unsigned Id; //TrackInfo -> Id.
		unsigned char* Data;
		unsigned DataSize;
		double PTS, DTS; //seconds. -1.0 is invalid.
		double Duration; //seconds. -1.0 is invalid.
		bool KeyFrame;
		int64_t InFilePosition;
	};

	typedef Isom::StructTable<TrackInfo> Tracks;
	typedef Isom::ReadonlyStructTable<TrackInfo> ExternTracks;

public:
	explicit Aka4Splitter(Isom::IStreamSource* data) throw() : _stm(data), _extern_tracks(&_tracks)
	{ _pkt_buffer = nullptr; _pkt_buf_size = 0; _extern_pkt_buf = nullptr; }
	~Aka4Splitter() throw() { Close(); }

public:
	bool Open(unsigned* isom_err_code = nullptr, bool alloc_max_pkt_buf = false);
	void Close();

	const GlobalInfo* GetGlobalInfo() { return &_info; }
	ExternTracks* GetTracks() { return &_extern_tracks; }

	enum SeekMode
	{
		SeekMode_Reset,
		SeekMode_Prev,
		SeekMode_Next
	};
	bool Seek(double seconds, SeekMode mode, bool keyframe = false);
	bool Reset()
	{ return Seek(0.0,SeekMode_Reset); }
	bool ReadPacket(DataPacket* pkt);

	unsigned InternalGetMaxPacketBufferSize() throw();
	inline void InternalSetExternalPacketBuffer(unsigned char* buf) throw() { _extern_pkt_buf = buf; }
	inline unsigned InternalGetBestSeekTrackIndex() const throw() { return _best_seek_track_index; }
	inline Isom::Parser* InternalGetCore() throw() { return _parser.get(); }

public:
	static int QueryDefaultAudioTrackIndex(Aka4Splitter* splitter) throw();
	static int QueryDefaultVideoTrackIndex(Aka4Splitter* splitter) throw();

private:
	bool InitGlobalInfo(const std::shared_ptr<Isom::Parser>& parser) throw();
	bool InitTracks(const std::shared_ptr<Isom::Parser>& parser) throw();

	bool FindNextSample(unsigned* track_index, unsigned* next_index);
	bool SeekAllStreams(double seconds, bool prev_or_next, bool key_frame);
	bool SeekSingleTrack(unsigned index, double seconds, bool prev_or_next, bool key_frame);

	bool AllocPacketBuffer(unsigned size);
	bool PreallocMaxPacketBuffer();

	int64_t GetLongestTrackDuration(unsigned* index) throw();
	unsigned FindBestSeekTrackIndex() throw();

	bool CheckApplyStartTimeOffset(TrackInfo* ti) throw();
	bool ConvertIsomTrackToNative(Isom::Parser::Track* isom, TrackInfo* ti) throw();
	inline unsigned ConvertTrackId(unsigned id) const throw()
	{ return _track_num_start_with_zero ? id:id - 1; }

private:
	Isom::IStreamSource* _stm;
	std::shared_ptr<Isom::Parser> _parser;

	GlobalInfo _info;
	Tracks _tracks;
	ExternTracks _extern_tracks;

	bool* _process_tracks_start_time_offset;
	bool _track_num_start_with_zero;
	unsigned _best_seek_track_index;

	unsigned char* _extern_pkt_buf;

	unsigned char* _pkt_buffer;
	unsigned _pkt_buf_size;

	int64_t _sample_prev_filepos; //debug...

public:
	enum EsdsObjTypes
	{
		EsdsObjType_Invalid = 0,
		EsdsObjType_Text = 0x08,
		EsdsObjType_Video_MPEG4 = 0x20,
		EsdsObjType_Video_H264 = 0x21,
		EsdsObjType_Video_HEVC = 0x23,
		EsdsObjType_Audio_AAC = 0x40, //LC, SBR etc.
		EsdsObjType_Video_MPEG2_Simple = 0x60,
		EsdsObjType_Video_MPEG2_Main = 0x61,
		EsdsObjType_Video_MPEG2_SNR = 0x62,
		EsdsObjType_Video_MPEG2_Spatial = 0x63,
		EsdsObjType_Video_MPEG2_High = 0x64,
		EsdsObjType_Video_MPEG2_422 = 0x65,
		EsdsObjType_Audio_AAC_MPEG2_Main = 0x66,
		EsdsObjType_Audio_AAC_MPEG2_Low = 0x67,
		EsdsObjType_Audio_AAC_MPEG2_SSR = 0x68,
		EsdsObjType_Audio_MPEG2 = 0x69,
		EsdsObjType_Video_MPEG1 = 0x6A,
		EsdsObjType_Audio_MPEG3 = 0x6B,
		EsdsObjType_Video_MJPEG = 0x6C,
		EsdsObjType_Image_PNG = 0x6D,
		EsdsObjType_Image_JP2K = 0x6E,
		EsdsObjType_Video_VC1 = 0xA3,
		EsdsObjType_Video_Dirac = 0xA4,
		EsdsObjType_Audio_AC3 = 0xA5,
		EsdsObjType_Audio_EAC3 = 0xA6,
		EsdsObjType_Audio_DRA = 0xA7,
		EsdsObjType_Audio_G719 = 0xA8,
		EsdsObjType_Audio_DTS = 0xA9,
		EsdsObjType_Audio_DTS_HD = 0xAA,
		EsdsObjType_Audio_DTS_HD_Master = 0xAB,
		EsdsObjType_Audio_DTS_Express = 0xAC,
		EsdsObjType_Audio_Opus = 0xAD,
		EsdsObjType_Audio_Vorbis = 0xDD,
		EsdsObjType_Audio_Qcelp = 0xE1
	};
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif //_AKA4_SPLITTER_H