#ifndef __ISOM_STREAM_SOURCE_H_
#define __ISOM_STREAM_SOURCE_H_

#include <cstdint>

namespace Isom
{
	struct IStreamSource
	{
		virtual unsigned Read(void* buf, unsigned size) = 0;
		virtual unsigned Write(void* buf, unsigned size) { return 0; }
		
		virtual bool Seek(int64_t pos) = 0;
		virtual int64_t Tell() = 0;
		virtual int64_t GetSize() = 0;

		virtual bool IsSeekable() { return true; }
		virtual bool IsFromNetwork() { return false; }
	protected:
		virtual ~IStreamSource() {}
	};
}

#endif //__ISOM_STREAM_SOURCE_H_