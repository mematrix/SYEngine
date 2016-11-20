#ifndef __MEDIA_AV_IO_H
#define __MEDIA_AV_IO_H

#include <stdio.h>

#define RESET_AV_MEDIA_IO(x) ((x)->Seek(0,0))

class IAVMediaIO
{
public:
	virtual unsigned Read(void* pb,unsigned size) = 0;
	virtual unsigned Write(void* pb,unsigned size) = 0;
	virtual bool Seek(long long offset,int whence) = 0;
	virtual long long Tell() = 0;
	virtual long long GetSize() = 0;

    virtual void SetLiveStream() {}
	virtual bool IsLiveStream() { return false;  }
	virtual bool IsAliveStream() { return false; }

	virtual bool IsCancel() { return false; }

	virtual bool IsError() { return false; }
	virtual int GetErrorCode() { return -1; }

	virtual bool GetPlatformSpec(void** pp,int* spec_code) { return false; }
protected:
	virtual ~IAVMediaIO() {}
};

#endif //__MEDIA_AV_IO_H