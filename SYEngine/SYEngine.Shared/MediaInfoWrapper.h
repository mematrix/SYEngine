#pragma once

#include "pch.h"
#include "MediaInfo.h"

namespace SYEngine
{
	public ref class MediaInformation sealed
	{
	public:
		MediaInformation()
		{ RtlZeroMemory(&_format, sizeof(_format));
		  RtlZeroMemory(&_audio, sizeof(_audio)); RtlZeroMemory(&_video, sizeof(_video)); }
		virtual ~MediaInformation() {}

	public:
		[Windows::Foundation::Metadata::DefaultOverloadAttribute]
		Windows::Foundation::IAsyncAction^ OpenAsync(Windows::Foundation::Uri^ uri);
		Windows::Foundation::IAsyncAction^ OpenAsync(Windows::Storage::Streams::IRandomAccessStream^ stream);

		//May return null
		Windows::Media::MediaProperties::AudioEncodingProperties^ GetAudioInfo();
		Windows::Media::MediaProperties::VideoEncodingProperties^ GetVideoInfo();

		property double Duration
		{
			double get() { return _format.duration; }
		}

		property int64 FileSize
		{
			int64 get() { return (int64)_format.totalFileSize; }
		}

		property Platform::String^ MimeType
		{
			Platform::String^ get()
			{
				WCHAR mimeType[256] = {0};
				MultiByteToWideChar(CP_ACP, 0, _format.mimeType, -1, mimeType, _countof(mimeType));
				return ref new Platform::String(mimeType);
			}
		}

		double GetKeyFrameTime(double userTime);

	private:
		void InitAVInfo();

		std::shared_ptr<MediaInfo> _info;
		MediaInfo::FormatInfo _format;
		MediaInfo::StreamInfo _audio, _video;
	};
}