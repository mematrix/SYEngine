#include "pch.h"
#include "MediaInfo.h"

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
HRESULT MediaInfo::Open(ABI::Windows::Storage::Streams::IRandomAccessStream* rtStream)
{
	if (rtStream == NULL)
		return E_INVALIDARG;

	ComPtr<IMFByteStream> stream;
	HRESULT hr = MFCreateMFByteStreamOnStreamEx(rtStream, &stream);
	if (FAILED(hr))
		return hr;

	return Open(stream.Get());
}
#endif

HRESULT MediaInfo::Open(IMFByteStream* stream)
{
	ComPtr<IMFSourceResolver> resolver;
	HRESULT hr = MFCreateSourceResolver(&resolver);
	if (FAILED(hr))
		return hr;

	MF_OBJECT_TYPE objType = MF_OBJECT_INVALID;
	ComPtr<IUnknown> punk;
	hr = resolver->CreateObjectFromByteStream(stream, NULL,
		MF_RESOLUTION_MEDIASOURCE|MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
		NULL, &objType, &punk);
	if (FAILED(hr))
		return hr;
	if (objType != MF_OBJECT_MEDIASOURCE)
		return E_FAIL;

	return InternalOpen(punk.Get());
}

HRESULT MediaInfo::Open(LPCWSTR uri)
{
	ComPtr<IMFSourceResolver> resolver;
	HRESULT hr = MFCreateSourceResolver(&resolver);
	if (FAILED(hr))
		return hr;

	MF_OBJECT_TYPE objType = MF_OBJECT_INVALID;
	ComPtr<IUnknown> punk;
	hr = resolver->CreateObjectFromURL(uri, MF_RESOLUTION_MEDIASOURCE, NULL, &objType, &punk);
	if (FAILED(hr))
		return hr;
	if (objType != MF_OBJECT_MEDIASOURCE)
		return E_FAIL;

	return InternalOpen(punk.Get());
}

HRESULT MediaInfo::InternalOpen(IUnknown* punk)
{
	ComPtr<IMFMediaSource> source;
	HRESULT hr = punk->QueryInterface(IID_PPV_ARGS(&source));
	if (FAILED(hr))
		return hr;

	ComPtr<IMFPresentationDescriptor> ppd;
	hr = source->CreatePresentationDescriptor(&ppd);
	if (FAILED(hr))
		return hr;

	DWORD streamCount = 0;
	ppd->GetStreamDescriptorCount(&streamCount);
	if (streamCount == 0)
		return E_UNEXPECTED;

	UINT64 duration = MFGetAttributeUINT64(ppd.Get(), MF_PD_DURATION, 0);
	UINT64 fileSize = MFGetAttributeUINT64(ppd.Get(), MF_PD_TOTAL_FILE_SIZE, 0);
	WCHAR mimeType[128] = {0};
	ppd->GetString(MF_PD_MIME_TYPE, mimeType, _countof(mimeType), NULL);

	memset(&_info, 0, sizeof(_info));
	_info.streamCount = streamCount;
	_info.duration = double(duration) / 10000000.0;
	_info.totalFileSize = fileSize;
	WideCharToMultiByte(CP_ACP, 0, mimeType, -1, _info.mimeType, _countof(_info.mimeType), NULL, NULL);

	hr = InternalLoadStreams(ppd.Get());
	if (FAILED(hr))
		return hr;

    //MFGetService(source.Get(), MF_SCRUBBING_SERVICE, IID_PPV_ARGS(&_seekInfo));
    ComPtr<IMFGetService> s;
    hr = source.As(&s);
    if (SUCCEEDED(hr))
        s->GetService(MF_SCRUBBING_SERVICE, IID_PPV_ARGS(&_seekInfo));

	_mediaSource = source;
	return S_OK;
}

