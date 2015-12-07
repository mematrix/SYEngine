#include "HDMediaStream.h"

HRESULT HDMediaStream::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == nullptr)
		return E_POINTER;

	*ppv = nullptr;
	if (iid == IID_IUnknown)
		*ppv = static_cast<IUnknown*>(this);
	else if (iid == IID_IMFMediaEventGenerator)
		*ppv = static_cast<IMFMediaEventGenerator*>(this);
	else if (iid == IID_IMFMediaStream)
		*ppv = static_cast<IMFMediaStream*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}