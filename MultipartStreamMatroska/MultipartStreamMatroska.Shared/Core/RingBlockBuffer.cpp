#include "RingBlockBuffer.h"
#include "MemoryStream.h"
#include <malloc.h>
#include <memory.h>
#include <new>

bool RingBlockBuffer::Initialize()
{
	if (_one_block_size < 1024 || _max_block_count < 2)
		return false;
	if (_blocks != NULL)
		return false;
	if (!AllocBlockList()) //申请所有block的list
		return false;
	_read_block_index = _write_block_index = 0; //开始的时候read和write都是0
	return true;
}

void RingBlockBuffer::Reinitialize()
{
	if (_blocks == NULL)
		return;

	_read_block_index = _write_block_index = 0;
	for (unsigned i = 0; i < _max_block_count; i++) {
		Block* b = GetBlock(i);
		b->WriteAllowFlag = true;
		b->ReadOffset = 0;
		b->Stream->SetLength(0);
	}
}

unsigned RingBlockBuffer::ReadBytes(void* buf, unsigned size)
{
	if (buf == NULL || size == 0)
		return 0;

	unsigned total_rs = 0;
	unsigned pending_size = size;
	char* pb = (char*)buf;
	for (unsigned i = _read_block_index; i < _max_block_count; i++) {
		Block* b = GetBlock(i); //取得下一个block
		if (b->Stream->GetLength() == 0) //如果这个block没有数据
			break; //该返回了

		unsigned len = b->Stream->GetLength() - b->ReadOffset; //取得还能读多少
		if (len == 0)
			break; //如果当前这个block已经有的数据已经被消耗完，需要等WriteBytes

		unsigned rs = len > pending_size ? pending_size : len; //实际应该读多少
		memcpy(pb, b->Stream->Buffer()->Get<char*>() + b->ReadOffset, rs); //读取
		pb += rs;
		total_rs += rs; //总共读了多少
		pending_size -= rs; //还剩余该读多少

		if (rs + b->ReadOffset == b->Stream->GetLength() && !b->WriteAllowFlag) {
			//如果本次读到的数据量消耗完这个block了
			//刷新这个block，设置为没有可读的数据（需要WriteBytes）
			b->Stream->SetLength(0); //清空可读数据
			b->ReadOffset = 0;
			b->WriteAllowFlag = true; //可写了
			_read_block_index = GetReadNextBlockIndex(); //更新read的index到下一个
		}else{
			//如果本次读的数据量没有消耗完这个block
			b->ReadOffset += rs; //记录消耗了多少的offset
		}

		if (pending_size == 0) //该读的都全部读到了
			break;
	}

	return total_rs;
}

