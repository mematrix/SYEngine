#include "MatroskaCodecIds.h"
#include <string.h>

MatroskaCodecIds MatroskaFindAudioCodecId(const char* name)
{
	if (name == nullptr ||
		strlen(name) == 0)
		return MKV_Audio_Unknown;
	if (*name != 'A')
		return MKV_Audio_Unknown;

	for (auto ids : MKVAudioCodecIds)
	{
		if (strstr(name,ids.Name))
			return ids.Id;
	}

	return MKV_Audio_Unknown;
}

MatroskaCodecIds MatroskaFindVideoCodecId(const char* name)
{
	if (name == nullptr ||
		strlen(name) == 0)
		return MKV_Video_Unknown;
	if (*name != 'V')
		return MKV_Video_Unknown;

	for (auto ids : MKVVideoCodecIds)
	{
		if (strstr(name,ids.Name))
			return ids.Id;
	}

	return MKV_Video_Unknown;
}

MatroskaCodecIds MatroskaFindSubtitleCodecId(const char* name)
{
	if (name == nullptr ||
		strlen(name) == 0)
		return MKV_Subtitle_Unknown;
	if (*name != 'S')
		return MKV_Subtitle_Unknown;

	for (auto ids : MKVSubtitleCodecIds)
	{
		if (strstr(name,ids.Name))
			return ids.Id;
	}

	return MKV_Subtitle_Unknown;
}

MatroskaCodecIds MatroskaFindCodecId(const char* name)
{
	auto a = MatroskaFindAudioCodecId(name);
	auto v = MatroskaFindVideoCodecId(name);
	auto s = MatroskaFindSubtitleCodecId(name);

	if (a == MKV_Audio_Unknown &&
		v == MKV_Video_Unknown &&
		s == MKV_Subtitle_Unknown)
		return MKV_Error_CodecId;

	if (a != MKV_Audio_Unknown)
		return a;
	else if (v != MKV_Video_Unknown)
		return v;
	else
		return s;
}