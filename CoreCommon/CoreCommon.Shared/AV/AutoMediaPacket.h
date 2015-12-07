#ifndef __AUTO_MEDIA_PACKET_H
#define __AUTO_MEDIA_PACKET_H

#include "MediaPacket.h"

class CAutoMediaPacket final
{
	AVMediaPacket _packet;

public:
	explicit CAutoMediaPacket(unsigned size) throw()
	{
		AllocMediaPacket(&_packet,size);
	}

	explicit CAutoMediaPacket(void* pd,unsigned size) throw()
	{
		AllocMediaPacketAndCopy(&_packet,pd,size);
	}

	explicit CAutoMediaPacket(AVMediaPacket& other) throw()
	{
		_packet = other;
	}

	explicit CAutoMediaPacket(const AVMediaPacket* other) throw()
	{
		if (other == nullptr)
			return;

		CopyMediaPacket((AVMediaPacket*)other,&_packet);
	}

	~CAutoMediaPacket() throw()
	{
		FreeMediaPacket(&_packet);
	}

public:
	AVMediaPacket* Get() throw()
	{
		return &_packet;
	}

	const AVMediaPacket* Get() const throw()
	{
		return &_packet;
	}
};

#endif //__AUTO_MEDIA_PACKET_H