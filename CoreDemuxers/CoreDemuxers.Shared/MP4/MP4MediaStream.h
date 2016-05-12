#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MP4_MEDIA_STREAM_H
#define __MP4_MEDIA_STREAM_H

#include <shared_memory_ptr.h>
#include <MediaAVFormat.h>

#include <AAC/AACAudioDescription.h>
#include <MP3/MPEGAudioDescription.h>
#include <OGG/VorbisAudioDescription.h>
#include <ALAC/ALACAudioDescription.h>
#include <AC3/AC3AudioDescription.h>
#include <H264/X264VideoDescription.h>
#include <H264/AVC1VideoDescription.h>
#include <MPEG4/MPEG4VideoDescription.h>
#include <CommonAudioDescription.h>
#include <CommonVideoDescription.h>

#include "Core/Aka4Splitter.h"

class MP4MediaStream : public IAVMediaStream
{
public:
	MP4MediaStream() = delete;
	explicit MP4MediaStream(Aka4Splitter::TrackInfo* info,unsigned index) throw()
	{ _info = info; _nal_size = 0; _stream_index = index; _movsub2text = false; }

public:
	MediaMainType GetMainType() { return _main_type; }
	MediaCodecType GetCodecType() { return _codec_type; }

	MediaSubtitleStreamInfo* GetSubtitleInfo() { return &_subtitle_info; }

	int GetStreamIndex() { return _stream_index; }
	const char* GetStreamName()
	{ return _info->Name ? _info->Name:nullptr; }
	const char* GetLanguageName() { return _info->LangId; }

	double GetDuration()
	{ return _info->InternalTimeScale > 0 ? (double)_info->InternalTrack->TotalDuration / _info->InternalTimeScale:0; }
	unsigned GetBitrate()
	{ return _info->BitratePerSec; }
	long long GetSamples()
	{ return _info->InternalTrack->TotalFrames; }

	int GetRotation()
	{ return _info->InternalTrack->VideoRotation; }
	int GetRawTimescale()
	{ return _info->InternalTrack->MediaInfo.TimeScale; }
	float GetContainerFps()
	{ return _info->Video.FrameRate; }

	IAudioDescription* GetAudioInfo()
	{ if (_main_type == MEDIA_MAIN_TYPE_AUDIO) return _audio_desc.get(); return nullptr; }
	IVideoDescription* GetVideoInfo()
	{ if (_main_type == MEDIA_MAIN_TYPE_VIDEO) return _video_desc.get(); return nullptr; }

public:
	inline unsigned GetNaluSize() const throw() { return _nal_size; }
	inline bool IsAppleMovText() const throw() { return _movsub2text; }
	inline Aka4Splitter::TrackInfo* GetTrackInfo() throw() { return _info; }

	bool ProbeAudio(std::shared_ptr<Aka4Splitter>& demux) throw();
	bool ProbeVideo(std::shared_ptr<Aka4Splitter>& demux,bool avc1) throw();
	bool ProbeText(std::shared_ptr<Aka4Splitter>& demux,bool non_tx3g) throw();

	bool InitAudioAAC(unsigned char* data,unsigned size); //LC
	bool InitAudioALAC(unsigned char* data,unsigned size);
	bool InitAudioVorbis(unsigned char* data,unsigned size);
	bool InitAudioAC3(unsigned fcc);
	bool InitAudioCommon();

	bool InitVideoMPEG4(unsigned char* data,unsigned size);
	bool InitVideoH264(unsigned char* avcc,unsigned size);
	bool InitVideoAVC1();
	bool InitVideoHEVC(unsigned char* avcc,unsigned size) { return false; }
	bool InitVideoHVC1() { return false; }

private:
	Aka4Splitter::TrackInfo* _info;

	MediaMainType _main_type;
	MediaCodecType _codec_type;

	MediaSubtitleStreamInfo _subtitle_info;
	bool _movsub2text;

	std::shared_ptr<IAudioDescription> _audio_desc;
	std::shared_ptr<IVideoDescription> _video_desc;
	unsigned _nal_size;

	int _stream_index;
};

#endif //__MP4_MEDIA_STREAM_H