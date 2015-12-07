#include "AkaMatroskaEbml.h"

using namespace AkaMatroska;

int Core::CalcEbmlIdBytes(uint32_t id)
{
	//big-endian order.
	unsigned char* value = (unsigned char*)&id;
	for (int i = 0; i < 4; ++i) {
		if (*value != 0)
			break;
		value++;
	}
	static int magic[] = {128, 64, 32, 16, 8, 4, 2, 1};
	for (int i = 0; i < 8; i++) {
		if (((*value) & magic[i]) != 0)
			return i + 1;
	}
	return 0;
}

int Core::CalcEbmlSizeBytes(uint64_t num)
{
	//from libavformat(ffmpeg)
	int bytes = 1;
	while ((num + 1) >> bytes * 7)
		bytes++;
	return bytes;
}

bool Ebml::WriteId(Core::IOCallback* cb, uint32_t id)
{
	uint32_t temp = EBML_SWAP32(id);
	int len = Core::CalcEbmlIdBytes(temp);
	if (len == 0 || len > 4)
		return false;
	temp >>= ((4 - len) * 8);
	return cb->Write(&temp, len) == len;
}

bool Ebml::WriteSize(Core::IOCallback* cb, uint64_t size, int force_preserve_bytes)
{
	int needed_bytes = force_preserve_bytes ? force_preserve_bytes : Core::CalcEbmlSizeBytes(size);
	if (needed_bytes > 8)
		return false;
	uint64_t temp = size | 1ULL << needed_bytes * 7;
	temp = EBML_SWAP64(temp);
	temp >>= ((8 - needed_bytes) * 8);
	return cb->Write(&temp, needed_bytes) == needed_bytes;
}

bool Ebml::WriteSizeUnknown(Core::IOCallback* cb, int range_bytes)
{
	unsigned char size = (0x1FF >> range_bytes);
	cb->Write(&size, 1);
	static unsigned char temp[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	return cb->Write(&temp, range_bytes - 1) == (range_bytes - 1);
}

void Ebml::WriteVoid(Core::IOCallback* cb, unsigned free_size)
{
	Master Void(cb);
	unsigned char buf[256] = {};
	if (free_size <= 256) {
		cb->Write(&buf, free_size);
	}else{
		unsigned n = free_size / 256;
		unsigned mod_size = free_size % 256;
		for (unsigned i = 0; i < n; i++)
			cb->Write(&buf, 256);
		if (free_size > 0)
			cb->Write(&buf, free_size);
	}
}

void Ebml::Master::InitMaster(uint32_t id, uint64_t maybe_size) throw()
{
	int bytes = maybe_size ? Core::CalcEbmlSizeBytes(maybe_size) : 8;
	Ebml::WriteId(_callback, id);
	Ebml::WriteSizeUnknown(_callback, bytes);
	_cur_pos = _callback->GetPosition();
	_size_offset_bytes = bytes;
}

void Ebml::Master::Complete() throw()
{
	if (_callback == nullptr)
		return;

	int64_t pos = _callback->GetPosition();
	if (_callback->SetPosition(_cur_pos - _size_offset_bytes) <= 0)
		return;
	Ebml::WriteSize(_callback, pos - _cur_pos, _size_offset_bytes);
	_callback->SetPosition(pos);
	_callback = nullptr;
}

bool Ebml::Master::PutElementUInt(uint32_t eid, uint64_t value)
{
	int bytes = 1;
	uint64_t temp = value;
	while (temp >>= 8)
		bytes++;

	if (!Ebml::WriteId(_callback, eid) || !Ebml::WriteSize(_callback, bytes))
		return false;
	temp = EBML_SWAP64(value);
	temp >>= ((8 - bytes) * 8);
	return _callback->Write(&temp, bytes) == bytes;
}

bool Ebml::Master::PutElementSInt(uint32_t eid, int64_t value)
{
	int bytes = 1;
	uint64_t temp = 2 * (value < 0 ? value^-1 : value);;
	while (temp >>= 8)
		bytes++;

	if (!Ebml::WriteId(_callback, eid) || !Ebml::WriteSize(_callback, bytes))
		return false;
	temp = EBML_SWAP64(value);
	temp >>= ((8 - bytes) * 8);
	return _callback->Write(&temp, bytes) == bytes;
}

bool Ebml::Master::PutElementFloat(uint32_t eid, double value)
{
	if (!Ebml::WriteId(_callback, eid) || !Ebml::WriteSize(_callback, 8))
		return false;
	uint64_t temp = *(uint64_t*)&value;
	temp = EBML_SWAP64(temp);
	return _callback->Write(&temp, 8) == 8;
}

bool Ebml::Master::PutElementBinary(uint32_t eid, const void* data, unsigned size)
{
	if (!Ebml::WriteId(_callback, eid) || !Ebml::WriteSize(_callback, size))
		return false;
	return _callback->Write(data, size) == size;
}

bool Ebml::Master::PutElementBArray(uint32_t eid, const void* data[], unsigned size[], unsigned count)
{
	unsigned total_size = 0;
	for (unsigned i = 0; i < count; i++)
		total_size += size[i];

	if (!Ebml::WriteId(_callback, eid) || !Ebml::WriteSize(_callback, total_size))
		return false;

	for (unsigned i = 0; i < count; i++)
	{
		if (_callback->Write(data[i], size[i]) != size[i])
			return false;
	}
	return true;
}