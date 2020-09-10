#pragma once
#include "MediaInformation.g.h"

#include "MediaInfo.h"


namespace winrt::SYEngine::implementation {

struct MediaInformation : MediaInformationT<MediaInformation>
{
    MediaInformation();

    Windows::Foundation::IAsyncAction OpenAsync(Windows::Foundation::Uri uri);

    Windows::Foundation::IAsyncAction OpenAsync(Windows::Storage::Streams::IRandomAccessStream stream);

    Windows::Media::MediaProperties::AudioEncodingProperties GetAudioInfo();

    Windows::Media::MediaProperties::VideoEncodingProperties GetVideoInfo();

    double Duration();

    int64_t FileSize();

    hstring MimeType();

    double GetKeyFrameTime(double user_time);

private:
    void InitAVInfo();

    std::shared_ptr<MediaInfo> info_;
    MediaInfo::FormatInfo format_;
    MediaInfo::StreamInfo audio_;
    MediaInfo::StreamInfo video_;
};

}
