#include "IsomParser.h"

using namespace Isom;
using namespace Isom::Types;
using namespace Isom::Object;
using namespace Isom::Object::Internal;

struct TagMap
{
	unsigned Type;
	const char* Name;
};
static const TagMap GlobalMetaKeyMaps[] = {
	{iTunes::apple_tag_title, "title"},
	{iTunes::apple_tag_sub_title, "sub_title"},
	{iTunes::apple_tag_album_artist, "album_artist"},
	{iTunes::apple_tag_album, "album"},
	{iTunes::apple_tag_original_artist, "original_artist"},
	{iTunes::apple_tag_artist0, "artist"},
	{iTunes::apple_tag_artist1, "artist"},
	{iTunes::apple_tag_date, "date"},
	{iTunes::apple_tag_genre0, "genre"},
	{iTunes::apple_tag_genre1, "genre_id3v1"},
	{iTunes::apple_tag_comment0, "comment"},
	{iTunes::apple_tag_comment1, "comment"},
	{iTunes::apple_tag_copyright0, "copyright"},
	{iTunes::apple_tag_copyright1, "copyright"},
	{iTunes::apple_tag_composer0, "composer"},
	{iTunes::apple_tag_composer1, "composer"},
	{iTunes::apple_tag_producer0, "producer"},
	{iTunes::apple_tag_producer1, "producer"},
	{iTunes::apple_tag_performers, "performers"},
	{iTunes::apple_tag_director, "director"},
	{iTunes::apple_tag_disclaimer, "disclaimer"},
	{iTunes::apple_tag_chapter, "chapter"},
	{iTunes::apple_tag_track0, "track"},
	{iTunes::apple_tag_track1, "track"},
	{iTunes::apple_tag_lyrics, "lyrics"},
	{iTunes::apple_tag_disc, "disc"},
	{iTunes::apple_tag_rating, "rating"},
	{iTunes::apple_tag_link, "link"},
	{iTunes::apple_tag_podcast, "podcast"},
	{iTunes::apple_tag_description, "description"},
	{iTunes::apple_tag_category, "category"},
	{iTunes::apple_tag_keywords, "keywords"},
	{iTunes::apple_tag_original_source, "original_source"},
	{iTunes::apple_tag_original_format, "original_format"},
	{iTunes::apple_tag_host_computer, "host_computer"},
	{iTunes::apple_tag_encoder0, "encoder"},
	{iTunes::apple_tag_encoder1, "encoder"},
	{iTunes::apple_tag_encoder2, "encoder"},
	{iTunes::apple_tag_sort_title, "sort_title"},
	{iTunes::apple_tag_sort_artist, "sort_artist"},
	{iTunes::apple_tag_sort_album_title, "sort_album_title"},
	{iTunes::apple_tag_sort_album_artist, "sort_album_artist"}
};

unsigned Parser::OnMetadataBox(BoxObject* box, bool itunes)
{
	if (box->size < 9)
		return ISOM_ERR_OK;

	const char* name = nullptr;
	for (auto n : GlobalMetaKeyMaps)
	{
		if (n.Type == box->type)
		{
			name = n.Name;
			break;
		}
	}

	unsigned sz = (unsigned)box->size - 8;
	if (itunes)
	{
		unsigned size, tag;
		if (!StmReadUI32(&size))
			return ISOM_ERR_COMMON_IO;
		if (!StmReadUI32(&tag))
			return ISOM_ERR_COMMON_IO;

		if (ISOM_SWAP32(tag) == ISOM_FCC('data') && size <= sz)
		{
			unsigned type;
			if (!StmReadUI32(&type))
				return ISOM_ERR_COMMON_IO;
			if (!StmReadSkip(4))
				return ISOM_ERR_COMMON_IO;

			if (size > 16)
			{
				size -= 16;
				if (box->type == iTunes::apple_tag_cover)
				{
					Tag pic = {};
					pic.Type = box->type;
					pic.Name = "cover";
					pic.DataPointer = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(size));
					if (pic.DataPointer)
					{
						pic.Length = _stream->Read(pic.DataPointer, size);
						if (!_tags.Add(&pic))
							return ISOM_ERR_COMMON_MEMORY;
					}
				}else{
					Tag t = {};
					t.Type = box->type;
					t.Name = name;
					t.DataPointer = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED(size + 32));
					if (t.DataPointer)
					{
						memset(t.DataPointer, 0, size + 32);
						t.Length = _stream->Read(t.DataPointer, size);
						if (box->type == ISOM_FCC('trkn') || box->type == ISOM_FCC('disk') && t.Length > 7)
						{
							uint16_t cur = *(uint16_t*)(t.DataPointer + 2);
							uint16_t total = *(uint16_t*)(t.DataPointer + 4);
							cur = ISOM_SWAP16(cur);
							total = ISOM_SWAP16(total);
							sprintf((char*)t.DataPointer, "%d,%d", cur, total);
						}
						if (name)
							t.StringPointer = (char*)t.DataPointer;
						if (!_tags.Add(&t))
							return ISOM_ERR_COMMON_MEMORY;
					}
				}
			}
		}
	}

	return ISOM_ERR_OK;
}