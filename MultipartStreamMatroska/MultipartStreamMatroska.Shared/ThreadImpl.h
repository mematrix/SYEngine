/* ---------------------------------------------------------------
 -
 - 单线程回调父类，用于实现单线程任务。
 - 创建于：2015-09-18
 - 作者：ShanYe
 --------------------------------------------------------------- */

#pragma once

#include <Windows.h>

class ThreadImpl
{
protected:
	ThreadImpl() throw() : _hThread(NULL) {}
	~ThreadImpl() throw()
	{ if (_hThread) CloseHandle(_hThread); }

	bool ThreadStart(void* param = NULL);
	void ThreadWait(unsigned time = INFINITE)
	{ WaitForSingleObjectEx(_hThread, time, FALSE); }
	bool ThreadIsRun() const throw()
	{ return _hThread != NULL; }

	virtual void ThreadInvoke(void* param) = 0;
	virtual void ThreadEnded() {}

private:
	void OnThreadInvoke(void* param);

	HANDLE _hThread;
	static DWORD WINAPI DoThreadInvoke(PVOID pv);
};