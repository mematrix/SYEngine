#include "IsomCodec.h"
#include "IsomTypes.h"

#define ESDS_TAG_ES_DESC 3
#define ESDS_TAG_DEC_CFG_DESC 4
#define ESDS_TAG_DEC_SPEC_DESC 5

using namespace Isom;
using namespace Isom::Types;

static const unsigned GlobalAudioTags[] = {
	isom_box_codec_mp4a, isom_box_codec_alac, isom_box_codec_mp3,
	isom_box_codec_ac3, isom_box_codec_eac3, isom_box_codec_sac3,
	isom_box_codec_samr, isom_box_codec_sawb, isom_box_codec_sawp,
	isom_box_codec_alaw, isom_box_codec_ulaw,
	isom_box_codec_spex, isom_box_codec_lpcm,
	isom_box_codec_g719, isom_box_codec_g726,
	isom_box_codec_twos, isom_box_codec_raw,
	isom_box_codec_mlpa, isom_box_codec_Opus
};
static const unsigned GlobalVideoTags[] = {
	isom_box_codec_avc1, isom_box_codec_hvc1, isom_box_codec_hev1,
	isom_box_codec_mp4v, isom_box_codec_divx, isom_box_codec_xvid,
	isom_box_codec_h263, isom_box_codec_s263, isom_box_codec_H263,
	isom_box_codec_3iv1, isom_box_codec_3iv2, isom_box_codec_3ivd,
	isom_box_codec_div3, isom_box_codec_cvid,
	isom_box_codec_m1v, isom_box_codec_m1v1,
	isom_box_codec_mpeg, isom_box_codec_mp1v,
	isom_box_codec_m2v1, isom_box_codec_mp2v,
	isom_box_codec_vp31, isom_box_codec_ffv1,
	isom_box_codec_drac, isom_box_codec_mjp2,
	isom_box_codec_svc1, isom_box_codec_svc2,
	isom_box_codec_avs2, isom_box_codec_vc1
};

bool CodecSampleEntryParser::ParseSampleEntry(unsigned type, const unsigned char* data, unsigned size) throw()
{
	_info.MainFcc = type;
	unsigned offset = 0;
	if (IsAudioEntry(type))
		offset = InitAudioSampleEntry(type, data, size);
	else if (IsVideoEntry(type))
		offset = InitVideoSampleEntry(type, data, size);

	if (offset > 0)
	{
		ScanChildBoxToList(data + offset, size - offset);
		if (_boxs.GetCount() > 0)
			ProcessChildBoxs();
	}
	return true;
}

unsigned CodecSampleEntryParser::InitAudioSampleEntry(unsigned type, const unsigned char* data, unsigned size)
{
	unsigned offset = 0;
	if (size < 28)
		return 0;

	offset = 28;
	uint16_t qt_ver = *(uint16_t*)(data + 8);
	qt_ver = ISOM_SWAP16(qt_ver);
	_info.Audio.QT_Version = qt_ver;

	uint16_t nch = *(uint16_t*)(data + 16);
	uint16_t bit = *(uint16_t*)(data + 18);
	uint32_t rate = *(uint32_t*)(data + 22);
	_info.Audio.ChannelCount = ISOM_SWAP16(nch);
	_info.Audio.SampleSize = ISOM_SWAP16(bit);
	_info.Audio.SampleRate = ISOM_SWAP32(rate);

	//from android-stagefright.
	if (type == isom_box_codec_samr)
	{
		_info.Audio.ChannelCount = 1;
		_info.Audio.SampleRate = 8000;
	}else if (type == isom_box_codec_sawb)
	{
		_info.Audio.ChannelCount = 1;
		_info.Audio.SampleRate = 16000;
	}

	unsigned qt_v2_ext_size = 0;
	if (qt_ver == 2 && size > 40)
	{
		qt_v2_ext_size = *(unsigned*)(data + 28);
		qt_v2_ext_size = ISOM_SWAP32(qt_v2_ext_size);
		if (qt_v2_ext_size <= 72)
			qt_v2_ext_size = 0;
		else
			qt_v2_ext_size -= 72;
	}

	if (qt_ver == 1)
	{
		if (size >= (offset + 16))
		{
			_info.Audio.QT_SamplesPerFrame = *(uint32_t*)(data + offset);
			_info.Audio.QT_BytesPerFrame = *(uint32_t*)(data + offset + 8);
		}
		offset += 16;
	}else if (qt_ver == 2) {
		if (size >= (offset + 36))
		{
			_info.Audio.QT_SamplesPerFrame = *(uint32_t*)(data + offset + 28);
			_info.Audio.QT_BytesPerFrame = *(uint32_t*)(data + offset + 32);
		}
		offset += 36 + qt_v2_ext_size;
	}
	if (qt_ver > 0)
	{
		_info.Audio.QT_BytesPerFrame = ISOM_SWAP32(_info.Audio.QT_BytesPerFrame);
		_info.Audio.QT_SamplesPerFrame = ISOM_SWAP32(_info.Audio.QT_SamplesPerFrame);
	}

	if (offset >= size)
		offset = 0;
	return offset;
}

