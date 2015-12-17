#include "HDMediaSource.h"

HRESULT HDMediaSource::QueueAsyncOperation(SourceOperation::Operation opType) throw()
{
	ComPtr<SourceOperation> op;

	if (opType == SourceOperation::OP_REQUEST_DATA)
	{
		CritSec::AutoLock lock(_cs);

		//如果OnRequestSample已经在运行，直接返回
		if (_pSampleRequest != nullptr)
			return S_OK;
	}

	HRESULT hr = SourceOperation::CreateOperation(opType,op.GetAddressOf());
	if (FAILED(hr))
		return hr;

	return QueueOperation(op.Get());
}

HRESULT HDMediaSource::QueueOperation(SourceOperation* op)
{
	if (op == nullptr)
		return E_INVALIDARG;

	{
		CritSec::AutoLock lock(_cs);
		op->SetMagic(_taskMagicNumber);
	}

	return _taskWorkQueue.PutWorkItem(&_taskInvokeCallback,
		static_cast<IUnknown*>(op));

	/*
	return MFPutWorkItem2(_taskWorkQueue.GetWorkQueue(),
		0,&_taskInvokeCallback,
		static_cast<IUnknown*>(op));
	*/
}

HRESULT HDMediaSource::ValidateOperation(SourceOperation* op)
{
	CritSec::AutoLock lock(_cs);

	if (op->GetMagic() != _taskMagicNumber)
		return MF_E_NOTACCEPTING;

	return S_OK;
}

HRESULT HDMediaSource::DispatchOperation(SourceOperation* op)
{
	/*
#if defined(_DEBUG) && defined(_DESKTOP_APP)
	static DWORD GlobalOperationThreadId;
	if (GlobalOperationThreadId != GetCurrentThreadId())
	{
		GlobalOperationThreadId = GetCurrentThreadId();
		DbgLogPrintf(L"DispatchOperation Thread:%d.",GetCurrentThreadId());
	}
#endif
	*/

	{
		CritSec::AutoLock lock(_cs);

		if (CheckShutdown() == MF_E_SHUTDOWN)
			return S_OK;
	}

	HRESULT hr = S_OK;
	
	switch (op->GetOperation())
	{
	case SourceOperation::OP_OPEN:
		hr = DoOpen();
		break;
	case SourceOperation::OP_START:
		hr = DoStart(op);
		break;
	case SourceOperation::OP_PAUSE:
		hr = DoPause(op);
		break;
	case SourceOperation::OP_STOP:
		hr = DoStop(op);
		break;
	case SourceOperation::OP_SETRATE:
		hr = DoSetRate(op);
		break;
	case SourceOperation::OP_REQUEST_DATA:
		hr = OnRequestSample(op);
		break;
	case SourceOperation::OP_END_OF_STREAM:
		hr = OnEndOfStream(op);
		break;
	default:
		hr = E_UNEXPECTED;
	}

	if (FAILED(hr))
		ProcessOperationError(hr);

	return hr;
}

HRESULT HDMediaSource::ProcessOperationError(HRESULT hrStatus) throw()
{
	CritSec::AutoLock lock(_cs);

	if (_state == STATE_OPENING)
		return CompleteOpen(hrStatus);
	else if (_state != STATE_SHUTDOWN)
		return QueueEvent(MEError,GUID_NULL,hrStatus,nullptr);

	return S_OK;
}

HRESULT HDMediaSource::OnInvoke(IMFAsyncResult* pAsyncResult)
{
	if (pAsyncResult == nullptr)
		return E_UNEXPECTED;

	ComPtr<IUnknown> pUnk;
	HRESULT hr = pAsyncResult->GetState(pUnk.GetAddressOf());
	if (FAILED(hr))
		return hr;

	auto op = static_cast<SourceOperation*>(pUnk.Get());

	if (op->GetOperation() == SourceOperation::OP_REQUEST_DATA)
	{
		if (SUCCEEDED(ValidateOperation(op)))
			DispatchOperation(op);
	}else{
		if (SUCCEEDED(ValidateOperation(op)))
		{
			CritSec::AutoLock lock(_cs);
			DispatchOperation(op);
		}
	}

	return S_OK;
}