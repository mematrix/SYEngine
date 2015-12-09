#include "MP4MediaFormat.h"

unsigned MP4MediaFormat::GetValue(const char* name,void* copyTo)
{
	unsigned result = 0;
	auto list = _core->InternalGetCore()->GetTags();
	if (name == nullptr || list->GetCount() == 0)
		return 0;

	const char* stick_name = name;
	if (strcmpi(stick_name,AV_METADATA_TYPE_NUMBER) == 0)
		stick_name = "track";

	for (unsigned i = 0;i < list->GetCount();i++)
	{
		auto tag = list->Get(i);
		if (tag->Name == nullptr || tag->StringPointer == nullptr)
			continue;
		if (strlen(tag->Name) > 255)
			continue;
		if (strcmpi(stick_name,tag->Name) != 0)
			continue;

		result = strlen(tag->StringPointer);
		if (copyTo)
			memcpy(copyTo,tag->StringPointer,result);
		break;
	}
	return result;
}

const char* MP4MediaFormat::GetValueName(unsigned index)
{
	if (index >= GetValueCount())
		return nullptr;
	if (_core->InternalGetCore()->GetTags()->GetCount() == 0)
		return nullptr;
	return _core->InternalGetCore()->GetTags()->Get(index)->Name;
}

AVMetadataCoverImageType MP4MediaFormat::GetCoverImageType()
{
	if (GetCoverImage(nullptr) == 0)
		return MetadataImageType_Unknown;
	return MetadataImageType_JPG;
}

unsigned MP4MediaFormat::GetCoverImage(unsigned char* copyTo)
{
	if (!_is_m4a_file)
		return 0;
	auto list = _core->InternalGetCore()->GetTags();
	if (list->GetCount() == 0)
		return 0;

	unsigned index = 0;
	for (unsigned i = 0;i < list->GetCount();i++)
	{
		if (list->Get(i)->StringPointer != nullptr || list->Get(i)->Name == nullptr)
			continue;
		if (strcmpi(list->Get(i)->Name,"cover") != 0)
			continue;
		index = i;
		break;
	}

	auto item = list->Get(index);
	if (item->DataPointer == nullptr || item->Length == 0)
		return 0;
	if (copyTo)
		memcpy(copyTo,item->DataPointer,item->Length);
	return item->Length;
}