unsigned RingBlockBuffer::WriteBytes(const void* buf, unsigned size)
{
	if (buf == NULL || size == 0)
		return 0;

	Block* b = GetBlock(_write_block_index); //取得当前的需要写的block
	if (!b->WriteAllowFlag) //如果当前的block已经不可写（需要ReadBytes消耗）
		return 0; //返回失败
	
	unsigned wsize = 0; //存储总共写了多少，返回值
	Block* nb = GetNextWriteBlock(); //取得下一个写的block
	//计算当前block写size的数据量后，会是多大
	unsigned block_write_after_size = b->Stream->GetLength() + size;
	//判断如果当前block写size的数据量后，并不大于block_size，或者下一个block不可写（只能写当前block）
	if (block_write_after_size <= _one_block_size || !nb->WriteAllowFlag) {
		wsize = b->Stream->Write(buf, size); //写到当前block
		if (b->Stream->GetLength() >= _one_block_size) {
			//如果当前block的长度以及大于最大block_size
			b->WriteAllowFlag = false; //不能再写这个block了
			_write_block_index = GetWriteNextBlockIndex(); //write的index切换到下一个
		}
	}else{
		//如果block_write_after_size数据量一个block不能存储，并且下一个block可写
		//则使用多个block分批存储当前需要写入的数据量
		wsize = b->Stream->Write(buf, _one_block_size - b->Stream->GetLength()); //先写满当前的block
		b->WriteAllowFlag = false; //当前block不能写了
		_write_block_index = GetWriteNextBlockIndex(); //切换到下一个block

		const char* pb = (const char*)buf + wsize;
		unsigned pending_size = size - wsize; //还需要写多少数据量
		if (pending_size > 0) {
			while(1) {
				b = GetBlock(_write_block_index); //下一个block
				nb = GetNextWriteBlock(); //下下一个block

				if (!nb->WriteAllowFlag) {
					//下下一个block不能写
					//则尝试写满下一个block后退出
					wsize += b->Stream->Write(pb, pending_size);
					if (b->Stream->GetLength() >= _one_block_size) {
						//如果下一个block写满
						b->WriteAllowFlag = false;
						_write_block_index = GetWriteNextBlockIndex(); //真正切换下下一个block
					}
					break; //退出
				}

				//下下一个block还能写的情况
				unsigned b_max_written_size = _one_block_size - b->Stream->GetLength();
				unsigned want_ws = b_max_written_size > pending_size ? pending_size : b_max_written_size;
				wsize += b->Stream->Write(pb, want_ws); //尝试把下一个block写满
				pending_size -= want_ws;
				if (b->Stream->GetLength() >= _one_block_size) {
					//下一个block写满了
					b->WriteAllowFlag = false;
					_write_block_index = GetWriteNextBlockIndex(); //切换到下下一个block，重新循环
				}

				if (pending_size == 0) //该写的都写完了，退出
					break;
			}
		}
	}

	return wsize;
}

unsigned RingBlockBuffer::GetReadableSize()
{
	unsigned size = 0;
	for (unsigned i = _read_block_index; i < _max_block_count; i++) {
		auto b = GetBlock(i); //取得每一个block
		if (b->Stream->GetLength() == 0) //如果有一个block没有任何数据，就break
			break;
		size += (b->Stream->GetLength() - b->ReadOffset); //累加每个block的数据量
	}
	return size;
}

bool RingBlockBuffer::ForwardSeekInReadableRange(unsigned skip_bytes)
{
	unsigned prev_total_size = 0;
	unsigned block_index = INT_MAX;
	for (unsigned i = _read_block_index; i < _max_block_count; i++) {
		auto b = GetBlock(i);
		if (b->Stream->GetLength() == 0)
			break;
		unsigned now_size = b->Stream->GetLength() - b->ReadOffset;
		if ((now_size + prev_total_size) >= skip_bytes) {
			block_index = i;
			break;
		}
		prev_total_size += now_size;
	}
	if (block_index == INT_MAX)
		return false;

	auto cur_block = GetBlock(block_index);
	unsigned cur_block_read_offset = skip_bytes - prev_total_size;
	if (cur_block->ReadOffset + cur_block_read_offset >= cur_block->Stream->GetLength())
		return false;

	for (unsigned i = _read_block_index; i < block_index; i++) {
		auto b = GetBlock(i);
		b->Stream->SetLength(0);
		b->ReadOffset = 0;
		b->WriteAllowFlag = true;
	}
	cur_block->ReadOffset += cur_block_read_offset;
	_read_block_index = block_index;
	return true;
}

bool RingBlockBuffer::AllocBlockList()
{
	_blocks = (Block*)calloc(_max_block_count + 1, sizeof(Block));
	if (_blocks == NULL)
		return false;

	for (unsigned i = 0; i < _max_block_count; i++) {
		Block* b = GetBlock(i);
		b->WriteAllowFlag = true;
		b->Stream = new(std::nothrow) MemoryStream();
		if (b->Stream == NULL)
			return false;
	}
	return true;
}

void RingBlockBuffer::FreeBlockList()
{
	if (_blocks) {
		for (unsigned i = 0; i < _max_block_count; i++)
			delete GetBlock(i)->Stream;
		free(_blocks);
		_blocks = NULL;
	}
}