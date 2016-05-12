#include "MediaPacket.h"
#include "MediaMemory.h"
#include <memory.h>

unsigned AllocMediaPacket(AVMediaPacket* packet,unsigned size)
{
	if (packet == nullptr ||
		size == 0)
		return 0;

	memset(packet,0,sizeof(AVMediaPacket));
	
	if (!InitMediaBuffer(&packet->data,size))
		return 0;

	packet->stream_index = -1;
	packet->dts = packet->pts = PACKET_NO_PTS;
	return size;
}

unsigned AllocMediaPacketAndCopy(AVMediaPacket* packet,void* pd,unsigned size)
{
	if (!AllocMediaPacket(packet,size))
		return 0;

	memcpy(packet->data.buf,pd,size);
	return size;
}

bool CopyMediaPacket(AVMediaPacket* old_packet,AVMediaPacket* new_packet)
{
	if (old_packet == nullptr ||
		new_packet == nullptr)
		return false;

	if (old_packet->data.size == 0)
		return false;

	FreeMediaPacket(new_packet);

	*new_packet = *old_packet;
	new_packet->data.buf = new_packet->side_data.buf = nullptr;
	new_packet->data.size = new_packet->side_data.size = 0;

	if (old_packet->data.buf)
	{
		if (!InitMediaBuffer(&new_packet->data,old_packet->data.size))
			return false;
	}
	if (old_packet->side_data.buf)
	{
		if (!InitMediaBuffer(&new_packet->side_data,old_packet->side_data.size))
			return false;
	}

	memcpy(new_packet->data.buf,old_packet->data.buf,old_packet->data.size);
	memcpy(new_packet->side_data.buf,old_packet->side_data.buf,old_packet->side_data.size);

	new_packet->data.size = old_packet->data.size;
	new_packet->side_data.size = old_packet->side_data.size;
	return true;
}

bool ShrinkMediaPacket(AVMediaPacket* packet,unsigned new_data_size)
{
	if (packet == nullptr ||
		new_data_size == 0)
		return false;

	if (packet->data.buf == nullptr)
		return false;
	if (new_data_size > packet->data.size)
		return false;
	if (new_data_size == packet->data.size)
		return true;
	if ((packet->flag & MEDIA_PACKET_FIXED_BUFFER_FLAG) != 0)
		return false;

	AVMediaBuffer buffer = {};
	if (!InitMediaBuffer(&buffer,new_data_size))
		return false;

	memcpy(buffer.buf,packet->data.buf,new_data_size);
	
	FreeMediaBuffer(&packet->data);
	packet->data = buffer;

	return true;
}

bool GrowMediaPacket(AVMediaPacket* packet,unsigned add_size)
{
	if (packet == nullptr)
		return false;

	if (packet->data.buf == nullptr)
		return false;
	if (add_size == 0)
		return true;
	if ((packet->flag & MEDIA_PACKET_FIXED_BUFFER_FLAG) != 0)
		return false;

	if (!AdjustMediaBufferSize(&packet->data,packet->data.size + add_size))
		return false;

	return true;
}

void FreeMediaPacket(AVMediaPacket* packet)
{
	if (packet == nullptr)
		return;
	
	if ((packet->flag & MEDIA_PACKET_FIXED_BUFFER_FLAG) == 0)
	{
		if (packet->data.buf != nullptr)
			FreeMediaBuffer(&packet->data);
		if (packet->side_data.buf != nullptr)
			FreeMediaBuffer(&packet->side_data);
	}

	memset(packet,0,sizeof(AVMediaPacket));
	packet->stream_index = -1;
}