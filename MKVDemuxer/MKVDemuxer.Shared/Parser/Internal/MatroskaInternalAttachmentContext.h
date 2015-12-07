#ifndef _MATROSKA_INTERNAL_ATTACHMENT_CONTEXT_H
#define _MATROSKA_INTERNAL_ATTACHMENT_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct AttachedFile
{
	char* FileName; //UTF8
	char* FileDescription; //UTF8
	char FileMimeType[64];
	long long FileDataSize;
	long long OnAbsolutePosition;

	void ToDefault() throw()
	{
		FileName = FileDescription = nullptr;
		FileMimeType[0] = 0;

		FileDataSize = OnAbsolutePosition = 0;
	}

	void FreeResources() throw()
	{
		if (FileName != nullptr)
		{
			free(FileName);
			FileName = nullptr;
		}

		if (FileDescription != nullptr)
		{
			free(FileDescription);
			FileDescription = nullptr;
		}
	}

	bool CopyFrom(char* name,char* desc) throw()
	{
		if (name == nullptr)
			return false;

		FileName = strdup(name);
		if (desc && strlen(desc) > 0)
			FileDescription = strdup(desc);

		if (FileName == nullptr)
			return false;

		return true;
	}
};

}}}}

#endif //_MATROSKA_INTERNAL_ATTACHMENT_CONTEXT_H