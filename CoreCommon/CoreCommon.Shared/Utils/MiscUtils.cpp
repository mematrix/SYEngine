#include <stdio.h>
#include <ctype.h>
#include <memory.h>

char* msvc_strlwr(char* s)
{
    auto str = s;
    while (1) {
        if (*str == '\0')
            break;
        *str = tolower(*str);
        str++;
    }
    return s;
}

char* msvc_strupr(char* s)
{
    auto str = s;
    while (1) {
        if (*str == '\0')
            break;
        *str = toupper(*str);
        str++;
    }
    return s;
}

unsigned short MakeAACAudioSpecificConfig(unsigned audioObjectType,unsigned samplingFrequency,unsigned channelConfiguration)
{
	if (audioObjectType == 0 ||
		samplingFrequency < 7350 ||
		channelConfiguration < 1)
		return 0;

	static const int aac_sample_rate_table[] = {
		96000,88200,64000,48000,44100,32000,
		24000,22050,16000,12000,11025,8000,7350,
		0,0,0,0};

	unsigned samplingFrequencyIndex = -1;
	for (unsigned i = 0;i < 16;i++) {
		if (aac_sample_rate_table[i] == samplingFrequency)
		{
			samplingFrequencyIndex = i;
			break;
		}
	}

	if (samplingFrequencyIndex == -1)
		return 0;

	unsigned short result = 0;
	unsigned char config1 = (audioObjectType << 3) | ((samplingFrequencyIndex & 0x0E) >> 1);
	unsigned char config2 = ((samplingFrequencyIndex & 0x01) << 7) | (channelConfiguration << 3);
	*(unsigned char*)&result = config1;
	*((unsigned char*)&result + 1) = config2;

	return result;
}

int ParseMKVOggXiphHeader(unsigned char* buf,unsigned size,unsigned* h1size,unsigned* h2size,unsigned* h3size)
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

int MemorySearch(const unsigned char* buf,unsigned bufSize,const unsigned char* search,unsigned searchSize)
{
	if (buf == nullptr || bufSize == 0 || search == nullptr || searchSize == 0)
		return -1;
	if (searchSize >= bufSize)
		return -1;

	for (unsigned i = 0;i < (bufSize - searchSize - 1);i++)
	{
		if (memcmp(buf + i,search,searchSize) == 0)
			return (int)i;
	}

	return -1;
}