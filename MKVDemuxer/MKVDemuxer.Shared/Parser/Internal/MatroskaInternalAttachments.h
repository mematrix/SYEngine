#ifndef _MATROSKA_INTERNAL_ATTACHMENTS_H
#define _MATROSKA_INTERNAL_ATTACHMENTS_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalAttachmentContext.h"

#define __DEFAULT_MKV_FILES 3

namespace MKV {
namespace Internal {
namespace Object {

class Attachments
{
public:
	explicit Attachments(EBML::EbmlHead& head) throw() : _head(head) {}
	~Attachments()
	{
		for (unsigned i = 0;i < _list.GetCount();i++)
			_list.GetItem(i)->FreeResources();
	}

public:
	bool ParseAttachments(long long size) throw();

	unsigned GetFileCount() throw() { return _list.GetCount(); }
	Context::AttachedFile* GetFile(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}

private:
	struct AttachedFile
	{
		char FileName[256]; //UTF8
		char FileDescription[1024]; //UTF8
		char FileMimeType[64];
		long long FileDataSize;
		long long OnAbsolutePosition;

		bool IsCompleted() throw()
		{
			if (FileName[0] != 0 &&
				FileDescription[0] != 0 &&
				FileMimeType[0] != 0 &&
				FileDataSize != 0 &&
				OnAbsolutePosition != 0)
				return true;

			return false;
		}
	};
	bool ParseAttachedFile(AttachedFile& file,long long size) throw();

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::AttachedFile,__DEFAULT_MKV_FILES> _list;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_ATTACHMENTS_H