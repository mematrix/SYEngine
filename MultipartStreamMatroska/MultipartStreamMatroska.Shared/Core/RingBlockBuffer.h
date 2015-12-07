#ifndef __RING_BLOCK_BUFFER_H
#define __RING_BLOCK_BUFFER_H

#include <stdio.h>

class MemoryStream;
class RingBlockBuffer
{
	struct Block
	{
		MemoryStream* Stream;
		unsigned ReadOffset;
		bool WriteAllowFlag;
	};

public:
	RingBlockBuffer() throw() : _blocks(NULL), _one_block_size(64 * 1024), _max_block_count(64) {}
	RingBlockBuffer(unsigned one_block_size_kb, unsigned max_block_count) throw() : _blocks(NULL)
	{ _one_block_size = one_block_size_kb * 1024; _max_block_count = max_block_count; }
	~RingBlockBuffer() throw() { Uninitialize(); }

	bool Initialize();
	bool Initialize(unsigned one_block_size_kb, unsigned max_block_count)
	{ _one_block_size = one_block_size_kb * 1024; _max_block_count = max_block_count; return Initialize(); }
	void Uninitialize()
	{ FreeBlockList(); }
	void Reinitialize();

	unsigned ReadBytes(void* buf, unsigned size);
	unsigned WriteBytes(const void* buf, unsigned size);

	unsigned GetReadableSize();
	bool IsBlockWriteable()
	{ return GetBlock(_write_block_index)->WriteAllowFlag; }

private:
	inline Block* GetBlock(unsigned index)
	{ return _blocks + index; }

	inline unsigned GetReadNextBlockIndex()
	{ return _read_block_index + 1 == _max_block_count ? 0 : _read_block_index + 1; }
	inline Block*  GetNextReadBlock()
	{ return GetBlock(GetReadNextBlockIndex()); }

	inline unsigned GetWriteNextBlockIndex()
	{ return _write_block_index + 1 == _max_block_count ? 0 : _write_block_index + 1; }
	inline Block*  GetNextWriteBlock()
	{ return GetBlock(GetWriteNextBlockIndex()); }

	bool AllocBlockList();
	void FreeBlockList();

private:
	Block* _blocks;
	unsigned _one_block_size;
	unsigned _max_block_count;

	unsigned _read_block_index, _write_block_index;
};

#endif //__RING_BLOCK_BUFFER_H