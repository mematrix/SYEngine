#include "MFMediaIOEx.h"

unsigned MFMediaIOEx::Read(void* pb,unsigned size)
{
	if (!CreateEvents())
		return 0;

	_dwReadOKSize = 0;
	_blockState = true;
	HRESULT hr = _pStream->BeginRead((BYTE*)pb,size,this,nullptr);
	if (FAILED(hr))
	{
		_blockState = false;
		return 0;
	}
	auto result = GetReadResult();
	_blockState = false;

	if (result != ReadOK || _dwReadOKSize == 0)
		return 0;
	return _dwReadOKSize;
}

MFMediaIOEx::ReadResult MFMediaIOEx::GetReadResult()
{
	HANDLE handles[] = {_hEventRead,_hEventCancel};
	DWORD result = WaitForMultipleObjectsEx(_countof(handles),handles,FALSE,_dwReadTimeout,FALSE);
	if (result == WAIT_TIMEOUT)
		return ReadTimeout;
	else if (result == WAIT_FAILED)
		return ReadFailed;
	return result == 0 ? ReadOK:ReadCancel;
}

HRESULT MFMediaIOEx::Invoke(IMFAsyncResult* pAsyncResult)
{
	_dwReadOKSize = 0;
	if (_pStream)
		_pStream->EndRead(pAsyncResult,(ULONG*)&_dwReadOKSize);
	SetEvent(_hEventRead);
	return S_OK;
}

HRESULT MFMediaIOEx::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == nullptr)
		return E_POINTER;
	if (iid != IID_IUnknown && iid != IID_IMFAsyncCallback)
		return E_NOINTERFACE;

	*ppv = static_cast<IMFAsyncCallback*>(this);
	AddRef();
	return S_OK;
}