#ifndef _MATROSKA_EBML_DOC_H
#define _MATROSKA_EBML_DOC_H

#include "MatroskaEbml.h"

namespace MKV {
namespace EBML {

struct EbmlDocument
{
	unsigned EbmlVersion;
	unsigned EbmlReadVersion;
	unsigned EbmlMaxIdLength;
	unsigned EbmlMaxSizeLength;
	char DocType[32];
	unsigned DocTypeVersion;
	unsigned DocTypeReadVersion;

	inline long long GetSegmentSize() const throw() { return _segmentSize; }

	void Default() throw()
	{
		EbmlVersion = EbmlReadVersion = 1;
		EbmlMaxIdLength = 4;
		EbmlMaxSizeLength = 8;
		DocType[0] = 0;
		DocTypeVersion = DocTypeReadVersion = 1;
	}

	bool IsDocumentSupported() throw()
	{
		if (EbmlVersion > SUPPORT_EBML_VERSION ||
			EbmlMaxIdLength > SUPPORT_EBML_ID_LENGTH ||
			EbmlMaxSizeLength > SUPPORT_EBML_SIZE_LENGTH)
			return false;

		if (DocTypeReadVersion > 3 ||
			DocTypeReadVersion == 0 ||
			DocTypeVersion == 0)
			return false;

		if (strcmpi(DocType,MKV_DOC_TYPE_MAIN) != 0 &&
			strcmpi(DocType,MKV_DOC_TYPE_WEBM) != 0)
			return false;

		return true;
	}

	bool Parse(MKV::DataSource::IDataSource* dataSource) throw()
	{
		_segmentSize = 0;

		EbmlHead head(dataSource);
		if (!head.IsValid())
			return false;

		if (head.Size() > 128 || head.Size() == 0)
			return false;
		if (head.Id() != EBML_ID_HEADER)
			return false;

		bool result = false;
		while (1)
		{
			head.Parse(dataSource);
			if (!head.IsValid())
				break;
			if (head.Size() == 0)
				break;

			if (head.SkipIfVoid())
				continue;

			if (head.Id() == MKV_ID_L0_SEGMENT)
			{
				_segmentSize = head.Size();

				result = true;
				break;
			}

			EBML_VALUE_DATA value = {};
			switch (head.Id())
			{
			case EBML_ID_HEAD_VERSION:
				if (!EbmlReadValue(head,value))
					return false;

				EbmlVersion = value.Ui32;
				break;
			case EBML_ID_HEAD_READ_VERSION:
				if (!EbmlReadValue(head,value))
					return false;

				EbmlReadVersion = value.Ui32;
				break;
			case EBML_ID_HEAD_MAX_ID_LENGTH:
				if (!EbmlReadValue(head,value))
					return false;

				EbmlMaxIdLength = value.Ui32;
				break;
			case EBML_ID_HEAD_MAX_SIZE_LENGTH:
				if (!EbmlReadValue(head,value))
					return false;

				EbmlMaxSizeLength = value.Ui32;
				break;
			case EBML_ID_HEAD_DOC_TYPE:
				EbmlReadString(head,DocType,32);
				break;
			case EBML_ID_HEAD_DOC_TYPE_VERSION:
				if (!EbmlReadValue(head,value))
					return false;

				DocTypeVersion = value.Ui32;
				break;
			case EBML_ID_HEAD_DOC_TYPE_READ_VERSION:
				if (!EbmlReadValue(head,value))
					return false;

				DocTypeReadVersion = value.Ui32;
				break;
			default:
				head.Skip();
			}
		}

		return result;
	}

private:
	long long _segmentSize;
};

}} //MKV::EBML.

#endif //_MATROSKA_EBML_DOC_H