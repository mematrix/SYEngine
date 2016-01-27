#ifndef __TRANSFORM_FILTER_H_
#define __TRANSFORM_FILTER_H_

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mferror.h>
#include <mftransform.h>

#ifndef _DEFINE_COM_INTERFACE
#define _DEFINE_COM_INTERFACE(n,x) class n : public x
#endif
#ifndef _DEFINE_COM_INTERFACE_IID
#define _DEFINE_COM_INTERFACE_IID(n,x,g) MIDL_INTERFACE(g) n : public x
#endif

_DEFINE_COM_INTERFACE_IID(ITransformFilter,IUnknown,"52C32D23-A134-4334-95F0-A63A2C58982B")
{
public:
	virtual STDMETHODIMP GetService(REFIID riid, void** ppv) PURE;
};

#endif //__TRANSFORM_FILTER_H_