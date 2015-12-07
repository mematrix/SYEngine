#ifndef __ES_PACKET_CUTTER_H
#define __ES_PACKET_CUTTER_H

#include "SimpleContainers/List.h"

class ESPacketCutter final
{
public:
	explicit ESPacketCutter(unsigned char sync_word) throw()
	{
		_syncWordSize = 1;
		_syncWord[0] = sync_word;
	}
	explicit ESPacketCutter(unsigned short sync_word) throw()
	{
		_syncWordSize = 2;
		*(unsigned short*)&_syncWord[0] = sync_word;
	}
	explicit ESPacketCutter(unsigned int synw_word) throw()
	{
		_syncWordSize = 4;
		*(unsigned int*)&_syncWord[0] = synw_word;
	}
	explicit ESPacketCutter(unsigned char* sync_word,unsigned word_size) throw()
	{
		_syncWordSize = word_size;
		memcpy(_syncWord,sync_word,word_size);
	}

public:
	bool Parse(unsigned char* buf,unsigned bufSize) throw();

	unsigned GetFrameCount() const throw() { return _frames.GetCount(); }
	bool GetFrame(unsigned index,unsigned* offset,unsigned* size) throw();

private:
	int FindNextSyncWord(unsigned char* buffer,unsigned size);

private:
	unsigned char _syncWord[64];
	unsigned _syncWordSize;

	struct Frame
	{
		unsigned offset;
		unsigned size;
	};
	AVUtils::List<Frame,10> _frames;
};

#endif //__ES_PACKET_CUTTER_H