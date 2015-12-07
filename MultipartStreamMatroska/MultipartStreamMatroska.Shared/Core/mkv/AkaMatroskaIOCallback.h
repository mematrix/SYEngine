#ifndef __AKA_MATROSKA_IO_CALLBACK_H
#define __AKA_MATROSKA_IO_CALLBACK_H

#include <cstdio>
#include <cstdint>

namespace AkaMatroska {
namespace Core {

	struct IOCallback
	{
		virtual unsigned Read(void* buf, unsigned size) { return 0; }
		virtual unsigned Write(const void* buf, unsigned size) = 0;
		
		virtual int64_t GetPosition() = 0; //same ftell
		virtual int64_t SetPosition(int64_t offset) = 0; //same lseek.(-1 is failed.)

		virtual int64_t GetSize() { return 0; }
	protected:
		virtual ~IOCallback() {}
	};
}}

#endif //__AKA_MATROSKA_IO_CALLBACK_H