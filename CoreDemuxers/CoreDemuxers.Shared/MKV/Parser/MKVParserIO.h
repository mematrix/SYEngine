#ifndef _MKV_PARSER_IO_H
#define _MKV_PARSER_IO_H

#include <stdio.h>

namespace MKVParser
{
	struct IMKVParserIO
	{
		virtual unsigned Read(void* buf,unsigned size) = 0;
		virtual bool Seek(long long offset = 0,int whence = SEEK_SET) = 0;
		virtual long long Tell() = 0;
		virtual long long GetSize() = 0;
	protected:
		virtual ~IMKVParserIO() {}
	};
}

#endif //_MKV_PARSER_IO_H