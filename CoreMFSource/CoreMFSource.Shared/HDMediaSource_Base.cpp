#include "HDMediaSource.h"

HRESULT HDMediaSource::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == nullptr)
		return E_POINTER;

	*ppv = nullptr;
	if (iid == IID_IUnknown ||
		iid == IID_IMFMediaEventGenerator ||
		iid == IID_IMFMediaSource)
		*ppv = static_cast<IMFMediaSource*>(this);
	else if (iid == IID_IMFGetService)
		*ppv = static_cast<IMFGetService*>(this);
	else if (iid == IID_IMFRateControl)
		*ppv = static_cast<IMFRateControl*>(this);
	else if (iid == IID_IMFRateSupport)
		*ppv = static_cast<IMFRateSupport*>(this);
	else if (iid == IID_IMFMetadataProvider)
		*ppv = static_cast<IMFMetadataProvider*>(this);
	else if (iid == IID_IMFMetadata)
		*ppv = static_cast<IMFMetadata*>(this);
	else if (iid == IID_IPropertyStore)
		*ppv = static_cast<IPropertyStore*>(this);
	else if (iid == __uuidof(IMFSeekInfo))
		*ppv = static_cast<IMFSeekInfo*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}