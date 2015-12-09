#include "IsomParser.h"

using namespace Isom;

bool Parser::StmReadUI64(uint64_t* value)
{
	uint64_t temp;
	if (_stream->Read(&temp, 8) != 8)
		return false;
	*value = ISOM_SWAP64(temp);
	return true;
}

bool Parser::StmReadUI32(uint32_t* value)
{
	uint32_t temp;
	if (_stream->Read(&temp, 4) != 4)
		return false;
	*value = ISOM_SWAP32(temp);
	return true;
}

bool Parser::StmReadUI24(uint32_t* value)
{
	uint32_t temp;
	if (_stream->Read(&temp, 3) != 3)
		return false;
	temp <<= 8;
	*value = ISOM_SWAP32(temp);
	return true;
}

bool Parser::StmReadUI16(uint32_t* value)
{
	uint16_t temp;
	if (_stream->Read(&temp, 2) != 2)
		return false;
	*value = (uint32_t)(ISOM_SWAP16(temp));
	return true;
}

bool Parser::StmReadUI08(uint32_t* value)
{
	uint8_t temp;
	if (_stream->Read(&temp, 1) != 1)
		return false;
	*value = temp;
	return true;
}

bool Parser::StmReadSkip(unsigned size)
{
	if (size > 128)
		return false;
	char buf[128];
	return _stream->Read(buf, size) == size;
}