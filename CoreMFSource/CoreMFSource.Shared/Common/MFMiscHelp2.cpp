#include "MFMiscHelp2.h"

void WMF::MediaType::SetMajorAndSubType(IMFMediaType* pMediaType,const GUID& majorType,const GUID& subType)
{
	pMediaType->SetGUID(MF_MT_MAJOR_TYPE,majorType);
	pMediaType->SetGUID(MF_MT_SUBTYPE,subType);
}

GUID WMF::MediaType::GetMediaTypeGUID(IMFMediaType* pMediaType,WMF::MediaType::MFMediaType guidType)
{
	GUID result = GUID_NULL;
	if (guidType == MediaType_MajorType)
		pMediaType->GetGUID(MF_MT_MAJOR_TYPE,&result);
	else if (guidType == MediaType_SubType)
		pMediaType->GetGUID(MF_MT_SUBTYPE,&result);

	return result;
}

HRESULT WMF::MediaType::GetBasicVideoInfo(IMFMediaType* pMediaType,WMF::MediaType::MFVideoBasicInfo* info)
{
	if (pMediaType == nullptr || info == nullptr)
		return E_INVALIDARG;

	if (WMF::MediaType::GetMediaTypeGUID(pMediaType,MediaType_MajorType) != 
		MFMediaType_Video)
		return MF_E_INVALIDMEDIATYPE;

	HRESULT hr = MFGetAttributeSize(pMediaType,MF_MT_FRAME_SIZE,
		(PUINT32)&info->width,(PUINT32)&info->height);

	if (FAILED(hr))
		return hr;

	hr = MFGetAttributeRatio(pMediaType,MF_MT_FRAME_RATE,
		(PUINT32)&info->fps.Numerator,(PUINT32)&info->fps.Denominator);
	hr = MFGetAttributeRatio(pMediaType,MF_MT_PIXEL_ASPECT_RATIO,
		(PUINT32)&info->ar.Numerator,(PUINT32)&info->ar.Denominator);

	hr = pMediaType->GetUINT32(MF_MT_INTERLACE_MODE,(PUINT32)&info->interlace_mode);
	if (FAILED(hr))
		return hr;

	pMediaType->GetUINT32(MF_MT_SAMPLE_SIZE,&info->sample_size);
	return S_OK;
}

HRESULT WMF::MediaType::GetBasicAudioInfo(IMFMediaType* pMediaType,MFAudioBasicInfo* info)
{
	if (pMediaType == nullptr || info == nullptr)
		return E_INVALIDARG;

	if (WMF::MediaType::GetMediaTypeGUID(pMediaType,MediaType_MajorType) != 
		MFMediaType_Audio)
		return MF_E_INVALIDMEDIATYPE;

	pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,(PUINT32)&info->channelCount);
	pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,(PUINT32)&info->sampleRate);
	pMediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,(PUINT32)&info->sampleSize);
	pMediaType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,(PUINT32)&info->blockAlign);
	pMediaType->GetUINT32(MF_MT_AUDIO_CHANNEL_MASK,(PUINT32)&info->chLayout);

	return S_OK;
}

HRESULT WMF::MediaType::GetBlobDataAlloc(IMFMediaType* pMediaType,const GUID& guidKey,BYTE** ppBuffer,PDWORD pdwSize)
{
	if (pMediaType == nullptr)
		return E_INVALIDARG;
	if (ppBuffer == nullptr)
		return E_POINTER;

	UINT32 nSize = 0;
	HRESULT hr = pMediaType->GetBlobSize(guidKey,&nSize);
	if (FAILED(hr))
		return hr;

	if (nSize == 0)
		return E_UNEXPECTED;

	if (pdwSize)
		*pdwSize = nSize;

	PBYTE pb = (PBYTE)CoTaskMemAlloc(nSize + 1);
	if (pb == nullptr)
		return E_OUTOFMEMORY;

	hr = pMediaType->GetBlob(guidKey,pb,nSize,&nSize);
	if (FAILED(hr))
		return hr;

	*ppBuffer = pb;
	return S_OK;
}