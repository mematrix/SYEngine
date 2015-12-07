#include "MatroskaInternalSeekHead.h"

using namespace MKV;
using namespace MKV::Internal::Object;
using namespace MKV::Internal::Object::Context;

bool SeekHead::ParseSeekHead(long long size)
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

		if (!_head.MatchId(MKV_ID_L2_SEEKHEAD_SEEKENTRY))
		{
			_head.Skip();
			continue;
		}

		unsigned id = INVALID_EBML_ID;
		long long pos = INVALID_EBML_SIZE;
		if (!ParseSeekEntry(&id,&pos))
			return false;

		HeadSeekIndex index;
		index.SeekID = id;
		index.SeekPosition = pos;

		_list.AddItem(&index);
	}

	return true;
}

bool SeekHead::ParseSeekEntry(unsigned* id,long long* pos)
{
	while (1)
	{
		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (_head.MatchId(MKV_ID_L3_SEEKHEAD_SEEKID))
		{
			if (_head.Size() != 4) {
				_head.Skip();
			}else{
				unsigned char seekId[4] = {};
				if (!EBML::EbmlReadBinary(_head,seekId))
					return false;

				*id = MKV_SWAP32(*(unsigned*)&seekId);
			}
		}else if (_head.MatchId(MKV_ID_L3_SEEKHEAD_SEEKPOS))
		{
			EBML::EBML_VALUE_DATA value;
			if (!EBML::EbmlReadValue(_head,value))
				return false;

			*pos = (long long)value.Ui64;
		}else{
			break;
		}

		if (*id != INVALID_EBML_ID &&
			*pos != INVALID_EBML_SIZE)
			break;
	}

	return true;
}