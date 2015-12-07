#include "FLVMediaFormat.h"

IAVMediaFormat* CreateFLVMediaDemuxer()
{
	IAVMediaFormat* parser = new (std::nothrow)FLVMediaFormat();
	return parser;
}

std::shared_ptr<IAVMediaFormat> CreateFLVMediaDemuxerSP()
{
	auto parser = std::make_shared<FLVMediaFormat>();
	return parser;
}

bool CheckFileStreamFLV(IAVMediaIO* pIo)
{
	if (!pIo->IsAliveStream())
		if (pIo->GetSize() < 128)
			return false;

	unsigned char head[4] = {};

	pIo->Seek(0,SEEK_SET);
	if (pIo->Read(&head,4) != 4)
		return false;

	if (head[0] == 'F' &&
		head[1] == 'L' &&
		head[2] == 'V' &&
		head[3] == 1)
		return true;

	return false;
}