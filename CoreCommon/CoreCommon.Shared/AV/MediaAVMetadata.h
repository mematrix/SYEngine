#ifndef __MEDIA_AV_METADATA_H
#define __MEDIA_AV_METADATA_H

#define AV_METADATA_TYPE_TITLE "title"
#define AV_METADATA_TYPE_ALBUM "album"
#define AV_METADATA_TYPE_ARTIST "artist"
#define AV_METADATA_TYPE_GENRE "genre"
#define AV_METADATA_TYPE_COMPOSER "composer"
#define AV_METADATA_TYPE_DATE "date"
#define AV_METADATA_TYPE_NUMBER "number"
#define AV_METADATA_TYPE_COMMENT "comment"
#define AV_METADATA_TYPE_COPYRIGHT "copyright"

enum AVMetadataStringType
{
	MetadataStrType_ANSI,
	MetadataStrType_UTF8,
	MetadataStrType_Unicode
};

enum AVMetadataCoverImageType
{
	MetadataImageType_Unknown,
	MetadataImageType_BMP,
	MetadataImageType_JPG,
	MetadataImageType_PNG,
	MetadataImageType_GIF,
	MetadataImageType_TIFF
};

class IAVMediaMetadata
{
public:
	virtual AVMetadataStringType GetStrType() { return AVMetadataStringType::MetadataStrType_UTF8; }
	virtual unsigned GetValueCount() { return 0; }
	virtual unsigned GetValue(const char* name,void* copyTo) { return 0; }
	virtual const char* GetValueName(unsigned index) { return nullptr; }
	virtual AVMetadataCoverImageType GetCoverImageType() { return AVMetadataCoverImageType::MetadataImageType_Unknown; }
	virtual unsigned GetCoverImage(unsigned char* copyTo) { return false; }
};

#endif //__MEDIA_AV_METADATA_H