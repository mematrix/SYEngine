#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_WORK_QUEUE_H
#define __MF_WORK_QUEUE_H

#include "MFCommon.h"

namespace WMF{

class AutoWorkQueue final
{
public:
	AutoWorkQueue() throw()
	{
		HRESULT hr = MFLockPlatform();
		if (SUCCEEDED(hr))
			hr = MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD,&_dwWorkQueue);

		if (FAILED(hr))
			_dwWorkQueue = MFASYNC_CALLBACK_QUEUE_STANDARD;
	}

	~AutoWorkQueue() throw()
	{
		if (_dwWorkQueue != MFASYNC_CALLBACK_QUEUE_STANDARD)
		{
			MFUnlockWorkQueue(_dwWorkQueue);
			MFUnlockPlatform();
		}
	}

public:
	inline DWORD GetWorkQueue() const throw()
	{
		return _dwWorkQueue;
	}

	inline HRESULT PutWorkItem(IMFAsyncCallback* pCallback,IUnknown* pState) const throw()
	{
		return MFPutWorkItem2(_dwWorkQueue,0,
			pCallback,pState);
	}

private:
	DWORD _dwWorkQueue;
};

} //namespace WMF.

#endif //__MF_WORK_QUEUE_H