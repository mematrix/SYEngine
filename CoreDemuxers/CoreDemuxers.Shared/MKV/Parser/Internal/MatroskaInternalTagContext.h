#ifndef _MATROSKA_INTERNAL_TAG_CONTEXT_H
#define _MATROSKA_INTERNAL_TAG_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct SimpleTag
{
	char* TagName; //UTF8
	char* TagString; //UTF8
	char TagLanguage[32];

	void ToDefault() throw()
	{
		TagName = TagString = nullptr;
		TagLanguage[0] = 0;
	}

	void FreeResources() throw()
	{
		if (TagName != nullptr)
		{
			free(TagName);
			TagName = nullptr;
		}

		if (TagString != nullptr)
		{
			free(TagString);
			TagString = nullptr;
		}
	}

	bool CopyFrom(char* name,char* str) throw()
	{
		if (name == nullptr ||
			str == nullptr)
			return false;

		TagName = strdup(name);
		TagString = strdup(str);
		if (TagName == nullptr ||
			TagString == nullptr)
			return false;

		return true;
	}
};

}}}}

#endif //_MATROSKA_INTERNAL_TAG_CONTEXT_H