#pragma once

#include "pch.h"
#include "MFMediaIO.h"
#include <Utils\SimpleContainers\Buffer.h>

class MFMediaIOEx : public IAVMediaIO, public IMFAsyncCallback
{
	ULONG _refCount;

	std::shared_ptr<MFMediaIO> _pReader;
	IMFByteStream* _pStream;
	HANDLE _hEventRead, _hEventCancel;

	AVUtils::Buffer _ioBuffer;
	DWORD _dwReadTimeout;
	DWORD _dwReadOKSize;
	bool _blockState;

	enum ReadResult
	{
		ReadOK,
		ReadCancel,
		ReadTimeout,
		ReadFailed
	};

public:
	MFMediaIOEx(IMFByteStream* pByteStream) throw() : _refCount(1)
	{
		_blockState = false;
		_dwReadTimeout = INFINITE;
		_hEventRead = _hEventCancel = nullptr;
		_pReader = std::make_shared<MFMediaIO>(pByteStream);
		_pStream = _pReader->GetMFByteStream();
	}
	virtual ~MFMediaIOEx() throw() { DestroyEvents(); }

public:
	unsigned Read(void* pb,unsigned size);

	unsigned Write(void* pb,unsigned size)
	{ return _pReader->Read(pb,size); }

	bool Seek(long long offset,int whence)
	{ return _pReader->Seek(offset,whence); }

	long long Tell()
	{ return _pReader->Tell(); }

	long long GetSize()
	{ return _pReader->GetSize(); }

	bool IsAliveStream()
	{ return _pReader->IsAliveStream(); }

	bool GetPlatformSpec(void** pp,int* spec_code)
	{ return _pReader->GetPlatformSpec(pp,spec_code); }

	inline void SetReadTimeout(unsigned timeout = INFINITE) { _dwReadTimeout = timeout; }
	inline void CancelReading() { if (_blockState) SetEvent(_hEventCancel); }

	inline IMFByteStream* GetMFByteStream() throw() { return _pStream; }

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef()
	{ return InterlockedIncrement(&_refCount); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_refCount); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

public: //IMFAsyncCallback
	STDMETHODIMP GetParameters(DWORD*,DWORD*) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult);

private:
	ReadResult GetReadResult();

	bool CreateEvents()
	{
		if (_hEventRead == nullptr)
		{
			_hEventRead = CreateEventExW(nullptr,nullptr,0,EVENT_ALL_ACCESS);
			if (_hEventRead == nullptr)
				return false;
		}
		if (_hEventCancel == nullptr)
		{
			_hEventCancel = CreateEventExW(nullptr,nullptr,0,EVENT_ALL_ACCESS);
			if (_hEventCancel == nullptr)
				return false;
		}
		return true;
	}

	void DestroyEvents()
	{
		if (_hEventRead)
			CloseHandle(_hEventRead);
		if (_hEventCancel)
			CloseHandle(_hEventCancel);
		_hEventRead = _hEventCancel = nullptr;
	}
};