#include "VorbisAudioDescription.h"

static unsigned ParseXiphNumber(unsigned char* buf,unsigned len,unsigned* bytes)
{
	unsigned result = 0;
	for (unsigned i = 0;i < len;i++)
	{
		result += buf[i];
		if (buf[i] != 0xFF)
		{
			if (bytes)
				*bytes = i + 1;
			break;
		}
	}
	return result;
}

#pragma pack(1)
struct VorbisIdentificationHeader
{
	unsigned vorbis_version;
	unsigned char audio_channels;
	unsigned audio_sample_rate;
	unsigned bitrate_maximum;
	unsigned bitrate_nominal;
	unsigned bitrate_minimum;
	unsigned char blocksize; //0 and 1
};
#pragma pack(1)

VorbisAudioDescription::VorbisAudioDescription(unsigned char* ph,unsigned size) throw()
{
	memset(&_basic_desc,0,sizeof(_basic_desc));
	memset(&_profile,0,sizeof(_profile));

	FlushAudioDescription(ph,size);
}

bool VorbisAudioDescription::GetProfile(void* profile)
{
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool VorbisAudioDescription::GetExtradata(void* p)
{
	if (p == nullptr ||
		_extradata.get() == nullptr)
		return 0;

	memcpy(p,_extradata.get(),_extradata_len);
	return true;
}

unsigned VorbisAudioDescription::GetExtradataSize()
{
	return _extradata_len;
}

bool VorbisAudioDescription::GetAudioDescription(AudioBasicDescription* desc)
{
	if (desc == nullptr || _basic_desc.nch == 0)
		return false;

	*desc = _basic_desc;
	return true;
}

void VorbisAudioDescription::FlushAudioDescription(unsigned char* ph,unsigned size)
{
	if (ph == nullptr ||
		size == 0)
		return;

	if (ph[0] != 2)
		return;
	unsigned hsize = ph[1];
	if (hsize < 30 || hsize > 254)
		return;
	unsigned offset = 0;
	ParseXiphNumber(ph + 2,size,&offset);
	offset += 2;

	if (ph[offset] != 1)
		return;
	offset += 7;

	VorbisIdentificationHeader id = {};
	memcpy(&id,ph + offset,sizeof(id));
	if (id.audio_channels == 0 || id.audio_channels > 8)
		return;

	_basic_desc.bitrate = id.bitrate_nominal;
	_basic_desc.nch = id.audio_channels;
	_basic_desc.srate = id.audio_sample_rate;
	_basic_desc.bits = 16;
	_basic_desc.compressed = true;

	_profile.bitrate = id.bitrate_nominal;
	_profile.max_bitrate = id.bitrate_maximum;
	_profile.min_bitrate = id.bitrate_minimum;
	_profile.block_size0 = 1 << (id.blocksize & 0x0F);
	_profile.block_size1 = 1 << (id.blocksize >> 4);

	_extradata_len = size;
	_extradata = std::unique_ptr<unsigned char>(new unsigned char[_extradata_len]);
	memcpy(_extradata.get(),ph,_extradata_len);
}