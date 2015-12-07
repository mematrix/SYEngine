#ifndef __MEDIA_AV_CHAPTER_H
#define __MEDIA_AV_CHAPTER_H

#define MAX_AV_CHAPTER_STRING_LEN 256

enum AVChapterType
{
	ChapterType_Marker,
	ChapterType_Timeline
};

enum AVChapterEncodingType
{
	ChapterStringEnc_ANSI,
	ChapterStringEnc_UTF8,
	ChapterStringEnc_UNICODE
};

struct AVMediaChapter
{
	AVChapterType type;
	AVChapterEncodingType str_enc_type;
	double index;
	double start_time; //seconds.
	double end_time; //seconds.
	double duration;
	char title[MAX_AV_CHAPTER_STRING_LEN];
	char desc[MAX_AV_CHAPTER_STRING_LEN];
	char lang[32];
	unsigned flags;
};

class IAVMediaChapters
{
public:
	virtual unsigned GetChapterCount() { return 0; }
	virtual AVMediaChapter* GetChapter(unsigned index) { return nullptr; }
};

#endif //__MEDIA_AV_CHAPTER_H