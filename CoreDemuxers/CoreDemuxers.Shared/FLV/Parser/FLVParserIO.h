#ifndef __FLV_PARSER_IO_INTERFACE_H
#define __FLV_PARSER_IO_INTERFACE_H

namespace FLVParser{

class IFLVParserIO
{
public:
	virtual int Read(void*,int) = 0;
	virtual bool Seek(long long) = 0;
	virtual bool SeekByCurrent(long long)
	{
		return false; //not impl.
	}
	virtual long long GetSize()
	{
		return -1; //not impl.
	}
protected:
	virtual ~IFLVParserIO() {}
};

} //FLVParser namespace.

#endif //__FLV_PARSER_IO_INTERFACE_H