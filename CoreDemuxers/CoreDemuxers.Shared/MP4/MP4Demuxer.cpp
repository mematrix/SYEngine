#include "MP4MediaFormat.h"

IAVMediaFormat* CreateMP4MediaDemuxer()
{
	IAVMediaFormat* parser = new (std::nothrow)MP4MediaFormat();
	return parser;
}

std::shared_ptr<IAVMediaFormat> CreateMP4MediaDemuxerSP()
{
	auto parser = std::make_shared<MP4MediaFormat>();
	return parser;
}

bool CheckFileStreamMP4(IAVMediaIO* pIo)
{
	if (!pIo->IsAliveStream())
		if (pIo->GetSize() < 128)
			return false;

	unsigned size = 0;
	unsigned head = 0;
	pIo->Seek(0,SEEK_SET);
	if (pIo->Read(&size,4) != 4)
		return false;
	if (pIo->Read(&head,4) != 4)
		return false;

	size = ISOM_SWAP32(size);
	if (head == ISOM_FCC('ftyp') ||
		head == ISOM_FCC('moov') ||
		head == ISOM_FCC('mdat') ||
		head == ISOM_FCC('free') ||
		head == ISOM_FCC('wide') ||
		head == ISOM_FCC('pnot') ||
		head == ISOM_FCC('skip'))
		if (size > 8)
			return true;

	return false;
}