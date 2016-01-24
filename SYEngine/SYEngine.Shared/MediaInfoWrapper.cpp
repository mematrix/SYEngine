#include "MediaInfoWrapper.h"

using namespace SYEngine;
using namespace Windows::Foundation;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Storage::Streams;

IAsyncAction^ MediaInformation::OpenAsync(Uri^ uri)
{
	if (_info == nullptr)
		_info = std::make_shared<MediaInfo>();
	if (_info == nullptr)
		throw ref new Platform::OutOfMemoryException();

	auto url = _wcsdup(uri->RawUri->Data());
	return concurrency::create_async([this, url]{
		_info->Close();
		HRESULT hr = _info->Open(url);
		free(url);
		if (FAILED(hr))
			throw ref new Platform::COMException(hr, "MediaInfo->Open ERR.");

		_info->GetFormatInfo(&_format);
		InitAVInfo();
	});
}

IAsyncAction^ MediaInformation::OpenAsync(IRandomAccessStream^ stream)
{
	if (_info == nullptr)
		_info = std::make_shared<MediaInfo>();
	if (_info == nullptr)
		throw ref new Platform::OutOfMemoryException();

	auto stm = reinterpret_cast<ABI::Windows::Storage::Streams::IRandomAccessStream*>(stream);
	return concurrency::create_async([this, stm]{
		_info->Close();
		HRESULT hr = _info->Open(stm);
		if (FAILED(hr))
			throw ref new Platform::COMException(hr, "MediaInfo->Open ERR.");

		_info->GetFormatInfo(&_format);
		InitAVInfo();
	});
}

AudioEncodingProperties^ MediaInformation::GetAudioInfo()
{
	if (_audio.codec == GUID_NULL)
		return nullptr;

	WCHAR guid[128] = {};
	StringFromGUID2(_audio.codec, guid, _countof(guid));
	auto prop = AudioEncodingProperties::CreatePcm(_audio.audio.sampleRate, _audio.audio.channels, 0);
	if (_audio.codec == MFAudioFormat_AAC)
		prop->Subtype = "AAC";
	else if (_audio.codec == MFAudioFormat_MP3)
		prop->Subtype = "MP3";
	else if (_audio.codec == MFAudioFormat_Dolby_AC3 || _audio.codec == MFAudioFormat_Dolby_DDPlus)
		prop->Subtype = "AC3";
	else if (_audio.codec == MFAudioFormat_DTS)
		prop->Subtype = "DTS";
	else
		prop->Subtype = ref new Platform::String(guid);
	return prop;
}

VideoEncodingProperties^ MediaInformation::GetVideoInfo()
{
	if (_video.codec == GUID_NULL)
		return nullptr;

	WCHAR guid[128] = {};
	StringFromGUID2(_video.codec, guid, _countof(guid));
	auto prop = VideoEncodingProperties::CreateH264();
	prop->Width = _video.video.width;
	prop->Height = _video.video.height;
	prop->ProfileId = _video.video.profile;
	prop->FrameRate->Denominator = _video.video.fps_den;
	prop->FrameRate->Numerator = _video.video.fps_num;
	if (_video.codec == MFVideoFormat_H264)
		prop->Subtype = "H264";
	else if (_video.codec == MFVideoFormat_HEVC)
		prop->Subtype = "HEVC";
	else
		prop->Subtype = ref new Platform::String(guid);
	return prop;
}

double MediaInformation::GetKeyFrameTime(double userTime)
{
	double result;
	if (!_info->GetKeyFrameTime(userTime, &result))
		return userTime;
	return result;
}

void MediaInformation::InitAVInfo()
{
	for (DWORD i = 0; i < _format.streamCount; i++) {
		MediaInfo::StreamInfo info = {};
		_info->GetStreamInfo(i, &info);
		if (info.type == MediaInfo::StreamInfo::StreamType::Audio)
			_audio = info;
		else if (info.type == MediaInfo::StreamInfo::StreamType::Video)
			_video = info;

		if (_audio.codec != GUID_NULL &&
			_video.codec != GUID_NULL)
			break;
	}
}