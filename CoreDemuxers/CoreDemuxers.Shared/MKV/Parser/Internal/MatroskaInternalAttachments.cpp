#include "MatroskaInternalAttachments.h"

using namespace MKV;
using namespace MKV::Internal::Object;

bool Attachments::ParseAttachments(long long size) throw()
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

		if (!_head.MatchId(MKV_ID_L2_ATTACHMENTS_ATTACHEDFILE))
		{
			_head.Skip();
			continue;
		}

		AttachedFile file = {};
		if (!ParseAttachedFile(file,_head.Size()))
			return false;

		Context::AttachedFile afile;
		afile.ToDefault();
		afile.FileDataSize = file.FileDataSize;
		afile.OnAbsolutePosition = file.OnAbsolutePosition;
		//strcpy_s(afile.FileMimeType,MKV_ARRAY_COUNT(afile.FileMimeType),file.FileMimeType);
		strcpy(afile.FileMimeType,file.FileMimeType);
		if (afile.CopyFrom(file.FileName,file.FileDescription))
			_list.AddItem(&afile);
	}

	return true;
}

bool Attachments::ParseAttachedFile(AttachedFile& file,long long size) throw()
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

		if (_head.MatchId(MKV_ID_L2_ATTACHMENTS_ATTACHEDFILE))
			break;

		switch (_head.Id())
		{
		case MKV_ID_L3_ATTACHMENTS_FILENAME:
			if (!EbmlReadStringSafe(_head,file.FileName))
				return false;
			break;
		case MKV_ID_L3_ATTACHMENTS_FILEDESC:
			if (_head.Size() > 512) {
				_head.Skip();
			}else{
				EbmlReadStringSafe(_head,file.FileDescription);
			}

			break;
		case MKV_ID_L3_ATTACHMENTS_FILEMIMETYPE:
			if (!EbmlReadStringSafe(_head,file.FileMimeType))
				return false;
			break;
		case MKV_ID_L3_ATTACHMENTS_FILEDATA:
			file.FileDataSize = _head.Size();
			file.OnAbsolutePosition = _head.GetDataSource()->Tell();
		default:
			_head.Skip();
		}
	}

	return true;
}