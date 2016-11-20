#include "RingBlockBuffer.h"
#include "MemoryStream.h"
#include <stdlib.h>
#include <memory.h>
#include <new>

bool RingBlockBuffer::Initialize()
{
	if (_one_block_size < 1024 || _max_block_count < 2)
		return false;
	if (_blocks != NULL)
		return false;
	if (!AllocBlockList()) //��������block��list
		return false;
	_read_block_index = _write_block_index = 0; //��ʼ��ʱ��read��write����0
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
		Block* b = GetBlock(i); //ȡ����һ��block
		if (b->Stream->GetLength() == 0) //������blockû������
			break; //�÷�����

		unsigned len = b->Stream->GetLength() - b->ReadOffset; //ȡ�û��ܶ�����
		if (len == 0)
			break; //�����ǰ���block�Ѿ��е������Ѿ��������꣬��Ҫ��WriteBytes

		unsigned rs = len > pending_size ? pending_size : len; //ʵ��Ӧ�ö�����
		memcpy(pb, b->Stream->Buffer()->Get<char*>() + b->ReadOffset, rs); //��ȡ
		pb += rs;
		total_rs += rs; //�ܹ����˶���
		pending_size -= rs; //��ʣ��ö�����

		if (rs + b->ReadOffset == b->Stream->GetLength() && !b->WriteAllowFlag) {
			//������ζ��������������������block��
			//ˢ�����block������Ϊû�пɶ������ݣ���ҪWriteBytes��
			b->Stream->SetLength(0); //��տɶ�����
			b->ReadOffset = 0;
			b->WriteAllowFlag = true; //��д��
			_read_block_index = GetReadNextBlockIndex(); //����read��index����һ��
		}else{
			//������ζ���������û�����������block
			b->ReadOffset += rs; //��¼�����˶��ٵ�offset
		}

		if (pending_size == 0) //�ö��Ķ�ȫ��������
			break;
	}

	return total_rs;
}

unsigned RingBlockBuffer::WriteBytes(const void* buf, unsigned size)
{
	if (buf == NULL || size == 0)
		return 0;

	Block* b = GetBlock(_write_block_index); //ȡ�õ�ǰ����Ҫд��block
	if (!b->WriteAllowFlag) //�����ǰ��block�Ѿ�����д����ҪReadBytes���ģ�
		return 0; //����ʧ��
	
	unsigned wsize = 0; //�洢�ܹ�д�˶��٣�����ֵ
	Block* nb = GetNextWriteBlock(); //ȡ����һ��д��block
	//���㵱ǰblockдsize���������󣬻��Ƕ��
	unsigned block_write_after_size = b->Stream->GetLength() + size;
	//�ж������ǰblockдsize���������󣬲�������block_size��������һ��block����д��ֻ��д��ǰblock��
	if (block_write_after_size <= _one_block_size || !nb->WriteAllowFlag) {
		wsize = b->Stream->Write(buf, size); //д����ǰblock
		if (b->Stream->GetLength() >= _one_block_size) {
			//�����ǰblock�ĳ����Լ��������block_size
			b->WriteAllowFlag = false; //������д���block��
			_write_block_index = GetWriteNextBlockIndex(); //write��index�л�����һ��
		}
	}else{
		//���block_write_after_size������һ��block���ܴ洢��������һ��block��д
		//��ʹ�ö��block�����洢��ǰ��Ҫд���������
		wsize = b->Stream->Write(buf, _one_block_size - b->Stream->GetLength()); //��д����ǰ��block
		b->WriteAllowFlag = false; //��ǰblock����д��
		_write_block_index = GetWriteNextBlockIndex(); //�л�����һ��block

		const char* pb = (const char*)buf + wsize;
		unsigned pending_size = size - wsize; //����Ҫд����������
		if (pending_size > 0) {
			while(1) {
				b = GetBlock(_write_block_index); //��һ��block
				nb = GetNextWriteBlock(); //����һ��block

				if (!nb->WriteAllowFlag) {
					//����һ��block����д
					//����д����һ��block���˳�
					wsize += b->Stream->Write(pb, pending_size);
					if (b->Stream->GetLength() >= _one_block_size) {
						//�����һ��blockд��
						b->WriteAllowFlag = false;
						_write_block_index = GetWriteNextBlockIndex(); //�����л�����һ��block
					}
					break; //�˳�
				}

				//����һ��block����д�����
				unsigned b_max_written_size = _one_block_size - b->Stream->GetLength();
				unsigned want_ws = b_max_written_size > pending_size ? pending_size : b_max_written_size;
				wsize += b->Stream->Write(pb, want_ws); //���԰���һ��blockд��
				pending_size -= want_ws;
				if (b->Stream->GetLength() >= _one_block_size) {
					//��һ��blockд����
					b->WriteAllowFlag = false;
					_write_block_index = GetWriteNextBlockIndex(); //�л�������һ��block������ѭ��
				}

				if (pending_size == 0) //��д�Ķ�д���ˣ��˳�
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
		auto b = GetBlock(i); //ȡ��ÿһ��block
		if (b->Stream->GetLength() == 0) //�����һ��blockû���κ����ݣ���break
			break;
		size += (b->Stream->GetLength() - b->ReadOffset); //�ۼ�ÿ��block��������
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