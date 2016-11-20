#include "MatroskaInternalCues.h"

using namespace MKV;
using namespace MKV::Internal::Object;
using namespace MKV::Internal::Object::Context;

bool Cues::ParseCues(long long size) throw()
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

		if (!_head.MatchId(MKV_ID_L2_CUES_POINTENTRY))
		{
			_head.Skip();
			continue;
		}

		CuePoint point;
		point.ToDefault();
		if (!ParseCuePoint(point,_head.Size()))
			return false;

		_list.AddItem(&point);
	}

	return true;
}

bool Cues::ParseCuePoint(Context::CuePoint& point,long long size) throw()
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

		if (!_head.MatchId(MKV_ID_L3_CUES_POINT_CUETIME) &&
			!_head.MatchId(MKV_ID_L3_CUES_POINT_CUETRACKPOSITION)) {
			_head.Skip();
			continue;
		}

		if (_head.MatchId(MKV_ID_L3_CUES_POINT_CUETIME))
		{
			EBML::EBML_VALUE_DATA value;
			if (!EBML::EbmlReadValue(_head,value))
				return false;

			point.CueTime = value.Ui64;
		}else{
			if (!ParseCueTrackPositions(point.Positions,_head.Size()))
				return false;
		}
	}

	return true;
}

static const unsigned CuePosUIntIdListCount = 5;
static const unsigned CuePosUIntIdList[] = {MKV_ID_L4_CUES_POINT_POS_CUETRACK,
	MKV_ID_L4_CUES_POINT_POS_CUECLUSTERPOSITION,
	MKV_ID_L4_CUES_POINT_POS_CUERELATIVEPOSITION,
	MKV_ID_L4_CUES_POINT_POS_CUEDURATION,
	MKV_ID_L4_CUES_POINT_POS_CUEBLOCKNUMBER};

bool Cues::ParseCueTrackPositions(Context::CueTrackPositions& pos,long long size) throw()
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
		if (_head.MatchIdList(CuePosUIntIdList,CuePosUIntIdListCount))
		{
			if (!EBML::EbmlReadValue(_head,value))
				return false;
		}

		switch (_head.Id())
		{
		case MKV_ID_L4_CUES_POINT_POS_CUETRACK:
			pos.CueTrack = value.Ui32;
			break;
		case MKV_ID_L4_CUES_POINT_POS_CUECLUSTERPOSITION:
			pos.CueClusterPosition = value.Ui64;
			break;
		case MKV_ID_L4_CUES_POINT_POS_CUERELATIVEPOSITION:
			pos.CueRelativePosition = value.Ui64;
			break;
		case MKV_ID_L4_CUES_POINT_POS_CUEDURATION:
			pos.CueDuration = value.Ui64;
			break;
		case MKV_ID_L4_CUES_POINT_POS_CUEBLOCKNUMBER:
			pos.CueBlockNumber = value.Ui64;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}