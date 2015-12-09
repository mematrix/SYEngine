#include "MatroskaInternalSegmentInfo.h"

using namespace MKV;
using namespace MKV::Internal::Object;

bool SegmentInfo::ParseSegmentInfo(long long size)
{
	_info.TimecodeScale = 1000000; //default.

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

		EBML::EBML_VALUE_DATA value;
		switch (_head.Id())
		{
		case MKV_ID_L2_INFO_TITLE:
			if (!EbmlReadStringSafe(_head,_info.Title))
				return false;
			break;
		case MKV_ID_L2_INFO_MUXINGAPP:
			if (!EbmlReadStringSafe(_head,_info.MuxingApp))
				return false;
			break;
		case MKV_ID_L2_INFO_WRITINGAPP:
			if (!EbmlReadStringSafe(_head,_info.WritingApp))
				return false;
			break;
		case MKV_ID_L2_INFO_DURATION:
			if (!EBML::EbmlReadValue(_head,value,true))
				return false;

			_info.Duration = value.Double;
			break;
		case MKV_ID_L2_INFO_TIMECODESCALE:
			if (!EBML::EbmlReadValue(_head,value))
				return false;

			_info.TimecodeScale = (long long)value.Ui64;
			break;
		case MKV_ID_L2_INFO_SEGMENTUID:
			if (_head.Size() == 16)
				_head.Read(_info.Uuid, 16);
			else
				_head.Skip();
			break;
		default:
			_head.Skip();
		}
	}

	return true;
}