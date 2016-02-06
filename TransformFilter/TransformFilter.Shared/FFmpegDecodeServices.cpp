#include "FFmpegDecodeServices.h"
#include "FFmpegVideoDecoder.h"
#include "FFmpegAudioDecoder.h"
#include "more_codec_uuid.h"

static struct FFCodecPair {
	REFGUID guid;
	AVCodecID codecid;
} kCodecPair[] = {
	{MFVideoFormat_AVC1, AV_CODEC_ID_H264},
	{MFVideoFormat_HVC1, AV_CODEC_ID_HEVC},
	{MFVideoFormat_H264, AV_CODEC_ID_H264},
	{MFVideoFormat_HEVC, AV_CODEC_ID_HEVC},
	{MFVideoFormat_H263, AV_CODEC_ID_H263},
	{MFVideoFormat_VP6,  AV_CODEC_ID_VP6},
	{MFVideoFormat_VP6F, AV_CODEC_ID_VP6F},
	{MFVideoFormat_VP8,  AV_CODEC_ID_VP8},
	{MFVideoFormat_VP9,  AV_CODEC_ID_VP9},
	{MFVideoFormat_RV30, AV_CODEC_ID_RV30},
	{MFVideoFormat_RV40, AV_CODEC_ID_RV40},
	{MFVideoFormat_MPG1, AV_CODEC_ID_MPEG1VIDEO},
	{MFVideoFormat_MPG2, AV_CODEC_ID_MPEG2VIDEO},
	{MFVideoFormat_MP4V, AV_CODEC_ID_MPEG4},
	{MFVideoFormat_MP4S, AV_CODEC_ID_MPEG4},
	{MFVideoFormat_M4S2, AV_CODEC_ID_MPEG4},
	{MFVideoFormat_MP43, AV_CODEC_ID_MSMPEG4V3},
	{MFVideoFormat_MJPG, AV_CODEC_ID_MJPEG},
	{MFVideoFormat_WMV1, AV_CODEC_ID_WMV1},
	{MFVideoFormat_WMV2, AV_CODEC_ID_WMV2},
	{MFVideoFormat_WMV3, AV_CODEC_ID_WMV3},
	{MFVideoFormat_WVC1, AV_CODEC_ID_VC1},
	{MFAudioFormat_AAC,  AV_CODEC_ID_AAC},
	{MFAudioFormat_MP3,  AV_CODEC_ID_MP3},
	{MFAudioFormat_FLAC,  AV_CODEC_ID_FLAC},
	{MFAudioFormat_ALAC,  AV_CODEC_ID_ALAC}
};

HRESULT FFmpegDecodeServices::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	if (iid != IID_IUnknown &&
		iid != __uuidof(ITransformLoader) &&
		iid != __uuidof(ITransformWorker))
		return E_NOINTERFACE;
	if (iid == __uuidof(ITransformLoader))
		*ppv = static_cast<ITransformLoader*>(this);
	else if (iid == __uuidof(ITransformWorker))
		*ppv = static_cast<ITransformWorker*>(this);
	else
		*ppv = this;
	AddRef();
	return S_OK;
}

HRESULT FFmpegDecodeServices::CheckMediaType(IMFMediaType* pMediaType)
{
	if (ConvertGuidToCodecId(pMediaType) == AV_CODEC_ID_NONE)
		return MF_E_INVALIDMEDIATYPE;
	if (!VerifyMediaType(pMediaType))
		return MF_E_INVALIDMEDIATYPE;
	return S_OK;
}

HRESULT FFmpegDecodeServices::SetInputMediaType(IMFMediaType* pMediaType)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (pMediaType == NULL) {
		_inputMediaType.Reset();
		_outputMediaType.Reset();
		return S_OK;
	}

	HRESULT hr = CheckMediaType(pMediaType);
	if (FAILED(hr))
		return hr;

	if (avcodec_find_decoder(ConvertGuidToCodecId(pMediaType)) == NULL)
		return E_ABORT;

	GUID majorType = GUID_NULL;
	pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
	if (majorType != MFMediaType_Video &&
		majorType != MFMediaType_Audio)
		return MF_E_INVALID_CODEC_MERIT;

	hr = (majorType == MFMediaType_Video ? InitVideoDecoder(pMediaType) : InitAudioDecoder(pMediaType));
	if (FAILED(hr))
		return hr;

	MFCreateMediaType(&_inputMediaType);
	pMediaType->CopyAllItems(_inputMediaType.Get());
	return S_OK;
}

HRESULT FFmpegDecodeServices::GetOutputMediaType(IMFMediaType** ppMediaType)
{
	return GetCurrentOutputMediaType(ppMediaType);
}

HRESULT FFmpegDecodeServices::GetCurrentInputMediaType(IMFMediaType** ppMediaType)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (ppMediaType == NULL)
		return E_POINTER;
	if (_inputMediaType == NULL)
		return MF_E_TRANSFORM_TYPE_NOT_SET;
	*ppMediaType = _inputMediaType.Get();
	(*ppMediaType)->AddRef();
	return S_OK;
}

