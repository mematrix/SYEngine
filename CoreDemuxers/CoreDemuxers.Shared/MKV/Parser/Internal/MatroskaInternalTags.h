#ifndef _MATROSKA_INTERNAL_TAGS_H
#define _MATROSKA_INTERNAL_TAGS_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalTagContext.h"

#define __DEFAULT_MKV_TAGS 20

#define MAX_MKV_TAG_NAME 256
#define MAX_MKV_TAG_STRING 1024

namespace MKV {
namespace Internal {
namespace Object {

class Tags
{
public:
	explicit Tags(EBML::EbmlHead& head) throw() : _head(head) {}
	~Tags()
	{
		for (unsigned i = 0;i < _list.GetCount();i++)
			_list.GetItem(i)->FreeResources();
	}

public:
	bool ParseTags(long long size) throw();

	unsigned GetTagCount() throw() { return _list.GetCount(); }
	Context::SimpleTag* GetTag(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}
	Context::SimpleTag* SearchTagByName(const char* name);

private:
	bool ParseTag(long long size) throw();

	struct SimpleTag
	{
		char TagName[MAX_MKV_TAG_NAME]; //UTF8
		char TagString[MAX_MKV_TAG_STRING]; //UTF8
		char TagLanguage[32];
	};
	bool ParseSimpleTag(SimpleTag& tag,long long size) throw();

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::SimpleTag,__DEFAULT_MKV_TAGS> _list;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_TAGS_H