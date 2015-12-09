#include "MKVParser.h"

using namespace MKVParser;

bool MKVFileParser::zlibDecompress(unsigned char* inputData,unsigned inputSize,unsigned char** outputData,unsigned* outputSize)
{
#ifdef _MKV_PARSER_NOUSE_ZLIB
	return false;
#else
	z_stream zs = {};
	if (inflateInit(&zs) != Z_OK)
		return false;

	zs.next_in = inputData;
	zs.avail_in = inputSize;

	unsigned pktSize = inputSize;
	auto pktData = (unsigned char*)malloc(MKV_ALLOC_ALIGNED(pktSize));
	if (pktData == nullptr)
	{
		inflateEnd(&zs);
		return false;
	}

	unsigned count = 0;
	int result = Z_OK;
	while (1)
	{
		if (count > 10)
		{
			free(pktData);
			return false;
		}

		pktSize *= 3;
		pktData = (unsigned char*)realloc(pktData,MKV_ALLOC_ALIGNED(pktSize));
		if (pktData == nullptr)
		{
			free(pktData);
			inflateEnd(&zs);
			return false;
		}

		zs.avail_out = pktSize - zs.total_out;
		zs.next_out = pktData + zs.total_out;

		result = inflate(&zs,Z_NO_FLUSH);
		if (result != Z_OK)
			break;

		count++;
	}
	pktSize = zs.total_out;
	inflateEnd(&zs);

	if (result != Z_STREAM_END)
	{
		free(pktData);
		return false;
	}

	*outputData = pktData;
	*outputSize = pktSize;
	return true;
#endif
}