HRESULT FFmpegDecodeServices::GetCurrentOutputMediaType(IMFMediaType** ppMediaType)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (ppMediaType == NULL)
		return E_POINTER;
	if (_outputMediaType == NULL)
		return MF_E_TRANSFORM_TYPE_NOT_SET;
	*ppMediaType = _outputMediaType.Get();
	(*ppMediaType)->AddRef();
	return S_OK;
}

HRESULT FFmpegDecodeServices::SetAllocator(ITransformAllocator* pAllocator)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	_sampleAllocator.Reset();
	_sampleAllocator = pAllocator;
	if (_video_decoder)
		_video_decoder->SetAllocator(pAllocator);
	return S_OK;
}

HRESULT FFmpegDecodeServices::ProcessSample(IMFSample* pSample, IMFSample** ppNewSample)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	HRESULT hr = S_OK;
	if (_video_decoder)
		hr = _video_decoder->Decode(pSample, ppNewSample);
	else if (_audio_decoder)
		hr = _audio_decoder->Decode(pSample, ppNewSample);
	return hr;
}

HRESULT FFmpegDecodeServices::ProcessFlush()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_video_decoder)
		_video_decoder->Flush();
	else if (_audio_decoder)
		_audio_decoder->Flush();
	return S_OK;
}

AVCodecID FFmpegDecodeServices::ConvertGuidToCodecId(IMFMediaType* pMediaType)
{
	GUID majorType = GUID_NULL, subType = GUID_NULL;
	pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
	pMediaType->GetGUID(MF_MT_SUBTYPE, &subType);
	if (majorType != MFMediaType_Audio && majorType != MFMediaType_Video)
		return AV_CODEC_ID_NONE;
	for (auto id : kCodecPair) {
		if (subType == id.guid)
			return id.codecid;
	}
	return AV_CODEC_ID_NONE;
}

bool FFmpegDecodeServices::VerifyMediaType(IMFMediaType* pMediaType)
{
	GUID majorType = GUID_NULL;
	pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
	if (majorType == MFMediaType_Audio)
		return VerifyAudioMediaType(pMediaType);
	else if (majorType == MFMediaType_Video)
		return VerifyVideoMediaType(pMediaType);
	return false;
}

bool FFmpegDecodeServices::VerifyAudioMediaType(IMFMediaType* pMediaType)
{
	if (MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_NUM_CHANNELS, 0) == 0)
		return false;
	if (MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0) < 8000)
		return false;
	if (MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 0) > 32)
		return false;
	return true;
}

bool FFmpegDecodeServices::VerifyVideoMediaType(IMFMediaType* pMediaType)
{
	GUID subType = GUID_NULL;
	pMediaType->GetGUID(MF_MT_SUBTYPE, &subType);

	UINT32 width = 0, height = 0;
	MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
	if (width == 0 ||
		height == 0)
		return false;

	if (subType == MFVideoFormat_H264 || subType == MFVideoFormat_HEVC ||
		subType == MFVideoFormat_MPG1 || subType == MFVideoFormat_MPEG2) {
		if ((MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_PROFILE, 0) == 0 ||
			MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_LEVEL, 0) == 0) &&
			subType != MFVideoFormat_MPG1)
			return false;

		UINT32 seqSize = 0;
		pMediaType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &seqSize);
		if (seqSize == 0)
			return false;
	}
	return true;
}

HRESULT FFmpegDecodeServices::InitAudioDecoder(IMFMediaType* pMediaType)
{
	auto dec = new(std::nothrow) FFmpegAudioDecoder();
	if (dec == NULL)
		return E_OUTOFMEMORY;

	auto mediaType = dec->Open(ConvertGuidToCodecId(pMediaType), pMediaType);
	if (mediaType == NULL) {
		delete dec;
		return E_FAIL;
	}

	_audio_decoder = dec;
	_outputMediaType.Attach(mediaType);
	return S_OK;
}

HRESULT FFmpegDecodeServices::InitVideoDecoder(IMFMediaType* pMediaType)
{
	auto dec = new(std::nothrow) FFmpegVideoDecoder();
	if (dec == NULL)
		return E_OUTOFMEMORY;

	auto mediaType = dec->Open(ConvertGuidToCodecId(pMediaType), pMediaType);
	if (mediaType == NULL) {
		delete dec;
		return E_FAIL;
	}

	_video_decoder = dec;
	_outputMediaType.Attach(mediaType);
	return S_OK;
}

void FFmpegDecodeServices::DestroyDecoders()
{
	if (_audio_decoder) {
		delete _audio_decoder;
		_audio_decoder = NULL;
	}
	if (_video_decoder) {
		_video_decoder->Release();
		_video_decoder = NULL;
	}
}