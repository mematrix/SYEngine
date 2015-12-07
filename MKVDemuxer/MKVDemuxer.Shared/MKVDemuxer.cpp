#include "MKVMediaFormat.h"

IAVMediaFormat* CreateMKVMediaDemuxer()
{
	IAVMediaFormat* parser = new (std::nothrow)MKVMediaFormat();
	return parser;
}

std::shared_ptr<IAVMediaFormat> CreateMKVMediaDemuxerSP()
{
	auto parser = std::make_shared<MKVMediaFormat>();
	return parser;
}

bool CheckFileStreamMKV(IAVMediaIO* pIo)
{
	if (!pIo->IsAliveStream())
		if (pIo->GetSize() < 128)
			return false;

	unsigned char head[4] = {};

	pIo->Seek(0,SEEK_SET);
	if (pIo->Read(&head,4) != 4)
		return false;

	if (head[0] == 0x1A &&
		head[1] == 0x45 &&
		head[2] == 0xDF &&
		head[3] == 0xA3)
		return true;

	return false;
}