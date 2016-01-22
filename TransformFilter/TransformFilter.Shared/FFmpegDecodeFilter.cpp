#include "FFmpegDecodeFilter.h"
#include "FFmpegDecodeServices.h"
#include <new>

HRESULT FFmpegDecodeFilter::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	if (iid != IID_IUnknown &&
		iid != __uuidof(ITransformFilter))
		return E_NOINTERFACE;
	*ppv = this;
	AddRef();
	return S_OK;
}

HRESULT FFmpegDecodeFilter::GetService(REFIID riid, void** ppv)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (_services == NULL) {
		avcodec_register_all();
		auto p = new(std::nothrow) FFmpegDecodeServices();
		if (p == NULL)
			return E_OUTOFMEMORY;
		p->QueryInterface(IID_PPV_ARGS(&_services));
		p->Release();
	}

	if (ppv == NULL)
		return E_POINTER;
	if (_services == NULL)
		return E_ABORT;
	return _services->QueryInterface(riid, ppv);
}