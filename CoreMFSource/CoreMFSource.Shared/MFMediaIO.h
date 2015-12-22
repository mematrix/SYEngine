#pragma once

#include "pch.h"
#include "DbgLogOutput.h"
#include <MediaAVIO.h>

class MFMediaIO : public IAVMediaIO
{
	ComPtr<IMFByteStream> _pByteStream;
	bool _alive;

public:
	MFMediaIO(IMFByteStream* pByteStream) throw() : _alive(false)
	{
		pByteStream->AddRef();
		_pByteStream.Attach(pByteStream);

		ComPtr<IMFByteStreamBuffering> pBuffering;
		if (SUCCEEDED(pByteStream->QueryInterface(IID_PPV_ARGS(&pBuffering))))
			_alive = true;
		ComPtr<IMFByteStreamTimeSeek> pTimeSeek;
		if (SUCCEEDED(pByteStream->QueryInterface(IID_PPV_ARGS(&pTimeSeek))))
			_alive = true;
	}

public:
	unsigned Read(void* pb,unsigned size)
	{
		ULONG ulRead = 0;
		_pByteStream->Read((PBYTE)pb,size,&ulRead);
		return ulRead;
	}

	unsigned Write(void* pb,unsigned size)
	{
		ULONG ulWriten = 0;
		_pByteStream->Write((PBYTE)pb,size,&ulWriten);
		return ulWriten;
	}

	bool Seek(long long offset,int whence)
	{
		DbgLogPrintf(L"MFMediaIO::Seek -> offset=%d, whence=%d", (int)offset, whence);
		if (whence == SEEK_END)
		{
			QWORD qwSize = 0;
			_pByteStream->GetLength(&qwSize);

			HRESULT hr = _pByteStream->Seek(msoBegin,offset > 0 ? qwSize - offset:qwSize + offset,
				MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO,&qwSize);

			return SUCCEEDED(hr);
		}

		QWORD qw = 0;
		HRESULT hr = _pByteStream->Seek((MFBYTESTREAM_SEEK_ORIGIN)whence,offset,
			MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO,&qw);

		return SUCCEEDED(hr);
	}

	long long Tell()
	{
		QWORD qw = 0;
		if (FAILED(_pByteStream->GetCurrentPosition(&qw)))
			return -1;

		return (long long)qw;
	}

	long long GetSize()
	{
		QWORD qw = 0;
		if (FAILED(_pByteStream->GetLength(&qw)))
			return -1;

		return (long long)qw;
	}

	bool IsAliveStream() { return _alive; }

	bool GetPlatformSpec(void** pp,int* spec_code)
	{
		if (pp == nullptr &&
			spec_code == nullptr)
			return false;

		if (pp != nullptr)
			*pp = _pByteStream.Get(); //Non-AddRef.
		if (spec_code)
			*spec_code = 0x753159;
		return true;
	}

	inline IMFByteStream* GetMFByteStream() throw() { return _pByteStream.Get(); }
};