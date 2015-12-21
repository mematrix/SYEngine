#include "HDMediaSource.h"

HRESULT SourceOperation::CreateOperation(SourceOperation::Operation op,SourceOperation** pp)
{
	if (pp == nullptr)
		return E_POINTER;

	auto pop = new (std::nothrow)SourceOperation(op);
	if (pop == nullptr)
		return E_OUTOFMEMORY;

	pop->AddRef();
	*pp = pop;

	return S_OK;
}

HRESULT SourceOperation::CreateStartOperation(IMFPresentationDescriptor* ppd,double seek_time,SourceOperation** pp)
{
	if (pp == nullptr)
		return E_POINTER;

	auto pop = new (std::nothrow)StartOperation(ppd,seek_time);
	if (pop == nullptr)
		return E_OUTOFMEMORY;

	pop->AddRef();
	*pp = pop;

	return S_OK;
}

HRESULT SourceOperation::CreateSetRateOperation(BOOL fThin,FLOAT flRate,SourceOperation** pp)
{
	if (pp == nullptr)
		return E_POINTER;

	auto pop = new (std::nothrow)SetRateOperation(fThin,flRate);
	if (pop == nullptr)
		return E_OUTOFMEMORY;

	pop->AddRef();
	*pp = pop;

	return S_OK;
}

ULONG SourceOperation::AddRef()
{
	return InterlockedIncrement(&RefCount);
}

ULONG SourceOperation::Release()
{
	ULONG _RefCount = InterlockedDecrement(&RefCount);
	if (_RefCount == 0)
		delete this;

	return _RefCount;
}

HRESULT SourceOperation::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == nullptr)
		return E_POINTER;

	if (iid != IID_IUnknown)
		return E_NOINTERFACE;

	*ppv = static_cast<IUnknown*>(this);
	AddRef();

	return S_OK;
}

SourceOperation::SourceOperation(Operation op) throw() : _op(op)
{
	PropVariantInit(&_data);
}

SourceOperation::~SourceOperation() throw()
{
	PropVariantClear(&_data);
}

HRESULT SourceOperation::SetData(const PROPVARIANT& var) throw()
{
	return PropVariantCopy(&_data,&var);
}

StartOperation::StartOperation(IMFPresentationDescriptor* ppd,double seek_time) throw() : SourceOperation(OP_START)
{
	if (ppd == nullptr)
		return;

	ppd->AddRef();
	_pd.Attach(ppd);
	_seek_to_time = seek_time;
}

HRESULT StartOperation::GetPresentationDescriptor(IMFPresentationDescriptor** pp) const throw()
{
	if (pp == nullptr)
		return E_POINTER;

	if (_pd == nullptr)
		return MF_E_INVALIDREQUEST;

	*pp = _pd.Get();
	(*pp)->AddRef();

	return S_OK;
}