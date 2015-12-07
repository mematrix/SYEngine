#ifndef _MATROSKA_INTERNAL_CHAPTERS_H
#define _MATROSKA_INTERNAL_CHAPTERS_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalChapterContext.h"

#define __DEFAULT_MKV_CHAPTERS 10

namespace MKV {
namespace Internal {
namespace Object {

class Chapters
{
public:
	explicit Chapters(EBML::EbmlHead& head) throw() : _head(head) {}
	~Chapters()
	{
		for (unsigned i = 0;i < _list.GetCount();i++)
			_list.GetItem(i)->FreeResources();
	}

public:
	bool ParseChapters(long long size) throw();

	unsigned GetChapterCount() throw() { return _list.GetCount(); }
	Context::ChapterAtom* GetChapter(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}

private:
	bool ParseEditionEntry(long long size) throw();

	bool ParseChapterAtom(Context::ChapterAtom& atom,long long size) throw();
	bool ParseChapterDisplay(Context::ChapterDisplay& display,long long size) throw();

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::ChapterAtom,__DEFAULT_MKV_CHAPTERS> _list;
};

}}}

#endif //_MATROSKA_INTERNAL_CHAPTERS_H