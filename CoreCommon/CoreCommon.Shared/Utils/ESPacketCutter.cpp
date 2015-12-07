#include "ESPacketCutter.h"

bool ESPacketCutter::Parse(unsigned char* buf,unsigned bufSize)
{
	if (buf == nullptr || bufSize < _syncWordSize)
		return false;
	_frames.ClearItems();

	int offset = FindNextSyncWord(buf,bufSize);
	if (offset == -1)
		return false;

	unsigned total_offset = offset;
	while (1)
	{
		Frame frame;
		frame.offset = total_offset;

		offset = FindNextSyncWord(buf + total_offset + _syncWordSize,bufSize - total_offset - _syncWordSize);
		if (offset != -1)
			frame.size = offset + _syncWordSize;
		else
			frame.size = bufSize - total_offset;

		if (offset != -1)
			total_offset += offset + _syncWordSize;

		if (!_frames.AddItem(&frame))
			return false;
		if (offset == -1)
			break;
	}

	return true;
}

bool ESPacketCutter::GetFrame(unsigned index,unsigned* offset,unsigned* size)
{
	if (offset == nullptr || size == nullptr)
		return false;
	if (index >= _frames.GetCount())
		return false;

	auto p =_frames[index];
	*offset = p->offset;
	*size = p->size;
	return true;
}

int ESPacketCutter::FindNextSyncWord(unsigned char* buffer,unsigned size)
{
	for (unsigned i = 0;i < (size - _syncWordSize);i++)
	{
		if (memcmp(buffer + i,_syncWord,_syncWordSize) == 0)
			return (int)i;
	}
	return -1;
}