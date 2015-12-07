#include "MKVMediaFormat.h"

unsigned MKVMediaFormat::GetValue(const char* name,void* copyTo)
{
	unsigned result = 0;
	if (_parser->GetCore()->GetSegmentInfo() == nullptr ||
		_parser->GetCore()->GetTags() == nullptr)
		return 0;

	if (strcmpi(name,AV_METADATA_TYPE_TITLE) == 0)
	{
		auto str = _parser->GetCore()->GetSegmentInfo()->GetTitle();
		if (str != nullptr)
		{
			result = strlen(str);
			if (copyTo)
				memcpy(copyTo,str,result);
		}
		return result;
	}

	const char* stick_name = name;
	if (strcmpi(stick_name,AV_METADATA_TYPE_NUMBER) == 0)
		stick_name = "PART_NUMBER";

	for (unsigned i = 0;i < _parser->GetCore()->GetTags()->GetTagCount();i++)
	{
		auto tag = _parser->GetCore()->GetTags()->GetTag(i);
		if (tag->TagName == nullptr)
			continue;
		if (strlen(tag->TagName) > 255)
			continue;

		if (strcmpi(stick_name,tag->TagName) != 0)
			continue;
		if (tag->TagString == nullptr)
			continue;

		result = strlen(tag->TagString);
		if (copyTo)
			memcpy(copyTo,tag->TagString,result);
		break;
	}
	return result;
}

const char* MKVMediaFormat::GetValueName(unsigned index)
{
	if (index >= GetValueCount())
		return nullptr;
	if (_parser->GetCore()->GetTags() == nullptr)
		return nullptr;
	auto tag = _parser->GetCore()->GetTags()->GetTag(index);
	return tag->TagName;
}

AVMetadataCoverImageType MKVMediaFormat::GetCoverImageType()
{
	if (GetCoverImage(nullptr) == 0)
		return MetadataImageType_Unknown;
	return MetadataImageType_JPG;
}

unsigned MKVMediaFormat::GetCoverImage(unsigned char* copyTo)
{
	if (!_parser->IsMatroskaAudioOnly())
		return 0;
	if (_parser->GetCore()->GetAttachments() == nullptr)
		return 0;
	if (_parser->GetCore()->GetAttachments()->GetFileCount() == 0)
		return 0;

	MKV::Internal::Object::Context::AttachedFile* file = nullptr;
	for (unsigned i = 0;_parser->GetCore()->GetAttachments()->GetFileCount();i++)
	{
		auto p = _parser->GetCore()->GetAttachments()->GetFile(i);
		if (p->FileName == nullptr || p->FileMimeType == nullptr || p->FileDataSize == 0)
			continue;

		if (strcmpi(p->FileMimeType,"image/jpeg") != 0 &&
			strcmpi(p->FileMimeType,"image/jpg") != 0 &&
			strcmpi(p->FileMimeType,"image/png") != 0)
			continue;

		file = p;
		break;
	}
	if (file == nullptr)
		return 0;
	if (file->FileDataSize > 10485760)
		return 0;

	if (copyTo)
	{
		auto oldPos = _av_io->Tell();
		if (_av_io->Seek(file->OnAbsolutePosition,SEEK_SET))
			_av_io->Read(copyTo,(unsigned)file->FileDataSize);
		_av_io->Seek(oldPos,SEEK_SET);
	}

	return (unsigned)file->FileDataSize;
}