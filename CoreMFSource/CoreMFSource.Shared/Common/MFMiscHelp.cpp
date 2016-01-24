#include "MFMiscHelp.h"

HRESULT WMF::Misc::CreateSampleWithBuffer(IMFSample** ppSample,IMFMediaBuffer** ppBuffer,DWORD dwBufSize,DWORD dwAlignedSize)
{
	if (ppSample == nullptr ||
		ppBuffer == nullptr)
		return E_POINTER;
	if (dwBufSize == 0)
		return E_INVALIDARG;

	Microsoft::WRL::ComPtr<IMFSample> pSample;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;

	HRESULT hr = S_OK;
	if (dwAlignedSize > 0)
		hr = MFCreateAlignedMemoryBuffer(dwBufSize,dwAlignedSize - 1,pBuffer.GetAddressOf());
	else
		hr = MFCreateMemoryBuffer(dwBufSize,pBuffer.GetAddressOf());

	if (FAILED(hr))
		return hr;

	hr = MFCreateSample(pSample.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = pSample->AddBuffer(pBuffer.Get());
	if (FAILED(hr))
		return hr;

	if (ppSample)
		*ppSample = pSample.Detach();
	if (ppBuffer)
		*ppBuffer = pBuffer.Detach();

	return S_OK;
}

HRESULT WMF::Misc::CreateSampleAndCopyData(IMFSample** ppSample,PVOID pData,DWORD dwDataSize,MFTIME pts,MFTIME duration)
{
	if (pData == nullptr ||
		dwDataSize == 0)
		return E_INVALIDARG;

	Microsoft::WRL::ComPtr<IMFSample> pSample;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;

	HRESULT hr = WMF::Misc::CreateSampleWithBuffer(pSample.GetAddressOf(),
		pBuffer.GetAddressOf(),dwDataSize);
	if (FAILED(hr))
		return hr;

	if (pts != -1)
	{
		hr = pSample->SetSampleTime(pts);
		if (FAILED(hr))
			return hr;
	}

	if (duration > 0)
	{
		hr = pSample->SetSampleDuration(duration);
		if (FAILED(hr))
			return hr;
	}

	PBYTE data = nullptr;
	hr = pBuffer->Lock(&data,nullptr,nullptr);
	if (FAILED(hr))
		return hr;

	memcpy(data,pData,dwDataSize);

	pBuffer->Unlock();
	hr = pBuffer->SetCurrentLength(dwDataSize);
	if (FAILED(hr))
		return hr;

	*ppSample = pSample.Detach();
	return S_OK;
}

HRESULT WMF::Misc::CreateAudioMediaType(IMFMediaType** ppMediaType)
{
	if (ppMediaType == nullptr)
		return E_POINTER;

	HRESULT hr = MFCreateMediaType(ppMediaType);
	if (FAILED(hr))
		return hr;

	return (*ppMediaType)->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio);
}

HRESULT WMF::Misc::CreateVideoMediaType(IMFMediaType** ppMediaType)
{
	if (ppMediaType == nullptr)
		return E_POINTER;

	HRESULT hr = MFCreateMediaType(ppMediaType);
	if (FAILED(hr))
		return hr;

	return (*ppMediaType)->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
}

HRESULT WMF::Misc::IsAudioMediaType(IMFMediaType* pMediaType)
{
	if (pMediaType == nullptr)
		return E_INVALIDARG;

	GUID type = {};
	HRESULT hr = pMediaType->GetGUID(MF_MT_MAJOR_TYPE,&type);
	if (FAILED(hr))
		return hr;

	if (type == MFMediaType_Audio)
		return S_OK;

	return E_NOT_SET;
}

HRESULT WMF::Misc::IsVideoMediaType(IMFMediaType* pMediaType)
{
	if (pMediaType == nullptr)
		return E_INVALIDARG;

	GUID type = {};
	HRESULT hr = pMediaType->GetGUID(MF_MT_MAJOR_TYPE,&type);
	if (FAILED(hr))
		return hr;

	if (type == MFMediaType_Video)
		return S_OK;

	return E_NOT_SET;
}

