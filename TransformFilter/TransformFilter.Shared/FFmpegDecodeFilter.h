#pragma once

#include "TransformFilter.h"
#include <mutex>

class FFmpegDecodeFilter : public ITransformFilter
{
public:
	FFmpegDecodeFilter() : _ref_count(1), _services(NULL) {}
	virtual ~FFmpegDecodeFilter() { if (_services) _services->Release(); }

public:
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

	STDMETHODIMP GetService(REFIID riid, void** ppv);

private:
	ULONG _ref_count;

	IUnknown* _services;
	std::recursive_mutex _mutex;
};