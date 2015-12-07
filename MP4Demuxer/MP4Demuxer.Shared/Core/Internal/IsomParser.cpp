#include "IsomParser.h"

#define MAX_PARSE_BOX_DEPTH 14
#define BREAK_IF_RESULT_STOP(r) \
	if ((r) == ISOM_ERR_STOP) break;
#define RETURN_IF_RESULT_FAILED(r) \
	if ((r) != ISOM_ERR_OK) return (r);

using namespace Isom;
using namespace Isom::Types;
using namespace Isom::Object;
using namespace Isom::Object::Internal;

static const unsigned GlobalContainerBoxs[] = {
	isom_box_container_moov,
	isom_box_container_trak,
	isom_box_container_mdia,
	isom_box_container_minf,
	isom_box_container_stbl,
	isom_box_container_dinf,
	isom_box_container_edts,
	isom_box_container_udta,
	isom_box_container_meta,
	isom_box_container_ilst,
	isom_box_container_esds,
	isom_box_container_stsd,
	isom_box_container_dref
};

unsigned Parser::Parse() throw()
{
	_found_moov = _found_mdat = false;
	_current_track = nullptr;
	memset(&_header, 0, sizeof(_header));

	if (_stream == nullptr)
		return ISOM_ERR_FAIL;

	BoxObject file;
	file.type = isom_box_container_root;
	file.size = _stream->GetSize();
	if (file.size < 16 && !_stream->IsFromNetwork())
		return ISOM_ERR_COMMON_SIZE;
	if (file.size < 16)
		file.size = INT64_MAX;

	unsigned result = ParseContainer(&file, 0);
	if (result == ISOM_ERR_COMMON_IO)
	{
		if (file.size == _stream->Tell())
			result = ISOM_ERR_OK;
	}

	bool skip_no_mdat = false;
	if (_found_moov && !_found_mdat)
	{
		if (_tracks.GetCount() > 0)
			if (_tracks.Get(0)->SampleCount > 1)
				skip_no_mdat = true;
	}
	if (!_found_moov || (!_found_mdat && !skip_no_mdat))
		result = ISOM_ERR_COMMON_HEAD;

	return result;
}

void Parser::Free() throw()
{
	_current_track = nullptr;
	_found_moov = _found_mdat = false;

	for (unsigned i = 0; i < _tracks.GetCount(); i++)
	{
		auto p = _tracks[i];
		if (p)
			p->FreeResources();
	}
	for (unsigned i = 0; i < _chaps.GetCount(); i++)
	{
		auto p = _chaps[i];
		if (p)
			p->FreeResources();
	}
	for (unsigned i = 0; i < _tags.GetCount(); i++)
	{
		auto p = _tags[i];
		if (p)
			p->FreeResources();
	}
}

bool Parser::IsContainerBox(unsigned box_type)
{
	for (auto id : GlobalContainerBoxs)
		if (id == box_type)
			return true;
	return false;
}

unsigned Parser::ParseContainer(BoxObject* parent, int depth, unsigned head_offset_size)
{
	if (depth > MAX_PARSE_BOX_DEPTH)
		return ISOM_ERR_COMMON_DEPTH;

	auto bytes_able = parent->size - head_offset_size;
	while (1)
	{
		if (bytes_able <= 0)
			break;

		BoxObject box;
		unsigned s32;
		unsigned bsize_read_io_value_offset = 8;

		if (!StmReadUI32(&s32))
			return ISOM_ERR_COMMON_IO;
		if (_stream->Read(&box.type, 4) != 4)
			return ISOM_ERR_COMMON_IO;

		box.size = s32;
		if (box.size == 0)
		{
			box.size = _stream->GetSize() - _stream->Tell() + 8;
			if (box.size < 8)
				return ISOM_ERR_COMMON_SIZE;
		}else if (box.size == 1) {
			if (!StmReadUI64((uint64_t*)&box.size))
				return ISOM_ERR_COMMON_IO;
			bsize_read_io_value_offset = 16;
		}

		if (box.size > bytes_able)
			box.size = bytes_able;

		auto box_cur_pos = _stream->Tell();
		auto box_end_pos = box_cur_pos + box.size - bsize_read_io_value_offset;

		// Process Functions //
		/*
#ifdef _MSC_VER
#ifdef _DEBUG
		char dbg[512] = {};
		memset(dbg, ' ', 512);
		char name[8] = {};
		*(unsigned*)name = box.type;
		sprintf(&dbg[depth], "%s\n",name);
		printf(dbg);
#endif
#endif
		*/

		unsigned result = ISOM_ERR_OK;
		if (IsContainerBox(box.type))
		{
			BoxObject new_box = box;
			result = OnProcessContainer(&new_box, parent);
			BREAK_IF_RESULT_STOP(result);
			RETURN_IF_RESULT_FAILED(result);

			if (result != ISOM_ERR_SKIP)
			{
				result = ParseContainer(&new_box, depth + 1, bsize_read_io_value_offset);
				RETURN_IF_RESULT_FAILED(result);

				result = OnProcessContainerEnded(&new_box, parent);
				BREAK_IF_RESULT_STOP(result);
				RETURN_IF_RESULT_FAILED(result);
			}
		}else{
			result = OnProcessContent(&box, parent);
			BREAK_IF_RESULT_STOP(result);
			RETURN_IF_RESULT_FAILED(result);
		}
		// Process Functions //

		if (_stream->Tell() != box_end_pos)
		{
			if (_stream->Tell() == box_cur_pos)
			{
				result = OnProcessSkipped(&box, parent);
				BREAK_IF_RESULT_STOP(result);
				RETURN_IF_RESULT_FAILED(result);
			}

			if (!_stream->Seek(box_end_pos))
				return ISOM_ERR_COMMON_SEEK;
		}

		bytes_able -= box.size;
	}

	return ISOM_ERR_OK;
}