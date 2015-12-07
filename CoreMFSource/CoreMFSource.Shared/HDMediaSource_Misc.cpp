#include "HDMediaSource.h"
#include "HDMediaStream.h"

HRESULT HDMediaSource::ValidatePresentationDescriptor(IMFPresentationDescriptor* ppd)
{
	if (ppd == nullptr)
		return E_UNEXPECTED;

	DWORD dwStreamCount = 0;
	HRESULT hr = ppd->GetStreamDescriptorCount(&dwStreamCount);
	if (FAILED(hr))
		return hr;

	if (dwStreamCount != _streamList.Count())
		return E_INVALIDARG;

	BOOL fSelected = FALSE;
	for (unsigned i = 0;i < dwStreamCount;i++)
	{
		ComPtr<IMFStreamDescriptor> psd;
		hr = ppd->GetStreamDescriptorByIndex(i,&fSelected,psd.GetAddressOf());
		if (FAILED(hr))
			break;

		if (fSelected)
			break;
	}

	if (FAILED(hr))
		return hr;

	if (!fSelected)
		return E_INVALIDARG;

	BOOL bMaskFlag = FALSE;
	for (unsigned i = 0;i < dwStreamCount;i++)
	{
		ComPtr<IMFStreamDescriptor> psd;
		hr = ppd->GetStreamDescriptorByIndex(i,&fSelected,psd.GetAddressOf());
		if (FAILED(hr))
			break;

		ComPtr<IMFMediaTypeHandler> pHandler;
		hr = psd->GetMediaTypeHandler(pHandler.GetAddressOf());
		if (FAILED(hr))
			break;

		ComPtr<IMFMediaType> pMediaType;
		hr = pHandler->GetCurrentMediaType(pMediaType.GetAddressOf());
		if (FAILED(hr))
			break;

		UINT32 stream_id = 0xFFFFFFFF;
		pMediaType->GetUINT32(MF_MY_STREAM_ID,&stream_id);

		if (stream_id != 0xFFFFFFFF)
		{
			bMaskFlag = TRUE;
			break;
		}
	}

	if (FAILED(hr))
		return hr;

	if (!bMaskFlag)
		return E_INVALIDARG;

	return S_OK;
}

HRESULT HDMediaSource::FindMediaStreamById(int id,HDMediaStream** ppStream)
{
	if (id < 0)
		return MF_E_INVALIDINDEX;
	if (ppStream == nullptr)
		return E_POINTER;

	unsigned count = _streamList.Count();
	if (count == 0)
		return MF_E_UNEXPECTED;

	bool found = false;

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_RET(hr);

		if (id == pStream->GetStreamIndex())
		{
			found = true;

			*ppStream = pStream.Detach();
			break;
		}
	}

	return found ? S_OK:MF_E_INVALIDSTREAMNUMBER;
}

HRESULT HDMediaSource::ProcessSampleMerge(unsigned char* pHead,unsigned len,IMFSample* pOldSample,IMFSample** ppNewSample)
{
	ComPtr<IMFMediaBuffer> pOldMediaBuffer;

	HRESULT hr = pOldSample->GetBufferByIndex(0,pOldMediaBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	DWORD dwOldSize = 0;
	pOldMediaBuffer->GetCurrentLength(&dwOldSize);
	if (dwOldSize == 0)
		return MF_E_BUFFERTOOSMALL;

	WMF::AutoBufLock pOldBuf(pOldMediaBuffer.Get());
	if (pOldBuf.GetPtr() == nullptr)
		return MF_E_UNEXPECTED;

	ComPtr<IMFSample> pNewSample;
	ComPtr<IMFMediaBuffer> pNewMediaBuffer;

	hr = WMF::Misc::CreateSampleWithBuffer(pNewSample.GetAddressOf(),pNewMediaBuffer.GetAddressOf(),
		dwOldSize + len);

	if (FAILED(hr))
		return hr;

	WMF::AutoBufLock pNewLock(pNewMediaBuffer.Get());
	if (pNewLock.GetPtr() == nullptr)
		return MF_E_UNEXPECTED;

	memcpy(pNewLock.GetPtr(),pHead,len);
	memcpy(pNewLock.GetPtr() + len,pOldBuf.GetPtr(),dwOldSize);

	pNewLock.Unlock();

	hr = pNewMediaBuffer->SetCurrentLength(dwOldSize + len);
	if (FAILED(hr))
		return hr;

	*ppNewSample = pNewSample.Detach();
	return  S_OK;
}

DWORD HDMediaSource::SetupVideoQueueSize(float fps,UINT width,UINT height)
{
	if (width > 1920)
		return (DWORD)fps * 2;
	else
		return (DWORD)fps;
}

DWORD HDMediaSource::SetupAudioQueueSize(IMFMediaType* pAudioMediaType)
{
	if (_pMediaParser->GetMimeType())
	{
		if (strstr(_pMediaParser->GetMimeType(),"matroska") != nullptr)
			return 5;
		else if (strstr(_pMediaParser->GetMimeType(),"mpeg4") != nullptr)
			return 4;
	}
	return 2;
}

DWORD HDMediaSource::ObtainBestAudioStreamIndex(IMFPresentationDescriptor* ppd)
{
	static const GUID bestStreamTypes[] = {MFAudioFormat_PCM,
		MFAudioFormat_Float,
		MFAudioFormat_FLAC_FRAMED,
		MFAudioFormat_OPUS_FRAMED,
		MFAudioFormat_Vorbis_FRAMED,
		MFAudioFormat_AAC,
		MFAudioFormat_MP3,
		MFAudioFormat_TTA1,
		MFAudioFormat_WV4,
		MFAudioFormat_ALAC,
		MFAudioFormat_DTS,
		MFAudioFormat_Dolby_AC3};

	DWORD dwResult = 0xFF;
	DWORD dwStreamCount = 0;
	ppd->GetStreamDescriptorCount(&dwStreamCount);
	for (unsigned i = 0;i < dwStreamCount;i++)
	{
		ComPtr<IMFMediaType> pMediaType;
		HRESULT hr = WMF::Misc::GetMediaTypeFromPD(ppd,i,pMediaType.GetAddressOf());
		if (FAILED(hr))
			continue;

		GUID mainType = GUID_NULL,subType = GUID_NULL;
		pMediaType->GetGUID(MF_MT_MAJOR_TYPE,&mainType);
		pMediaType->GetGUID(MF_MT_SUBTYPE,&subType);
		if (mainType != MFMediaType_Audio)
			continue;

		for (auto g : bestStreamTypes)
		{
			if (g == subType)
			{
				dwResult = i;
				break;
			}
		}
		if (dwResult != 0xFF)
			break;
	}
	return dwResult;
}

double HDMediaSource::GetAllStreamMaxQueueTime()
{
	double time = 0.0;
	unsigned count = _streamList.Count();
	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		double n = pStream->GetSampleQueueLastTime();
		if (n > time)
			time = n;
	}
	return time;
}

bool HDMediaSource::IsNeedNetworkBuffering()
{
	unsigned count = _streamList.Count();
	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_BREAK(hr);

		if (pStream->IsActive() && pStream->GetSampleQueueCount() == 0)
			return true;
	}
	return false;
}