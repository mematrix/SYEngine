#include "HDMediaSource.h"
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <Shlwapi.h>
#endif

HRESULT HDMediaSource::GetService(REFGUID guidService,REFIID riid,LPVOID *ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (guidService == MF_RATE_CONTROL_SERVICE) { //针对Store应用必须提供IMFRateControl接口
		return QueryInterface(riid,ppvObject);
	}else if (guidService == MF_METADATA_PROVIDER_SERVICE) {
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
		return QueryInterface(riid,ppvObject);
#else
		CHAR szBuffer[MAX_PATH] = {};
		GetModuleFileNameA(nullptr,szBuffer,ARRAYSIZE(szBuffer));
		PathStripPathA(szBuffer);
		if (GetModuleHandleA("wmp.dll"))
		{
			if (_pMetadata)
			{
				ComPtr<IMFMetadata> pMetadata;
				if (SUCCEEDED(GetMFMetadata(_pPresentationDescriptor.Get(),0,0,pMetadata.GetAddressOf())))
					return QueryInterface(riid,ppvObject);
			}
		}else{
			return QueryInterface(riid,ppvObject);
		}
#endif
	}else if (guidService == MFNETSOURCE_STATISTICS_SERVICE) {
		if (_network_mode)
			return QueryInterface(riid,ppvObject);
	}else if (guidService == MF_SCRUBBING_SERVICE) {
		if (FAILED(MakeKeyFramesIndex()))
			return MF_E_UNSUPPORTED_SERVICE;
		return QueryInterface(riid,ppvObject);
	}

	return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT HDMediaSource::GetFastestRate(MFRATE_DIRECTION eDirection,BOOL fThin,float *pflRate)
{
	if (eDirection == MFRATE_REVERSE)
		return MF_E_REVERSE_UNSUPPORTED;
	if (fThin)
		return MF_E_THINNING_UNSUPPORTED;

	*pflRate = MAX_MFSOURCE_RATE_SUPPORT;
	return S_OK;
}

HRESULT HDMediaSource::GetSlowestRate(MFRATE_DIRECTION eDirection,BOOL fThin,float *pflRate)
{
	if (eDirection == MFRATE_REVERSE)
		return MF_E_REVERSE_UNSUPPORTED;
	if (fThin)
		return MF_E_THINNING_UNSUPPORTED;

	*pflRate = MIN_MFSOURCE_RATE_SUPPORT;
	return S_OK;
}

HRESULT HDMediaSource::IsRateSupported(BOOL fThin,float flRate,float *pflNearestSupportedRate)
{
	if (fThin)
		return MF_E_THINNING_UNSUPPORTED;

	if (flRate < MIN_MFSOURCE_RATE_SUPPORT ||
		flRate > MAX_MFSOURCE_RATE_SUPPORT) {
		if (pflNearestSupportedRate)
		{
			if (flRate > MAX_MFSOURCE_RATE_SUPPORT)
				*pflNearestSupportedRate = MAX_MFSOURCE_RATE_SUPPORT;
			else if (flRate < MIN_MFSOURCE_RATE_SUPPORT)
				*pflNearestSupportedRate = MIN_MFSOURCE_RATE_SUPPORT;
		}
		return MF_E_UNSUPPORTED_RATE;
	}

	return S_OK;
}

HRESULT HDMediaSource::SetRate(BOOL fThin,float flRate)
{
	DbgLogPrintf(L"%s::SetRate... (Thin:%d, Rate:%.2f)",L"HDMediaSource",fThin,flRate);

	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	if (fThin)
		return MF_E_THINNING_UNSUPPORTED;
	if (flRate < MIN_MFSOURCE_RATE_SUPPORT ||
		flRate > MAX_MFSOURCE_RATE_SUPPORT)
		return MF_E_UNSUPPORTED_RATE;

	if (flRate == _currentRate)
		return _pEventQueue->QueueEventParamVar(MESourceRateChanged,GUID_NULL,S_OK,nullptr);

	ComPtr<SourceOperation> op;
	hr = SourceOperation::CreateSetRateOperation(fThin,flRate,op.GetAddressOf());
	if (FAILED(hr))
		return hr;

	DbgLogPrintf(L"%s::SetRate OK.",L"HDMediaSource");

	return QueueOperation(op.Get());
}

HRESULT HDMediaSource::GetRate(BOOL* pfThin,float* pflRate)
{
	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	if (pfThin == nullptr || pflRate == nullptr)
		return E_POINTER;

	*pfThin = FALSE;
	*pflRate = _currentRate;

	return S_OK;
}

HRESULT HDMediaSource::GetNearestKeyFrames(const GUID *pguidTimeFormat,const PROPVARIANT *pvarStartPosition,PROPVARIANT *pvarPreviousKeyFrame,PROPVARIANT *pvarNextKeyFrame)
{
	if (pguidTimeFormat == nullptr || pvarStartPosition == nullptr)
		return E_INVALIDARG;
	if (pvarPreviousKeyFrame == nullptr || pvarNextKeyFrame == nullptr)
		return E_POINTER;
	if (*pguidTimeFormat != GUID_NULL)
		return MF_E_UNSUPPORTED_TIME_FORMAT;
	if (pvarStartPosition->vt != VT_I8 && pvarStartPosition->vt != VT_UI8)
		return E_FAIL;
	if (_key_frames_count == 0)
		return E_ABORT;

	double start_time = double(pvarStartPosition->hVal.QuadPart) / 10000000.0;
	double last_kf_time = *(_key_frames + (_key_frames_count - 1));
	if (start_time > last_kf_time)
		return E_FAIL;

	unsigned index = 0;
	for (unsigned i = 0;i < _key_frames_count;i++)
	{
		auto time = *(_key_frames + i);
		if (time >= start_time) //found
		{
			index = i;
			break;
		}
	}

	pvarPreviousKeyFrame->vt = pvarNextKeyFrame->vt = VT_I8;
	if (index == 0)
	{
		pvarPreviousKeyFrame->hVal.QuadPart = 
			pvarNextKeyFrame->hVal.QuadPart = LONG64(*_key_frames * 10000000.0);
		return S_OK;
	}

	pvarPreviousKeyFrame->hVal.QuadPart = 
		LONG64(*(_key_frames + (index - 1)) * 10000000.0);
	pvarNextKeyFrame->hVal.QuadPart =
		LONG64(*(_key_frames + index) * 10000000.0);
	return S_OK;
}

HRESULT HDMediaSource::MakeKeyFramesIndex()
{
	if (_key_frames != nullptr)
		return S_OK;
	if (_pMediaParser == nullptr)
		return E_FAIL;
	if (_pMediaParser->GetDuration() == 0.0 ||
		_pMediaParser->GetStreamCount() == 0)
		return E_FAIL;

	auto parser = (IAVMediaFormatEx*)_pMediaParser->StaticCastToInterface(AV_MEDIA_INTERFACE_ID_CASE_EX);
	if (parser == nullptr)
		return E_ABORT;
	if (parser->GetKeyFramesCount() < 1)
		return E_ABORT;

	_key_frames = (double*)malloc(sizeof(double) * (parser->GetKeyFramesCount() + 1));
	if (_key_frames == nullptr)
		return E_OUTOFMEMORY;

	_key_frames_count = parser->CopyKeyFrames(_key_frames);
	if (_key_frames_count > (unsigned)parser->GetKeyFramesCount())
	{
		_key_frames_count = 0;
		return E_FAIL;
	}
	return _key_frames_count > 0 ? S_OK:E_ABORT;
}