unsigned CodecSampleEntryParser::InitVideoSampleEntry(unsigned type, const unsigned char* data, unsigned size)
{
	unsigned offset = 0;
	if (size < 78)
		return 0;

	offset = 78;
	uint16_t w = *(uint16_t*)(data + 0x18);
	uint16_t h = *(uint16_t*)(data + 0x1A);
	uint32_t hr = *(uint32_t*)(data + 0x1C);
	uint32_t vr = *(uint32_t*)(data + 0x10);
	uint16_t fc = *(uint16_t*)(data + 0x28);
	uint16_t dp = *(uint16_t*)(data + 0x4A);
	_info.Video.Width = ISOM_SWAP16(w);
	_info.Video.Height = ISOM_SWAP16(h);
	_info.Video.HorizResolution = ISOM_SWAP32(hr);
	_info.Video.VertResolution = ISOM_SWAP32(vr);
	_info.Video.FrameCount = ISOM_SWAP16(fc);
	_info.Video.Depth = ISOM_SWAP16(dp);

	/*
	if (_info.Video.Width == 0)
		_info.Video.Width = 352;
	if (_info.Video.Height == 0)
		_info.Video.Height = 288;
	*/
	return offset;
}

void CodecSampleEntryParser::ScanChildBoxToList(const unsigned char* data, unsigned size)
{
	if (size < 9)
		return;
	auto p = data;
	auto p_end = data + size;
	while ((p < p_end) && (p_end - p > 8))
	{
		unsigned sz = *(unsigned*)p;
		sz = ISOM_SWAP32(sz);
		p += 4;
		unsigned type = *(unsigned*)p;
		p += 4;
		if (sz <= 8)
			continue;
		sz -= 8;

		sz = p_end - p < (int)sz ? p_end - p:sz; //min
		if (sz > 0)
		{
			SubBox b = {};
			b.DataSize = sz;
			b.DataType = type;
			b.DataPointer = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(sz));
			if (b.DataPointer)
				memcpy(b.DataPointer, p, sz);
			if (!_boxs.Add(&b))
				break;
		}
		p += sz;
	}
}

void CodecSampleEntryParser::ProcessChildBoxs()
{
	for (unsigned i = 0; i < _boxs.GetCount(); i++)
	{
		auto p = _boxs[i];
		if (p->DataPointer)
		{
			if (p->DataType == ISOM_FCC('avcC') || p->DataType == ISOM_FCC('hvcC') ||
				p->DataType == ISOM_FCC('alac') || p->DataType == ISOM_FCC('esds') ||
				p->DataType == ISOM_FCC('d263') || p->DataType == ISOM_FCC('damr') ||
				p->DataType == ISOM_FCC('dac3') || p->DataType == ISOM_FCC('dec3') ||
				p->DataType == ISOM_FCC('m4ds') || p->DataType == ISOM_FCC('dvc1') ||
				p->DataType == ISOM_FCC('mvcC') || p->DataType == ISOM_FCC('svcC')) {
				if (_info.CodecPrivateSize == 0) {
					_info.CodecId = p->DataType;
					_info.CodecPrivateSize = p->DataSize;
					_info.CodecPrivate = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(p->DataSize));
					if (_info.CodecPrivate)
					{
						memcpy(_info.CodecPrivate, p->DataPointer, p->DataSize);
						if (p->DataType == ISOM_FCC('esds'))
							ParseEsdsCodecPrivate();
					}
				}
			}
			if (p->DataType == ISOM_FCC('pasp'))
				ProcessVideoPasp(i);
			//else if (p->DataType == ISOM_FCC('btrt'))
				//ProcessVideoBitrate(i);

			if (_boxs.GetCount() == 1 && _info.MainFcc == ISOM_FCC('mp4a') &&
				p->DataType != ISOM_FCC('esds') && _info.CodecPrivate == nullptr) {
				_info.CodecId = ISOM_FCC('esds');
				_info.CodecPrivateSize = p->DataSize;
				_info.CodecPrivate = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(p->DataSize));
				if (_info.CodecPrivate)
				{
					memcpy(_info.CodecPrivate, p->DataPointer, p->DataSize);
					ParseEsdsCodecPrivate();
				}
			}
			free(p->DataPointer);
		}
	}
	_boxs.Clear();
}

