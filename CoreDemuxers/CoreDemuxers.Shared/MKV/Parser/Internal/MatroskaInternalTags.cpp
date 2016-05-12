#include "MatroskaInternalTags.h"

#ifndef _MSC_VER
#define strcmpi strcasecmp
#endif

using namespace MKV;
using namespace MKV::Internal::Object;

bool Tags::ParseTags(long long size) throw()
{
	if (_head.GetDataSource() == nullptr)
		return false;

	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (!_head.MatchId(MKV_ID_L2_TAGS_TAG))
		{
			_head.Skip();
			continue;
		}

		if (!ParseTag(_head.Size()))
			return false;
	}

	return true;
}

bool Tags::ParseTag(long long size) throw()
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (!_head.MatchId(MKV_ID_L3_TAGS_SIMPLETAG))
		{
			_head.Skip();
			continue;
		}

		SimpleTag simpleTag = {};
		if (!ParseSimpleTag(simpleTag,_head.Size()))
			return false;

		Context::SimpleTag tag;
		tag.ToDefault();
		//strcpy_s(tag.TagLanguage,MKV_ARRAY_COUNT(tag.TagLanguage),simpleTag.TagLanguage);
		strcpy(tag.TagLanguage,simpleTag.TagLanguage);
		if (tag.CopyFrom(simpleTag.TagName,simpleTag.TagString))
			_list.AddItem(&tag);
	}

	return true;
}

bool Tags::ParseSimpleTag(SimpleTag& tag,long long size) throw()
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (_head.MatchId(MKV_ID_L3_TAGS_SIMPLETAG))
			break;

		switch (_head.Id())
		{
		case MKV_ID_L4_TAGS_TAGNAME:
			if (!EbmlReadStringSafe(_head,tag.TagName))
				return false;
			break;
		case MKV_ID_L4_TAGS_TAGSTRING:
			if (!EbmlReadStringSafe(_head,tag.TagString))
				return false;
			break;
		case MKV_ID_L4_TAGS_TAGLANG:
			if (!EbmlReadStringSafe(_head,tag.TagLanguage))
				return false;
			break;
		default:
			_head.Skip();
		}
	}

	return true;
}

Context::SimpleTag* Tags::SearchTagByName(const char* name)
{
	if (name == nullptr)
		return nullptr;
	if (strlen(name) == 0)
		return nullptr;

	unsigned count = _list.GetCount();
	for (unsigned i = 0;i < count;i++)
	{
		auto tag = _list.GetItem(i);
		if (tag->TagName == nullptr || tag->TagString == nullptr)
			continue;
		if (strcmpi(tag->TagName,name) == 0)
			return tag;
	}
	return nullptr;
}