#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_AUTO_BUF_LOCK_H
#define __MF_AUTO_BUF_LOCK_H

#include "MFCommon.h"

namespace WMF{

class AutoBufLock final
{
	IMFMediaBuffer* _pMediaBuffer;
	PBYTE _pData;

public:
	explicit AutoBufLock(IMFMediaBuffer* pBuffer) throw() : _pMediaBuffer(nullptr), _pData(nullptr)
	{
		if (pBuffer != nullptr)
		{
			if (SUCCEEDED(pBuffer->Lock(&_pData,nullptr,nullptr)))
			{
				_pMediaBuffer = pBuffer;
				_pMediaBuffer->AddRef();
			}
		}
	}

	~AutoBufLock() throw()
	{
		Unlock();
	}

public:
	PBYTE GetPtr() const throw()
	{
		return _pData;
	}

	void Unlock() throw()
	{
		if (_pMediaBuffer)
		{
			_pMediaBuffer->Unlock();
			_pMediaBuffer->Release();
		}

		_pMediaBuffer = nullptr;
	}
};

} //namespace WMF.

#endif //__MF_AUTO_BUF_LOCK_H