void CodecSampleEntryParser::ProcessVideoPasp(unsigned index)
{
	auto p = _boxs[index];
	if (p->DataSize >= 8)
	{
		_info.Video.ParNum = *(unsigned*)p->DataPointer;
		_info.Video.ParDen = *(unsigned*)(p->DataPointer + 4);
		_info.Video.ParNum = ISOM_SWAP32(_info.Video.ParNum);
		_info.Video.ParDen = ISOM_SWAP32(_info.Video.ParDen);
	}
}

void CodecSampleEntryParser::ParseEsdsCodecPrivate()
{
	if (_info.esds.ObjectTypeIndication > 0)
		return;
	if (_info.CodecPrivateSize > 4)
		ProcessEsdsBoxs(_info.CodecPrivate + 4, _info.CodecPrivateSize - 4);
}

void CodecSampleEntryParser::ProcessEsdsBoxs(const unsigned char* esds, unsigned size)
{
	auto p = esds;
	auto end = esds + size;
	while (p < end)
	{
		unsigned tag = *p;
		p++;
		if (p >= end)
			break;

		unsigned len = 0;
		unsigned offset = GetEsdsDescLen(p, end - p, &len);
		if (offset == 0)
			break;
		p += offset;
		if (p >= end)
			break;

		if (p + len <= end)
		{
			if (tag != ESDS_TAG_ES_DESC && tag != ESDS_TAG_DEC_CFG_DESC && tag != ESDS_TAG_DEC_SPEC_DESC)
			{
				p += len;
				if (p < end)
					ProcessEsdsBoxs(p, end - p);
			}else{
				unsigned ofc = 0;
				switch (tag)
				{
				case ESDS_TAG_ES_DESC:
					ofc = OnEsdsESDescriptor(p, len);
					break;
				case ESDS_TAG_DEC_CFG_DESC:
					ofc = OnEsdsDecoderConfigDescriptor(p, len);
					break;
				case ESDS_TAG_DEC_SPEC_DESC:
					ofc = OnEsdsDecoderSpecificInfo(p, len);
					break;
				}
				p += ofc;
				if (p < end)
					ProcessEsdsBoxs(p, end - p);
			}
		}
	}
}

unsigned CodecSampleEntryParser::OnEsdsESDescriptor(const unsigned char* data, unsigned size)
{
	if (size < 3)
		return size;

	unsigned result = 3;
	int flags = data[2];
	if (flags & 0x80) //streamDependenceFlag
		result += 2;
	if (flags & 0x40) //URL_Flag
	{
		if (result >= size)
			return size;
		unsigned pd = data[result];
		result += (pd + 1);
	}
	if (flags & 0x20) //OCRstreamFlag
		result += 2;

	return result;
}

unsigned CodecSampleEntryParser::OnEsdsDecoderConfigDescriptor(const unsigned char* data, unsigned size)
{
	if (size < 13)
		return size;

	_info.esds.ObjectTypeIndication = data[0];
	_info.esds.StreamType = data[1];
	_info.esds.MaxBitrate = *(unsigned*)(data + 5);
	_info.esds.AvgBitrate = *(unsigned*)(data + 9);
	_info.esds.MaxBitrate = ISOM_SWAP32(_info.esds.MaxBitrate);
	_info.esds.AvgBitrate = ISOM_SWAP32(_info.esds.AvgBitrate);
	return 13;
}

unsigned CodecSampleEntryParser::OnEsdsDecoderSpecificInfo(const unsigned char* data, unsigned size)
{
	if (size > 0)
	{
		if (_info.esds.DecoderConfig != NULL)
			free(_info.esds.DecoderConfig);
		_info.esds.DecoderConfigSize = size;
		_info.esds.DecoderConfig = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(size));
		if (_info.esds.DecoderConfig)
			memcpy(_info.esds.DecoderConfig, data, size);
	}
	return size;
}

unsigned CodecSampleEntryParser::GetEsdsDescLen(const unsigned char* data, unsigned size, unsigned* len)
{
	int ret = 0;
	unsigned count = size > 4 ? 4 : size;
	if (count == 0)
		return 0;

	unsigned i;
	for (i = 0; i < count; i++)
	{
		int c = data[i];
		ret = (ret << 7) | (c & 0x7F);
		if (!(c & 0x80))
			break;
	}
	*len = ret;
	return i + 1;
}

bool CodecSampleEntryParser::IsAudioEntry(unsigned type) throw()
{
	for (auto id : GlobalAudioTags)
		if (id == type)
			return true;
	return false;
}

bool CodecSampleEntryParser::IsVideoEntry(unsigned type) throw()
{
	for (auto id : GlobalVideoTags)
		if (id == type)
			return true;
	return false;
}