#include "FFmpegDecodeFilter.h"
#include <new>

HRESULT WINAPI CreateAVCodecTransformFilter(IUnknown** ppunk)
{
	if (ppunk == NULL)
		return E_INVALIDARG;

	auto p = new(std::nothrow) FFmpegDecodeFilter();
	if (p == NULL)
		return E_OUTOFMEMORY;

	*ppunk = static_cast<IUnknown*>(p);
	return S_OK;
}