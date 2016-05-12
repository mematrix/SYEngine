#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MKV_MEDIA_STREAM_H
#define __MKV_MEDIA_STREAM_H

#include <shared_memory_ptr.h>
#include <MediaAVFormat.h>

#include <AAC/AACAudioDescription.h>
#include <MP3/MPEGAudioDescription.h>
#include <OGG/VorbisAudioDescription.h>
#include <FLAC/FLACAudioDescription.h>
#include <ALAC/ALACAudioDescription.h>
#include <AC3/AC3AudioDescription.h>
#include <H264/X264VideoDescription.h>
#include <H264/AVC1VideoDescription.h>
#include <MPEG2/MPEG2VideoDescription.h>
#include <MPEG4/MPEG4VideoDescription.h>
#include <VC1/VC1VideoDescription.h>
#include <CommonAudioDescription.h>
#include <CommonVideoDescription.h>

#include "Parser/MKVParser.h"
#include "RealCook.h"

class MKVMediaStream : public IAVMediaStream
{
public:
	MKVMediaStream() = delete;
	explicit MKVMediaStream(MKVParser::MKVTrackInfo* info);

public:
	MediaMainType GetMainType() { return _main_type; }
	MediaCodecType GetCodecType() { return _codec_type; }

	MediaSubtitleStreamInfo* GetSubtitleInfo() { return &_subtitle_info; }

	int GetStreamIndex() { return _stream_index; }
	const char* GetStreamName();
	const char* GetLanguageName();

	float GetContainerFps() { return _fps; }

	IAudioDescription* GetAudioInfo();
	IVideoDescription* GetVideoInfo();

public:
	//for Subtitle ASS to SRT.
	inline void ForceChangeCodecType(MediaCodecType new_ct) throw() { _codec_type = new_ct; }

	inline double GetFrameDuration() const throw() { return _frame_duration; }
	inline unsigned GetH264HEVCNalSize() const throw() { return _264lengthSizeMinusOne; }
	inline bool HasBFrameExists() const throw() { return _has_bFrame; }

	inline bool IsRealCookAudio() const throw() { return _real_cook_audio; }
	inline bool IsRealRV40Video() const throw() { return _real_rv40_video; }

	inline bool IsSubtitleWithASS() const throw() { return _ssa_ass_format; }

	inline void SetBFrameFlag() throw() {  _has_bFrame = true; }
	inline double GetDecodeTimestamp() const throw() { return _decode_timestamp; }
	inline void SetDecodeTimestamp(double dts) throw() { _decode_timestamp = dts; }
	inline void IncrementDTS() throw() { _decode_timestamp += _frame_duration; }

	bool ProbeAudio(std::shared_ptr<MKVParser::MKVFileParser>& parser) throw();
	bool ProbeVideo(std::shared_ptr<MKVParser::MKVFileParser>& parser,bool force_avc1) throw();
	bool ProbeSubtitle(std::shared_ptr<MKVParser::MKVFileParser>& parser) throw();

private:
	MKVParser::MKVTrackInfo _info;

	MediaMainType _main_type;
	MediaCodecType _codec_type;

	int _stream_index;
	double _frame_duration;
	float _fps;
	unsigned _264lengthSizeMinusOne;

	double _decode_timestamp;
	bool _has_bFrame;

	bool _real_cook_audio, _real_rv40_video;

	std::shared_ptr<IAudioDescription> _audio_desc;
	std::shared_ptr<IVideoDescription> _video_desc;

	bool _ssa_ass_format;
	MediaSubtitleStreamInfo _subtitle_info;
	MKV::Internal::Object::Cluster::Block::AutoBuffer _subtitle_head;

	struct RealCookInfo
	{
		unsigned CodedFrameSize;
		unsigned FrameSize;
		unsigned SubPacketH;
		unsigned SubPacketSize;
	};
	RealCookInfo _cook_info;
};

#endif //__MKV_MEDIA_STREAM_H