HRESULT MediaInfo::InternalLoadStreams(IMFPresentationDescriptor* ppd)
{
	DWORD streamCount;
	ppd->GetStreamDescriptorCount(&streamCount);
	for (DWORD i = 0; i < streamCount; i++) {
		ComPtr<IMFStreamDescriptor> psd;
		BOOL selected;
		HRESULT hr = ppd->GetStreamDescriptorByIndex(i, &selected, &psd);
		if (FAILED(hr))
			return hr;

		ComPtr<IMFMediaTypeHandler> mediaTypeHandler;
		hr = psd->GetMediaTypeHandler(&mediaTypeHandler);
		if (FAILED(hr))
			return hr;

		ComPtr<IMFMediaType> mediaType;
		hr = mediaTypeHandler->GetCurrentMediaType(&mediaType);
		if (FAILED(hr))
			return hr;

		StreamInfo info = {};
		GUID type = GUID_NULL;
		mediaType->GetGUID(MF_MT_MAJOR_TYPE, &type);
		if (type == MFMediaType_Audio)
			info.type = StreamInfo::StreamType::Audio;
		else if (type == MFMediaType_Video)
			info.type = StreamInfo::StreamType::Video;
		else
			info.type = StreamInfo::StreamType::Unknown;

		UINT32 nameLen = 0;
		if (SUCCEEDED(psd->GetStringLength(MF_SD_STREAM_NAME, &nameLen)) && nameLen > 0) {
			WCHAR* name = (WCHAR*)calloc(3, nameLen);
			psd->GetString(MF_SD_STREAM_NAME, name, nameLen, &nameLen);
			info.name = name;
		}

		mediaType->GetGUID(MF_MT_SUBTYPE, &info.codec);
		if (info.type == StreamInfo::StreamType::Audio)
			hr = InternalInitAudio(mediaType.Get(), info);
		else if (info.type == StreamInfo::StreamType::Video)
			hr = InternalInitVideo(mediaType.Get(), info);

		if (FAILED(hr)) {
			if (info.name)
				free(const_cast<WCHAR*>(info.name));
			return hr;
		}

		UINT32 cpSize = 0;
		mediaType->GetBlobSize(MF_MT_USER_DATA, &cpSize);
		if (cpSize > 0) {
			mediaType->GetAllocatedBlob(MF_MT_USER_DATA,
				(UINT8**)&info.decoder.codecPrivate, &info.decoder.codecPrivateSize);
		}else{
			mediaType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &cpSize);
			if (cpSize > 0)
				mediaType->GetAllocatedBlob(MF_MT_MPEG_SEQUENCE_HEADER,
					(UINT8**)&info.decoder.codecPrivate, &info.decoder.codecPrivateSize);
		}

		_streams.push_back(std::move(info));
	}
	return S_OK;
}

HRESULT MediaInfo::InternalInitAudio(IMFMediaType* mediaType, StreamInfo& info)
{
	info.audio.channels = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
	info.audio.sampleRate = MFGetAttributeUINT32(mediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
	return S_OK;
}

HRESULT MediaInfo::InternalInitVideo(IMFMediaType* mediaType, StreamInfo& info)
{
	MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &info.video.width, &info.video.height);
	MFGetAttributeRatio(mediaType, MF_MT_PIXEL_ASPECT_RATIO, &info.video.pixelAR0, &info.video.pixelAR1);

	UINT32 fps_den = 0, fps_num = 0;
	MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &fps_num, &fps_den);
	info.video.frameRate = float(fps_num) / float(fps_den);
	info.video.fps_den = fps_den;
	info.video.fps_num = fps_num;

	info.video.profile = MFGetAttributeUINT32(mediaType, MF_MT_MPEG2_PROFILE, 0);
	info.video.profileLevel = MFGetAttributeUINT32(mediaType, MF_MT_MPEG2_LEVEL, 0);
	return S_OK;
}

void MediaInfo::Close()
{
	for (auto i = _streams.begin(); i != _streams.end(); ++i) {
		if (i->name)
			free(const_cast<WCHAR*>(i->name));
		if (i->decoder.codecPrivate)
			CoTaskMemFree(const_cast<void*>(i->decoder.codecPrivate));
	}
	_streams.clear();
	if (_mediaSource)
		_mediaSource->Shutdown();
	_mediaSource.Reset();
}

bool MediaInfo::GetKeyFrameTime(double userTime, double* kfTime)
{
	if (kfTime == NULL ||
		_seekInfo == NULL)
		return false;

	PROPVARIANT prev, next;
	PropVariantInit(&prev);
	PropVariantInit(&next);
	GUID timeFormat = GUID_NULL;
	PROPVARIANT time;
	PropVariantInit(&time);
	time.vt = VT_I8;
	time.hVal.QuadPart = (LONG64)(userTime * 10000000.0);
	if (FAILED(_seekInfo->GetNearestKeyFrames(&timeFormat, &time, &prev, &next)))
		return false;

	LONG64 kTime;
	if (prev.hVal.QuadPart == next.hVal.QuadPart) {
		kTime = prev.hVal.QuadPart;
		goto done;
	}
	if (time.hVal.QuadPart - prev.hVal.QuadPart >
		next.hVal.QuadPart - time.hVal.QuadPart)
		kTime = next.hVal.QuadPart;
	else
		kTime = prev.hVal.QuadPart;
done:
	double t = double(kTime) / 10000000.0;
	*kfTime = t + 0.1;
	return true;
}