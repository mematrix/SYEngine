#include "HDMediaStream.h"

void HDMediaStream::SetPrivateData(unsigned char* pb,unsigned len)
{
	_private_state = true;
	_private_data.Free();

	_private_data(len);
	memcpy(_private_data.Get(),pb,len);

	_private_size = len;
}