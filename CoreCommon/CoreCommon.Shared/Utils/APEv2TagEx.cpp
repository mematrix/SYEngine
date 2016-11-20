#include "APEv2TagEx.h"
#include "MiscUtils.h"
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#else
#define stricmp strcasecmp
#endif

bool IsFileID3v1Exists(IAVMediaIO* pIo)
{
	if (pIo == nullptr)
		return false;
	if (pIo->GetSize() < 128)
		return false;

	auto oldPos = pIo->Tell();
	if (!pIo->Seek(pIo->GetSize() - 128,SEEK_SET))
		return false;
	unsigned flag = 0;
	pIo->Read(&flag,3);

	pIo->Seek(oldPos,SEEK_SET);
	return flag == 0x474154;
}

unsigned DumpAPEv2TagHead(IAVMediaIO* pIo,APETAGEX_FILE_HEADER* head)
{
	if (pIo == nullptr)
		return 0;

	unsigned offset = 32;
	if (IsFileID3v1Exists(pIo))
		offset += 128;

	auto oldPos = pIo->Tell();
	if (pIo->GetSize() < (offset * 2))
		return 0;
	if (!pIo->Seek(pIo->GetSize() - offset,SEEK_SET))
		return 0;
	pIo->Read(head,32);
	head->tag_size -= 32;

	pIo->Seek(oldPos,SEEK_SET);
	return offset;
}

bool APETAGEX::Initialize(IAVMediaIO* pIo)
{
	if (pIo == nullptr)
		return false;
	if (data != nullptr)
		return false;

	auto oldPos = pIo->Tell();
	unsigned offset = DumpAPEv2TagHead(pIo,&header);
	if (offset == 0)
		return false;
	if (!header.Verify())
		return false;

	if (header.tag_size > pIo->GetSize())
		return false;
	if (!pIo->Seek(pIo->GetSize() - offset - header.tag_size,SEEK_SET))
		return false;

	data = (decltype(data))malloc(header.tag_size / 4 * 4 + 4);
	if (data == nullptr)
	{
		pIo->Seek(oldPos,SEEK_SET);
		return false;
	}
	memset(data,0,header.tag_size);
	if (pIo->Read(data,header.tag_size) != header.tag_size)
	{
		pIo->Seek(oldPos,SEEK_SET);
		return false;
	}

	return true;
}

unsigned APETAGEX::GetString(const char* name,char* copyTo)
{
	if (name == nullptr)
		return 0;
	
	unsigned size = 0,flag = 0;
	unsigned offset = SearchKeyOffset(name,&size,&flag,false);
	if (offset == 0)
		return 0;
	if (size == 0 && flag != 0)
		return 0;
	
	if (copyTo != nullptr)
		memcpy(copyTo,data + offset,size);
	return size;
}

unsigned APETAGEX::GetCoverImage(void* copyTo)
{
	unsigned size = 0,flag = 0;
	unsigned offset = SearchKeyOffset("cover",&size,&flag,true);
	if (offset == 0)
		return 0;
	if (size < 128 && flag != 2)
		return 0;

	unsigned result = 0;
	auto p = data + offset;
	for (unsigned i = 0;i < 128;i++)
	{
		if (*(unsigned*)(p + i) == 0xE0FFD8FF ||
			*(unsigned*)(p + i) == 0x474E5089) { //JPG&PNG Head
			offset += i;
			result = size - i;
			break;
		}
	}

	if (result != 0 && copyTo)
		memcpy(copyTo,data + offset,result);
	return result;
}

unsigned APETAGEX::SearchKeyOffset(const char* key,unsigned* size,unsigned* flag,bool strstr_mode)
{
	auto p = data;
	unsigned offset = 0;
	for (unsigned i = 0;i < header.tag_count;i++)
	{
		*size = *(unsigned*)p;
		*flag = *(unsigned*)(p + 4);

		auto name = (char*)(p + 8);
		unsigned name_size = strlen(name);
		if (name_size == 0)
			continue;

		unsigned total_size = 8 + name_size + 1 + *size;
		bool ok = false;
		if (!strstr_mode)
			ok = stricmp(key,name) == 0;
		else
			ok = strstr(msvc_strlwr(name),key) != nullptr;
		if (ok)
		{
			total_size -= *size;
			offset = (p + total_size) - data;
			break;
		}

		p += total_size;
	}
	return offset;
}