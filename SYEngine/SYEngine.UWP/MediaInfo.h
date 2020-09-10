#pragma once

#include "pch.h"
#include <vector>
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#include "MFSeekInfo.hxx"
#include <windows.storage.streams.h>
#endif

class MediaInfo
{
public:
	struct FormatInfo
	{
		DWORD streamCount;
		double duration;
		UINT64 totalFileSize;
		char mimeType[128];
	};

	struct StreamInfo
	{
		enum StreamType
		{
			Audio,
			Video,
			Unknown
		};
		StreamType type;
		GUID codec;
		const WCHAR* name;

		struct AudioInfo
		{
			unsigned channels;
			unsigned sampleRate;
		};
		struct VideoInfo
		{
			unsigned width, height;
			float frameRate;
			unsigned fps_den, fps_num;
			unsigned pixelAR0, pixelAR1;
			DWORD profile, profileLevel;
		};
		AudioInfo audio;
		VideoInfo video;

		struct DecoderInfo
		{
			const void* codecPrivate;
			unsigned codecPrivateSize;
		};
		DecoderInfo decoder;
	};

public:
	MediaInfo() { MFStartup(MF_VERSION); }
	~MediaInfo() { Close(); MFShutdown(); }

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	HRESULT Open(ABI::Windows::Storage::Streams::IRandomAccessStream* rtStream);
#endif
	HRESULT Open(IMFByteStream* stream);
	HRESULT Open(LPCWSTR uri);
	void Close();

	void GetFormatInfo(FormatInfo* info) { *info = _info; }
	void GetStreamInfo(DWORD index, StreamInfo* info)
	{ if (index < _streams.size()) *info = _streams.at(index); }

	bool GetKeyFrameTime(double userTime, double* kfTime);

private:
	HRESULT InternalOpen(IUnknown* punk);
	HRESULT InternalLoadStreams(IMFPresentationDescriptor* ppd);
	HRESULT InternalInitAudio(IMFMediaType* mediaType, StreamInfo& info);
	HRESULT InternalInitVideo(IMFMediaType* mediaType, StreamInfo& info);

	ComPtr<IMFMediaSource> _mediaSource;
	ComPtr<IMFSeekInfo> _seekInfo;

	FormatInfo _info;
	std::vector<StreamInfo> _streams;
};