HRESULT WMF::Misc::IsAudioPCMSubType(IMFMediaType* pMediaType)
{
	GUID subType = GUID_NULL;
	HRESULT hr = pMediaType->GetGUID(MF_MT_SUBTYPE,&subType);
	if (FAILED(hr))
		return hr;

	if (subType == MFAudioFormat_PCM ||
		subType == MFAudioFormat_Float)
		return S_OK;

	return E_NOT_SET;
}

HRESULT WMF::Misc::IsVideoRGBSubType(IMFMediaType* pMediaType)
{
	GUID subType = GUID_NULL;
	HRESULT hr = pMediaType->GetGUID(MF_MT_SUBTYPE,&subType);
	if (FAILED(hr))
		return hr;

	if (subType == MFVideoFormat_RGB8 ||
		subType == MFVideoFormat_RGB555 ||
		subType == MFVideoFormat_RGB565 ||
		subType == MFVideoFormat_RGB24 ||
		subType == MFVideoFormat_RGB32 ||
		subType == MFVideoFormat_ARGB32)
		return S_OK;

	return E_NOT_SET;
}

HRESULT WMF::Misc::IsVideoYUVSubType(IMFMediaType* pMediaType)
{
	GUID subType = GUID_NULL;
	HRESULT hr = pMediaType->GetGUID(MF_MT_SUBTYPE,&subType);
	if (FAILED(hr))
		return hr;

	if (subType == MFVideoFormat_I420 || //8bit
		subType == MFVideoFormat_IYUV ||
		subType == MFVideoFormat_UYVY ||
		subType == MFVideoFormat_YUY2 ||
		subType == MFVideoFormat_AYUV ||
		subType == MFVideoFormat_YV12 ||
		subType == MFVideoFormat_NV12 ||
		subType == MFVideoFormat_P010 || //10-16bit
		subType == MFVideoFormat_P016 ||
		subType == MFVideoFormat_P210 ||
		subType == MFVideoFormat_P216 ||
		subType == MFVideoFormat_Y210 ||
		subType == MFVideoFormat_Y216 ||
		subType == MFVideoFormat_Y410 ||
		subType == MFVideoFormat_Y416)
		return S_OK;

	return E_NOT_SET;
}

HRESULT WMF::Misc::GetMediaTypeFromPD(IMFPresentationDescriptor* ppd,DWORD dwStreamIndex,IMFMediaType** ppMediaType)
{
	DWORD dwStreamCount = 0;
	HRESULT hr = ppd->GetStreamDescriptorCount(&dwStreamCount);
	if (FAILED(hr))
		return hr;

	if (dwStreamIndex >= dwStreamCount)
		return E_UNEXPECTED;

	BOOL temp;
	Microsoft::WRL::ComPtr<IMFStreamDescriptor> psd;
	hr = ppd->GetStreamDescriptorByIndex(dwStreamIndex,&temp,psd.GetAddressOf());
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IMFMediaTypeHandler> pHandler;
	hr = psd->GetMediaTypeHandler(pHandler.GetAddressOf());
	if (FAILED(hr))
		return hr;

	return pHandler->GetCurrentMediaType(ppMediaType);
}

LONG64 WMF::Misc::SecondsToMFTime(double seconds)
{
	return (LONG64)(seconds * 10000000.0);
}

double WMF::Misc::SecondsFromMFTime(LONG64 time)
{
	return (double)time / 10000000.0;
}

double WMF::Misc::GetSecondsFromMFSample(IMFSample* pSample)
{
	if (pSample == nullptr)
		return -1.0;
	LONGLONG result = 0;
	if (FAILED(pSample->GetSampleTime(&result)))
		return -1.0;
	return (double)result / 10000000.0;
}

bool WMF::Misc::IsMFTExists(LPCWSTR clsid)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	GUID guid = GUID_NULL;
	if (FAILED(CLSIDFromString(clsid,&guid)))
		return false;
	LPWSTR name = NULL;
	if (FAILED(MFTGetInfo(guid,&name,NULL,NULL,NULL,NULL,NULL)))
		return false;
	if (name)
		CoTaskMemFree(name);
#endif
	return true;
}

bool WMF::Misc::IsMFTExists(LPCSTR clsid)
{
	WCHAR temp[MAX_PATH] = {};
	MultiByteToWideChar(CP_ACP,0,clsid,-1,temp,MAX_PATH);
	return WMF::Misc::IsMFTExists(temp);
}