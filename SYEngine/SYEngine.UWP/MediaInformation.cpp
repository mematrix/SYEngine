#include "pch.h"
#include "MediaInformation.h"
#include "MediaInformation.g.cpp"

#include <winrt/Windows.Media.MediaProperties.h>


namespace abi {

using namespace ABI::Windows::Storage::Streams;

}

namespace wr {

using namespace winrt::Windows::Media::MediaProperties;

}

namespace winrt::SYEngine::implementation {

MediaInformation::MediaInformation() : format_{}, audio_{}, video_{} {}

Windows::Foundation::IAsyncAction MediaInformation::OpenAsync(Windows::Foundation::Uri uri)
{
    if (nullptr == info_) {
        info_ = std::make_shared<MediaInfo>();
    }

    co_await winrt::resume_background();

    info_->Close();
    auto hr = info_->Open(uri.RawUri().c_str());
    if (FAILED(hr)) {
        throw_hresult(hr);
    }

    info_->GetFormatInfo(&format_);
    InitAVInfo();
}

Windows::Foundation::IAsyncAction MediaInformation::OpenAsync(Windows::Storage::Streams::IRandomAccessStream stream)
{
    if (!info_) {
        info_ = std::make_shared<MediaInfo>();
    }

    co_await winrt::resume_background();

    info_->Close();
    auto hr = info_->Open(static_cast<abi::IRandomAccessStream *>(winrt::get_abi(stream)));
    if (FAILED(hr)) {
        throw_hresult(hr);
    }

    info_->GetFormatInfo(&format_);
    InitAVInfo();
}

Windows::Media::MediaProperties::AudioEncodingProperties MediaInformation::GetAudioInfo()
{
    if (audio_.codec == GUID_NULL) {
        return nullptr;
    }

    auto prop = wr::AudioEncodingProperties::CreatePcm(audio_.audio.sampleRate, audio_.audio.channels, 0);
    if (audio_.codec == MFAudioFormat_AAC) {
        prop.Subtype(L"AAC");
    } else if (audio_.codec == MFAudioFormat_MP3) {
        prop.Subtype(L"MP3");
    } else if (audio_.codec == MFAudioFormat_Dolby_AC3 || audio_.codec == MFAudioFormat_Dolby_DDPlus) {
        prop.Subtype(L"AC3");
    } else if (audio_.codec == MFAudioFormat_DTS) {
        prop.Subtype(L"DTS");
    } else {
        WCHAR guid[128] = {};
        StringFromGUID2(audio_.codec, guid, _countof(guid));
        prop.Subtype(guid);
    }

    return prop;
}

Windows::Media::MediaProperties::VideoEncodingProperties MediaInformation::GetVideoInfo()
{
    if (video_.codec == GUID_NULL) {
        return nullptr;
    }

    auto prop = wr::VideoEncodingProperties::CreateH264();
    prop.Width(video_.video.width);
    prop.Height(video_.video.height);
    prop.ProfileId(video_.video.profile);
    prop.FrameRate().Denominator(video_.video.fps_den);
    prop.FrameRate().Numerator(video_.video.fps_num);
    if (video_.codec == MFVideoFormat_H264) {
        prop.Subtype(L"H264");
    } else if (video_.codec == MFVideoFormat_HEVC) {
        prop.Subtype(L"HEVC");
    } else {
        WCHAR guid[128] = {};
        StringFromGUID2(video_.codec, guid, _countof(guid));
        prop.Subtype(guid);
    }

    return prop;
}

double MediaInformation::Duration()
{
    return format_.duration;
}

int64_t MediaInformation::FileSize()
{
    return format_.totalFileSize;
}

hstring MediaInformation::MimeType()
{
    return to_hstring(format_.mimeType);
}

double MediaInformation::GetKeyFrameTime(double user_time)
{
    double result;
    if (!info_->GetKeyFrameTime(user_time, &result)) {
        return user_time;
    }

    return result;
}

void MediaInformation::InitAVInfo()
{
    for (int i = 0; i < format_.streamCount; ++i) {
        MediaInfo::StreamInfo info{};
        info_->GetStreamInfo(i, &info);
        if (info.type == MediaInfo::StreamInfo::StreamType::Audio) {
            audio_ = info;
        } else if (info.type == MediaInfo::StreamInfo::StreamType::Video) {
            video_ = info;
        }

        if (audio_.codec != GUID_NULL && video_.codec != GUID_NULL) {
            break;
        }
    }
}

}
