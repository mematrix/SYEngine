#include "ThreadImpl.h"

struct ThreadParams
{
	ThreadImpl* Object;
	void* Userdata;
	HANDLE NoitfyEvent;
};

bool ThreadImpl::ThreadStart(void* param)
{
	if (_hThread != NULL)
		return false;

	ThreadParams p;
	p.Object = this;
	p.Userdata = param;
#ifdef CreateEventEx
	p.NoitfyEvent = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
#else
	p.NoitfyEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
#endif
	if (p.NoitfyEvent == NULL)
		return false;

	_hThread = CreateThread(NULL, 0, &ThreadImpl::DoThreadInvoke, &p, 0, NULL);
	WaitForSingleObjectEx(p.NoitfyEvent, INFINITE, FALSE);
	CloseHandle(p.NoitfyEvent);
	return _hThread != NULL;
}

void ThreadImpl::OnThreadInvoke(void* param)
{
	ThreadInvoke(param);
	CloseHandle(_hThread);
	_hThread = NULL;
	ThreadEnded();
}

DWORD WINAPI ThreadImpl::DoThreadInvoke(PVOID pv)
{
	ThreadParams p = *(ThreadParams*)pv;
	SetEvent(p.NoitfyEvent);
	((ThreadImpl*)p.Object)->OnThreadInvoke(p.Userdata);
	return GetCurrentThreadId();
}