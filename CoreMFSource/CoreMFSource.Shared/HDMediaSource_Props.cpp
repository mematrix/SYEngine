#include "HDMediaSource.h"
#include "HDMediaStream.h"

static const DWORD kPids[] = {
	MFNETSOURCE_BUFFERPROGRESS_ID,
	MFNETSOURCE_BUFFERSIZE_ID,
	MFNETSOURCE_VBR_ID,
	MFNETSOURCE_CONTENTBITRATE_ID,
	MFNETSOURCE_MAXBITRATE_ID
};

HRESULT HDMediaSource::GetCount(DWORD* cProps)
{
	if (cProps == nullptr)
		return E_POINTER;
	*cProps = _countof(kPids);
	return S_OK;
}

HRESULT HDMediaSource::GetAt(DWORD iProp,PROPERTYKEY *pkey)
{
	if (pkey == nullptr)
		return E_POINTER;
	if (iProp >= _countof(kPids))
		return E_INVALIDARG;
	pkey->fmtid = MFNETSOURCE_STATISTICS;
	pkey->pid = kPids[iProp];
	return S_OK;
}

HRESULT HDMediaSource::GetValue(REFPROPERTYKEY key,PROPVARIANT *pv)
{
	if (pv == nullptr)
		return E_POINTER;
	if (key.fmtid != MFNETSOURCE_STATISTICS)
		return MF_E_NOT_FOUND;

	if (key.pid != MFNETSOURCE_BUFFERPROGRESS_ID && key.pid != MFNETSOURCE_BUFFERSIZE_ID)
		return MF_E_NOT_FOUND;

	PropVariantInit(pv);
	pv->vt = VT_I4;
	if (key.pid == MFNETSOURCE_BUFFERPROGRESS_ID)
		pv->lVal = _network_buffer_progress;
	else
		pv->lVal = (LONG)(_network_preroll_time * 1000.0);
	return S_OK;
}

unsigned HDMediaSource::QueryNetworkBufferProgressValue()
{
	unsigned result = 0;
	unsigned count = _streamList.Count();
	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		_streamList.GetAt(i,pStream.GetAddressOf());

		if (!pStream->IsActive())
			continue;
		result += pStream->QueryNetworkBufferProgressValue();
	}
	result /= _nPendingEOS;
	if (result >= 100)
		result = 99;

	DbgLogPrintf(L"%s::Network Buffering Progress %d.",L"HDMediaSource",result);
	return result;
}

HRESULT HDMediaSource::SendNetworkStartBuffering()
{
	DbgLogPrintf(L"%s::MEBufferingStarted %d...",L"HDMediaSource",(int)GetTickCount64());
	_network_buffering = true;
	return _pEventQueue->QueueEventParamVar(MEBufferingStarted,GUID_NULL,S_OK,nullptr);
}

HRESULT HDMediaSource::SendNetworkStopBuffering()
{
	DbgLogPrintf(L"%s::MEBufferingStopped %d.",L"HDMediaSource",(int)GetTickCount64());
	_network_buffering = false;
	return _pEventQueue->QueueEventParamVar(MEBufferingStopped,GUID_NULL,S_OK,nullptr);
}