#ifndef _MATROSKA_INTERNAL_CHAPTER_CONTEXT_H
#define _MATROSKA_INTERNAL_CHAPTER_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct ChapterDisplay
{
	char* ChapString; //UTF8
	char ChapLanguage[32]; //und

	void ToDefault() throw()
	{
		ChapString = nullptr;
		ChapLanguage[0] = 0;
	}

	void FreeResources() throw()
	{
		if (ChapString != nullptr)
		{
			free(ChapString);
			ChapString = nullptr;
		}
	}

	bool SetChapString(EBML::EbmlHead& head)
	{
		if (ChapString != nullptr)
			free(ChapString);

		ChapString = (char*)malloc(MKV_ALLOC_ALIGNED((unsigned)head.Size()));
		if (ChapString == nullptr)
			return false;
		if (!EBML::EbmlReadString(head,ChapString))
			return false;

		return true;
	}
};

struct ChapterAtom
{
	long long ChapterTimeStart;
	long long ChapterTimeEnd;
	bool fHidden;
	bool fEnabled;
	ChapterDisplay Display;

	void ToDefault() throw()
	{
		Display.ToDefault();
		
		ChapterTimeStart = ChapterTimeEnd = -1;
		fHidden = false;
		fEnabled = true;
	}

	void FreeResources() throw() { Display.FreeResources(); }
};

}}}} //MKV::Internal::Object::Context.

#endif //_MATROSKA_INTERNAL_CHAPTER_CONTEXT_H