#ifndef __APEV2_TAG_EX_H
#define __APEV2_TAG_EX_H

#include <MediaAVIO.h>
#include <memory.h>
#include <malloc.h>

#define APETAGEX_FLAG 0x5845474154455041ULL
#define APETAGEX_VER 2000

#define APE_TAG_ALBUM "album"
#define APE_TAG_ARTIST "artist"
#define APE_TAG_TITLE "title"
#define APE_TAG_GENRE "genre"
#define APE_TAG_COMPOSER "composer"
#define APE_TAG_YEAR "year"
#define APE_TAG_TRACK "track"
#define APE_TAG_CUESHEET "cuesheet"

#pragma pack(1)
struct APETAGEX_FILE_HEADER //32bytes
{
	unsigned long long flag;
	unsigned version;
	unsigned tag_size;
	unsigned tag_count;
	unsigned tag_flags;
	unsigned char reserved[8];

	bool Verify() const throw()
	{ return flag == APETAGEX_FLAG && version == APETAGEX_VER && tag_size > 0 && tag_count > 0; }
};
#pragma pack()

bool IsFileID3v1Exists(IAVMediaIO* pIo);
unsigned DumpAPEv2TagHead(IAVMediaIO* pIo,APETAGEX_FILE_HEADER* head);

struct APETAGEX
{
	APETAGEX_FILE_HEADER header;
	unsigned char* data;

	APETAGEX() : data(nullptr) { memset(&header,0,sizeof(header)); }
	~APETAGEX() { if (data) free(data); }

	bool Initialize(IAVMediaIO* pIo);
	unsigned GetString(const char* name,char* copyTo);
	unsigned GetCoverImage(void* copyTo);

	unsigned SearchKeyOffset(const char* key,unsigned* size,unsigned* flag,bool strstr_mode);
};

#endif //__APEV2_TAG_EX_H