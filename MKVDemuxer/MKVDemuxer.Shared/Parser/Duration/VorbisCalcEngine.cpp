#include "VorbisCalcEngine.h"
#include <memory.h>

static inline void InitOggPacket(ogg_packet& pkt,unsigned char* buf,unsigned len,bool first = false)
{
	memset(&pkt,0,sizeof(ogg_packet));
	pkt.b_o_s = first ? 1:0;
	pkt.bytes = len;
	pkt.packet = buf;
}

static int ParseOggVorbisUserdata(unsigned char* buf,unsigned size,unsigned* h1size,unsigned* h2size,unsigned* h3size)
{
	if (size < 5 || buf == nullptr)
		return 0;
	if (buf[0] != 2)
		return 0;
	if (buf[1] < 30 || buf[1] > 254)
		return 0;

	unsigned size2 = 0;
	unsigned p2size = 3;

	for (unsigned i = 2;i < (size / 2);i++)
	{
		size2 += buf[i];
		if (buf[i] != 0xFF)
			break;
		p2size++;
	}

	if (h1size)
		*h1size = buf[1];
	if (h2size)
		*h2size = size2;
	if (h3size)
		*h3size = size - size2 - buf[1] - p2size;

	return p2size;
}

bool VorbisDurationCalcParser::Initialize(unsigned char* extradata,unsigned len)
{
	memset(&_vi,0,sizeof(_vi));
	memset(&_vc,0,sizeof(_vc));

	if (extradata == nullptr || len < 100)
		return false;

	unsigned h1s = 0,h2s = 0,h3s = 0;
	int offset = ParseOggVorbisUserdata(extradata,len,&h1s,&h2s,&h3s);
	if (offset == 0)
		return false;
	if (h1s == 0 || h2s == 0 || h3s == 0)
		return false;

	ogg_packet pkt;
	InitOggPacket(pkt,extradata + offset,h1s,true);
	if (!vorbis_synthesis_idheader(&pkt))
		return false;

	vorbis_info_init(&_vi);
	vorbis_comment_init(&_vc);

	if (vorbis_synthesis_headerin(&_vi,&_vc,&pkt) != 0)
		return false;
	InitOggPacket(pkt,extradata + offset + h1s,h2s);
	if (vorbis_synthesis_headerin(&_vi,&_vc,&pkt) != 0)
		return false;
	InitOggPacket(pkt,extradata + offset + h1s + h2s,h3s);
	if (vorbis_synthesis_headerin(&_vi,&_vc,&pkt) != 0)
		return false;

	unsigned char bs = *(extradata + offset + 7 + 21);
	_blockSize0 = 1 << (bs & 0x0F);
	_blockSize1 = 1 << (bs >> 4);

	_prev_blocksize = _blockSize1;
	return true;
}

bool VorbisDurationCalcParser::Calc(unsigned char* buf,unsigned size,double* time)
{
	if (buf == nullptr || size == 0)
		return false;
	if (_vi.rate == 0)
		return false;

	ogg_packet pkt;
	InitOggPacket(pkt,buf,size);
	int bsize = (int)vorbis_packet_blocksize(&_vi,&pkt);
	int duration = (_prev_blocksize + bsize) >> 2;
	_prev_blocksize = bsize;

	*time = (double)duration / (double)_vi.rate;
	return true;
}