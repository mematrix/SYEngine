#include "MatroskaInternalChapters.h"

using namespace MKV;
using namespace MKV::Internal::Object;
using namespace MKV::Internal::Object::Context;

bool Chapters::ParseChapters(long long size)
{
	if (_head.GetDataSource() == nullptr)
		return false;

	int count_of_entry = 0;

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

		if (!_head.MatchId(MKV_ID_L2_CHAPTERS_EDITIONENTRY) ||
			count_of_entry > 0) {
			_head.Skip();
 			continue;
		}

		if (!ParseEditionEntry(_head.Size()))
			return false;

		count_of_entry++;
	}

	return true;
}

bool Chapters::ParseEditionEntry(long long size)
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

		if (!_head.MatchId(MKV_ID_L3_CHAPTERS_CHAPTERATOM))
		{
			_head.Skip();
			continue;
		}

		ChapterAtom atom;
		atom.ToDefault();
		if (!ParseChapterAtom(atom,_head.Size()))
			return false;

		_list.AddItem(&atom);
	}

	return true;
}

static const unsigned ChapterAtomUIntIdListCount = 4;
static const unsigned ChapterAtomUIntIdList[] = {
	MKV_ID_L4_CHAPTERS_CHAPTERTIMESTART,
	MKV_ID_L4_CHAPTERS_CHAPTERTIMEEND,
	MKV_ID_L4_CHAPTERS_CHAPTERFLAGHIDDEN,
	MKV_ID_L4_CHAPTERS_CHAPTERFLAGENABLED};

bool Chapters::ParseChapterAtom(Context::ChapterAtom& atom,long long size)
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

		EBML::EBML_VALUE_DATA value;
		if (_head.MatchIdList(ChapterAtomUIntIdList,ChapterAtomUIntIdListCount))
		{
			if (!EBML::EbmlReadValue(_head,value))
				return false;
		}

		switch (_head.Id())
		{
		case MKV_ID_L4_CHAPTERS_CHAPTERTIMESTART:
			atom.ChapterTimeStart = value.Ui64;
			break;
		case MKV_ID_L4_CHAPTERS_CHAPTERTIMEEND:
			atom.ChapterTimeEnd = value.Ui64;
			break;

		case MKV_ID_L4_CHAPTERS_CHAPTERFLAGHIDDEN:
			atom.fHidden = value.UIntToBool();
			break;
		case MKV_ID_L4_CHAPTERS_CHAPTERFLAGENABLED:
			atom.fEnabled = value.UIntToBool();
			break;

		case MKV_ID_L4_CHAPTERS_CHAPTERDISPLAY:
			if (!ParseChapterDisplay(atom.Display,_head.Size()))
				return false;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}

bool Chapters::ParseChapterDisplay(Context::ChapterDisplay& display,long long size)
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

		if (!_head.MatchId(MKV_ID_L5_CHAPTERS_CHAPSTRING) &&
			!_head.MatchId(MKV_ID_L5_CHAPTERS_CHAPLANG)) {
			_head.Skip();
			continue;
		}

		if (_head.MatchId(MKV_ID_L5_CHAPTERS_CHAPSTRING))
		{
			if (!display.SetChapString(_head))
				return false;
		}else{
			if (!EbmlReadStringSafe(_head,display.ChapLanguage))
				return false;
		}
	}

	return true;
}