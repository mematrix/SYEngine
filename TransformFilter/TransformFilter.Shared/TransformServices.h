#ifndef __TRANSFORM_SERVICES_H_
#define __TRANSFORM_SERVICES_H_

#include "TransformFilter.h"

_DEFINE_COM_INTERFACE_IID(ITransformAllocator,IUnknown,"90D4CA50-479B-4664-8008-CC8F88BF2607")
{
public:
	virtual STDMETHODIMP CreateSample(IMFSample** ppSample) PURE;
	virtual STDMETHODIMP IsUseDXVA(BOOL* bUseDXVA) PURE;
};

_DEFINE_COM_INTERFACE_IID(ITransformLoader,IUnknown,"91D4CA50-479B-4664-8008-CC8F88BF2607")
{
public:
	virtual STDMETHODIMP CheckMediaType(IMFMediaType* pMediaType) PURE;
	virtual STDMETHODIMP SetInputMediaType(IMFMediaType* pMediaType) PURE;
	virtual STDMETHODIMP GetOutputMediaType(IMFMediaType** ppMediaType) PURE;
	virtual STDMETHODIMP GetCurrentInputMediaType(IMFMediaType** ppMediaType) PURE;
	virtual STDMETHODIMP GetCurrentOutputMediaType(IMFMediaType** ppMediaType) PURE;
};

_DEFINE_COM_INTERFACE_IID(ITransformWorker,IUnknown,"92D4CA50-479B-4664-8008-CC8F88BF2607")
{
public:
	virtual STDMETHODIMP SetAllocator(ITransformAllocator* pAllocator) PURE;
	virtual STDMETHODIMP ProcessSample(IMFSample* pSample, IMFSample** ppNewSample) PURE;
	virtual STDMETHODIMP ProcessFlush() PURE;
};

#endif //__TRANSFORM_SERVICES_H_