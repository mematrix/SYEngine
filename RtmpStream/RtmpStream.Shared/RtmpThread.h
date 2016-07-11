// Copy from MultipartStreamMatroska
#pragma once

#include <Windows.h>

class RtmpThread {
protected:
    RtmpThread() throw() : _hThread(NULL) {}
    ~RtmpThread() throw()
    {
        if (_hThread) CloseHandle(_hThread);
    }

    bool ThreadStart(void* param = NULL);
    void ThreadWait(unsigned time = INFINITE)
    {
        WaitForSingleObjectEx(_hThread, time, FALSE);
    }
    bool ThreadIsRun() const throw()
    {
        return _hThread != NULL;
    }

    virtual void ThreadInvoke(void* param) = 0;
    virtual void ThreadEnded() {}

private:
    void OnThreadInvoke(void* param);

    HANDLE _hThread;
    static DWORD WINAPI DoThreadInvoke(PVOID pv);
};