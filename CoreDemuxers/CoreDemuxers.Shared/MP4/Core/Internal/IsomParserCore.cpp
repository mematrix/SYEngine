#include "IsomParser.h"

using namespace Isom;
using namespace Isom::Types;
using namespace Isom::Object;
using namespace Isom::Object::Internal;

typedef unsigned (Parser::*ProcessBoxCallback)(BoxObject* box, BoxObject* parent);
struct BoxCallback
{
	unsigned BoxType;
	ProcessBoxCallback Callback;
};
static const BoxCallback GlobalBoxCallbacks[] = {
	{isom_box_container_trak, &Parser::Internal_Process_trak},
	{isom_box_content_ftyp, &Parser::Internal_Process_ftyp},
	{isom_box_content_mvhd, &Parser::Internal_Process_mvhd},
	{isom_box_content_tkhd, &Parser::Internal_Process_tkhd},
	{isom_box_content_mdhd, &Parser::Internal_Process_mdhd},
	{isom_box_content_hdlr, &Parser::Internal_Process_hdlr},
	{isom_box_content_stts, &Parser::Internal_Process_stts},
	{isom_box_content_stss, &Parser::Internal_Process_stss},
	{isom_box_content_stsz, &Parser::Internal_Process_stsz},
	{isom_box_content_stco, &Parser::Internal_Process_stco},
	{isom_box_content_stsc, &Parser::Internal_Process_stsc},
	{isom_box_content_ctts, &Parser::Internal_Process_ctts},
	{isom_box_content_co64, &Parser::Internal_Process_stco},
	{isom_box_content_stz2, &Parser::Internal_Process_stsz},
	{isom_box_content_elst, &Parser::Internal_Process_elst},
	{isom_box_content_url, &Parser::Internal_Process_url},
	{isom_box_content_smhd, &Parser::Internal_Process_smhd},
	{isom_box_content_vmhd, &Parser::Internal_Process_vmhd},
	{isom_box_content_chpl, &Parser::Internal_Process_chpl},
	{isom_box_content_uuid, &Parser::Internal_Process_uuid},
	{isom_box_content_wide, &Parser::Internal_Process_wide}
};

unsigned Parser::OnProcessContainer(BoxObject* box, BoxObject* parent)
{
	if (box->type == isom_box_container_moov)
	{
		if (_found_moov)
			return ISOM_ERR_SKIP; //moov box已经存在，跳过重复的
		_found_moov = true;
	}else if (box->type == isom_box_container_trak)
	{
		return OnProcessContent(box, parent); //创建新的track添加到list
	}else if (box->type == isom_box_container_meta)
	{
		if (box->size < INT32_MAX && box->size >= 12)
		{
			if (!StmReadSkip(4))
				return ISOM_ERR_COMMON_IO;
			box->size -= 4;
		}
	}else if (box->type == isom_box_container_dref || box->type == isom_box_container_stsd)
	{
		if ((box->size - 8) > 8)
		{
			if (!GetBoxVersionAndFlags(nullptr, nullptr))
				return ISOM_ERR_COMMON_DATA;
			if (!StmReadSkip(4)) //box count
				return ISOM_ERR_COMMON_IO;
			box->size -= 8;
		}
	}
	return ISOM_ERR_OK;
}

unsigned Parser::OnProcessContainerEnded(BoxObject* box, BoxObject* parent)
{
	if (box->type == isom_box_container_moov)
	{
		if (_found_mdat)
			return ISOM_ERR_STOP; //如果已经找到mdat，又已经处理完moov，可以停止parse过程
	}else if (box->type == isom_box_container_trak)
	{
		if (_current_track)
		{
			//处理完trak的box数据，现在开始创建样本表
			unsigned result = BuildTrackIndexTable();
			if (result != ISOM_ERR_OK)
				return result;

			//修正全局信息的timescale
			if (_header.Movie.TimeScale <= 0)
			{
				_header.Movie.TimeScale = _current_track->MediaInfo.TimeScale;
				if (_header.Movie.TimeScale <= 0)
					_header.Movie.TimeScale = 1;
			}
		}
	}
	return ISOM_ERR_OK;
}

unsigned Parser::OnProcessContent(BoxObject* box, BoxObject* parent)
{
	for (auto cb : GlobalBoxCallbacks)
		if (cb.BoxType == box->type)
			return (this->*cb.Callback)(box, parent);
	return ISOM_ERR_OK;
}

unsigned Parser::OnProcessSkipped(BoxObject* box, BoxObject* parent)
{
	if (box->type == isom_box_content_mdat)
	{
		_found_mdat = true;
		if (_found_moov)
			return ISOM_ERR_STOP; //如果已经处理完moov，现在又已经找到mdat，则可以停止parse过程
	}
	if (parent->type == isom_box_container_stsd)
	{
		if (_current_track)
		{
			//仅处理stsd中的第一个sample-entry
			if (_current_track->CodecSampleEntry == nullptr)
				return SaveTrackSampleEntry(box);
			else
				_current_track->MultiStsdDesc = true; //如果有多个entry，设置一个flag
		}
	}else if (parent->type == isom_box_container_udta || parent->type == isom_box_container_ilst)
	{
		//处理iTunes元数据，如alac音频
		return OnMetadataBox(box,parent->type == isom_box_container_ilst);
	}
	return ISOM_ERR_OK;
}

double Parser::CalcStartTimeOffsetFromElst()
{
	if (_current_track->Temp.elst == nullptr || _current_track->Temp.elstCount == 0)
		return 0.0;

	double result = 0.0;
	unsigned start_index = 0;
	for (unsigned i = 0; i < _current_track->Temp.elstCount; i++)
	{
		auto p = &_current_track->Temp.elst[i];
		if (i == 0 && p->time == -1)
		{
			start_index = 1;
		}else if (i == start_index && p->time > 0) {
			auto st = p->time;
			if (_current_track->MediaInfo.TimeScale > 0)
				result = double(st) / double(_current_track->MediaInfo.TimeScale);
			break;
		}
	}
	return result;
}

unsigned Parser::CalcSampleCountFromChunks(int stsd_index)
{
	unsigned result = 0;
	if (_current_track->Temp.chunkCount == 0)
		return 0;

	for (unsigned i = 0; i < _current_track->Temp.stscCount; i++)
	{
		auto p = &_current_track->Temp.stsc[i];
		if (stsd_index != -1)
			if (p->id != stsd_index)
				continue;

		if (p->first > 0)
		{
			int current_index = p->first - 1;
			int next_index = (i + 1 == _current_track->Temp.stscCount) ? 
				_current_track->Temp.chunkCount + 1 : _current_track->Temp.stsc[i + 1].first;
			next_index--;
			if (next_index <= 0)
				break;

			result += ((next_index - current_index) * p->count);
		}
	}
	return result;
}

unsigned Parser::BuildTrackIndexTable()
{
	if (_current_track->Type == Track::TrackType_Audio)
		if (_current_track->Temp.sttsCount == 1)
			if (_current_track->Temp.stts[0].duration == 1)
				return ISOM_ERR_OK; //QuickTime PCM is unsupported.

	/*
	 算法说明：
	 1、根循环为for -> chunkCount，递增值i，0下标开始，全局索引变量sample_index = 0；
	 2、循环遍历stsc对象，取first == i的stsc对象索引stsc_index，取stsc_index + 1的下一个stsc对象的first，如果+1的值对于stsc对象已经结束，取chunkCount作为first；
	 3、使用+1的stsc对象的fitst - 当前stsc对象的first，记为chunk_count，取当前stsc对象的samples_per_chunk；
	 4、子循环for -> chunk_count，递增值依然是i，i < chunk_count；
	 5、当前i指向的chunkOffset命名为sample_offset；
	 6、子循环for -> samples_per_chunk，此循环退出，sample_offset更新为下一个chunkOffset[i]；
	 7、根据sample_index从stsz里面获取sample_size，下标sample_index的Sample的pos = sample_offset，sample_offset += sample_size，sample_index递增，以此类....；
	*/

	unsigned fixed_sample_size = _current_track->Temp.stszSampleSize;
	if (_current_track->Samples)
		return ISOM_ERR_OK;
	if (_current_track->Temp.stszCount == 0 && fixed_sample_size == 0)
		return ISOM_ERR_OK;
	unsigned count = _current_track->Temp.stszCount;
	if (count > 0) 
		fixed_sample_size = 0;
	else
		count = CalcSampleCountFromChunks();

	if (count >= UINT32_MAX / sizeof(IndexObject))
		return ISOM_ERR_COMMON_INDEX;
	_current_track->TimeStartOffset = (float)CalcStartTimeOffsetFromElst();

	_current_track->Samples = (IndexObject*)malloc((count + 4) * sizeof(IndexObject));
	if (_current_track->Samples == nullptr)
		return ISOM_ERR_COMMON_MEMORY;
	memset(_current_track->Samples, 0, count * sizeof(IndexObject));

	double time_scale = (double)_current_track->MediaInfo.TimeScale;
	unsigned sample_index = 0; //sample count
	unsigned stts_index = 0, ctts_index = 0, stss_index = 0;
	unsigned stts_iterator = 0, ctts_iterator = 0;
	unsigned stsc_index = 0;
	int64_t time_offset = 0;
	for (unsigned i = 0; i < _current_track->Temp.chunkCount; i++)
	{
		bool found_stsc = false;
		for (unsigned j = stsc_index; j < _current_track->Temp.stscCount; j++)
		{
			if (_current_track->Temp.stsc[j].first - 1 == i)
			{
				found_stsc = true;
				stsc_index = j;
				break;
			}
		}
		if (!found_stsc)
			break;

		int cur_stsc_cindex = _current_track->Temp.stsc[stsc_index].first - 1;
		int next_stsc_cindex = (stsc_index + 1 == _current_track->Temp.stscCount) ? 
				_current_track->Temp.chunkCount : _current_track->Temp.stsc[stsc_index + 1].first - 1;

		unsigned sample_per_chunk = _current_track->Temp.stsc[stsc_index].count;
		unsigned stsd_index = _current_track->Temp.stsc[stsc_index].id;
		int chunk_count = next_stsc_cindex - cur_stsc_cindex;
		for (int c = 0; c < chunk_count; c++, i++)
		{
			auto sample_offset = _current_track->Temp.chunkOffsets[i];
			for (unsigned n = 0; n < sample_per_chunk; n++)
			{
				unsigned sample_size = 
					fixed_sample_size ? fixed_sample_size : _current_track->Temp.stsz[sample_index];
				_current_track->TotalStreamSize += sample_size;

				auto p = &_current_track->Samples[sample_index];
				p->timestamp = time_scale <= 0 ? -1.0 : double(time_offset) / time_scale;
				p->size = sample_size;
				p->pos = sample_offset;
				p->stsd_index = stsd_index;

				if (_current_track->Temp.stssCount > 0 && stss_index < _current_track->Temp.stssCount)
				{
					if ((sample_index + 1) == _current_track->Temp.stss[stss_index])
					{
						p->keyframe = true;
						stss_index++;
					}
				}

				//calc dts.
				if (_current_track->Temp.stts && stts_index < _current_track->Temp.sttsCount)
				{
					time_offset += _current_track->Temp.stts[stts_index].duration;
					stts_iterator++;
					if (stts_iterator == _current_track->Temp.stts[stts_index].count)
					{
						stts_iterator = 0;
						stts_index++;
					}
				}
				//save pts-offset.
				if (_current_track->Temp.ctts && ctts_index < _current_track->Temp.cttsCount)
				{
					int cts = _current_track->Temp.ctts[ctts_index].duration;
					ctts_iterator++;
					if (ctts_iterator == _current_track->Temp.ctts[ctts_index].count)
					{
						ctts_iterator = 0;
						ctts_index++;
					}
					if (time_scale > 0)
						p->pts_offset = float(double(cts) / time_scale);
				}

				sample_offset += sample_size;
				sample_index++;
				if (sample_index >= count)
					goto end;
			}
		}
		i--;
	}
end:
	_current_track->SampleCount = sample_index;
	_current_track->Temp.FreeResources();

	return ISOM_ERR_OK;
}

unsigned Parser::SaveTrackSampleEntry(BoxObject* box)
{
	if (box->size <= 8)
		return ISOM_ERR_OK;

	_current_track->CodecTag = box->type;
	_current_track->CodecSampleEntry = (unsigned char*)malloc(ISOM_ALLOC_ALIGNED((unsigned)box->size));
	if (_current_track->CodecSampleEntry == nullptr)
		return ISOM_ERR_COMMON_MEMORY;
	memset(_current_track->CodecSampleEntry, 0, (unsigned)box->size);
	if (_stream->Read(_current_track->CodecSampleEntry, (unsigned)box->size - 8) != (unsigned)(box->size - 8))
		return ISOM_ERR_COMMON_DATA;
	_current_track->CodecSampleEntrySize = (unsigned)box->size - 8;

	if (_current_track->Type == Track::TrackType_Unknown)
	{
		if (box->type == ISOM_FCC('tx3g') || box->type == ISOM_FCC('text'))
			_current_track->Type = Track::TrackType_Subtitle;
		else if (box->type == ISOM_FCC('avc1') || box->type == ISOM_FCC('hvc1') || box->type == ISOM_FCC('hev1') || box->type == ISOM_FCC('mp4v'))
			_current_track->Type = Track::TrackType_Video;
		else if (box->type == ISOM_FCC('mp4a') || box->type == ISOM_FCC('alac') || box->type == ISOM_FCC('ac-3'))
			_current_track->Type = Track::TrackType_Audio;
	}

	_current_track->CodecInfo = new(std::nothrow) CodecSampleEntryParser();
	if (_current_track->CodecInfo == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	_current_track->CodecInfo->
		ParseSampleEntry(box->type, _current_track->CodecSampleEntry, _current_track->CodecSampleEntrySize);
	return ISOM_ERR_OK;
}

bool Parser::GetBoxVersionAndFlags(int* version, int* flags)
{
	unsigned v;
	unsigned f;
	if (!StmReadUI08(&v))
		return false;
	if (!StmReadUI24(&f))
		return false;
	if (version)
		*version = (int)v;
	if (flags)
		*flags = (int)f;
	return true;
}

unsigned Parser::Internal_Process_ftyp(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_root || box->size < 16)
		return ISOM_ERR_OK;
	if (_header.File.MajorBand != 0)
		return ISOM_ERR_OK;

	_stream->Read(&_header.File.MajorBand,4);
	StmReadUI32(&_header.File.MinorVersion);

	unsigned psize = (unsigned)box->size - 16;
	if (psize > 0 && psize < ISOM_ARRAY_COUNT(_header.File.CompBands))
		_stream->Read(_header.File.CompBands, psize);

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_mvhd(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_moov)
		return ISOM_ERR_OK;

	int ver = 0;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;

	uint64_t t64 = 0;
	uint32_t t32 = 0;
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_header.Movie.CreationTime = t64;
		StmReadUI64(&t64);
		_header.Movie.ModificationTime = t64;
	}else{
		StmReadUI32(&t32);
		_header.Movie.CreationTime = t32;
		StmReadUI32(&t32);
		_header.Movie.ModificationTime = t32;
	}

	StmReadUI32((uint32_t*)&_header.Movie.TimeScale);
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_header.Movie.Duration = t64;
	}else{
		StmReadUI32(&t32);
		_header.Movie.Duration = t32;
	}

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_tkhd(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_trak || _current_track == nullptr)
		return ISOM_ERR_OK;

	int ver = 0, flags = 0;
	if (!GetBoxVersionAndFlags(&ver, &flags))
		return ISOM_ERR_COMMON_DATA;

	_current_track->Header.Flags = flags;
	uint64_t t64 = 0;
	uint32_t t32 = 0;
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_current_track->Header.CreationTime = t64;
		StmReadUI64(&t64);
		_current_track->Header.ModificationTime = t64;
	}else{
		StmReadUI32(&t32);
		_current_track->Header.CreationTime = t32;
		StmReadUI32(&t32);
		_current_track->Header.ModificationTime = t32;
	}

	StmReadUI32((uint32_t*)&_current_track->Header.Id);
	StmReadSkip(4);
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_current_track->Header.Duration = t64;
	}else{
		StmReadUI32(&t32);
		_current_track->Header.Duration = t32;
	}

	StmReadSkip(8);
	StmReadSkip(8);

	//from android::stagefright
	int32_t a00 = 0, a01 = 0, a10 = 0, a11 = 0;
	StmReadUI32((uint32_t*)&a00);
	StmReadUI32((uint32_t*)&a01);
	StmReadSkip(4);
	StmReadUI32((uint32_t*)&a10);
	StmReadUI32((uint32_t*)&a11);

	int rotationDegrees = 0;
	static const int32_t kFixedOne = 0x10000;
	if (a00 == 0 && a01 == kFixedOne && a10 == -kFixedOne && a11 == 0)
		rotationDegrees = 90;
	else if (a00 == 0 && a01 == -kFixedOne && a10 == kFixedOne && a11 == 0)
		rotationDegrees = 270;
	else if (a00 == -kFixedOne && a01 == 0 && a10 == 0 && a11 == -kFixedOne)
		rotationDegrees = 180;

	_current_track->VideoRotation = rotationDegrees;
	StmReadSkip(9 * 4 - 20); //other matrix

	StmReadUI32((uint32_t*)&_current_track->Header.Width);
	StmReadUI32((uint32_t*)&_current_track->Header.Height);
	_current_track->Header.Width >>= 16;
	_current_track->Header.Height >>= 16;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_mdhd(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_mdia || _current_track == nullptr)
		return ISOM_ERR_OK;
	if (_current_track->MediaInfo.TimeScale != 0)
		return ISOM_ERR_COMMON_DATA;

	int ver = 0;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;
	if (ver > 1)
		return ISOM_ERR_COMMON_DATA;

	uint64_t t64 = 0;
	uint32_t t32 = 0;
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_current_track->MediaInfo.CreationTime = t64;
		StmReadUI64(&t64);
		_current_track->MediaInfo.ModificationTime = t64;
	}else{
		StmReadUI32(&t32);
		_current_track->MediaInfo.CreationTime = t32;
		StmReadUI32(&t32);
		_current_track->MediaInfo.ModificationTime = t32;
	}

	StmReadUI32((uint32_t*)&_current_track->MediaInfo.TimeScale);
	if (ver == 1)
	{
		StmReadUI64(&t64);
		_current_track->MediaInfo.Duration = t64;
	}else{
		StmReadUI32(&t32);
		_current_track->MediaInfo.Duration = t32;
	}

	StmReadUI16((uint32_t*)&_current_track->MediaInfo.LangId);
	StmReadSkip(2);

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_hdlr(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_mdia || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	StmReadSkip(4);
	unsigned type = 0;
	StmReadUI32(&type);
	type = ISOM_SWAP32(type);
	if (type == isom_type_name_vide)
		_current_track->Type = Track::TrackType_Video;
	else if (type == isom_type_name_soun)
		_current_track->Type = Track::TrackType_Audio;
	else if (type == isom_type_name_subp)
		_current_track->Type = Track::TrackType_Subtitle;

	StmReadSkip(3 * 4);
	unsigned tsize = (unsigned)box->size - 24;
	if (strncmp(_header.File.CompBands, "qt  ", 4) == 0)
	{
		StmReadSkip(1);
		tsize--;
	}
	if (tsize < ISOM_ARRAY_COUNT(_current_track->HandlerName))
		_stream->Read(_current_track->HandlerName, tsize);

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_stts(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;
	if (count == 0)
		return ISOM_ERR_OK;

	if (_current_track->Temp.stts)
		free(_current_track->Temp.stts);

	unsigned total_size = count * 8;
	_current_track->Temp.sttsCount = 0;
	_current_track->Temp.stts = (sttsObject*)malloc(total_size + 8);
	if (_current_track->Temp.stts == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	if (_stream->Read(_current_track->Temp.stts, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;
	
	int64_t total_duration = 0;
	int32_t total_frames = 0;
	for (unsigned i = 0; i < count; i++)
	{
		int sample_count = (&_current_track->Temp.stts[i])->count;
		int sample_duration = (&_current_track->Temp.stts[i])->duration;
		sample_count = ISOM_SWAP32(sample_count);
		sample_duration = ISOM_SWAP32(sample_duration);

		if (sample_duration < 0)
			sample_duration = 1;
		if (sample_count < 0)
			return ISOM_ERR_COMMON_INDEX;

		if (sample_duration > 0)
		{
			total_duration += (int64_t)sample_count * sample_duration;
			total_frames += sample_count;
		}

		_current_track->Temp.stts[i].count = sample_count;
		_current_track->Temp.stts[i].duration = sample_duration;
	}
	_current_track->TotalDuration = total_duration;
	_current_track->TotalFrames = total_frames;

	_current_track->Temp.sttsCount = count;
	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_stss(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;

	_current_track->KeyFrameExists = false;
	if (count == 0)
		return ISOM_ERR_OK;

	if (_current_track->Temp.stss)
		free(_current_track->Temp.stss);

	unsigned total_size = count * 4;
	_current_track->Temp.stssCount = 0;
	_current_track->Temp.stss = (unsigned*)malloc(total_size + 4);
	if (_current_track->Temp.stss == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	if (_stream->Read(_current_track->Temp.stss, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;

	for (unsigned i = 0; i < count; i++)
		_current_track->Temp.stss[i] = ISOM_SWAP32(_current_track->Temp.stss[i]);
	_current_track->Temp.stssCount = count;
	_current_track->KeyFrameExists = true;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_stsz(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned field_size = 0;
	unsigned sample_size = 0;
	if (box->type == isom_box_content_stsz)
	{
		if (!StmReadUI32(&sample_size))
			return ISOM_ERR_COMMON_IO;
		field_size = 32;
	}else{
		StmReadSkip(3);
		if (!StmReadUI08(&field_size))
			return ISOM_ERR_COMMON_IO;
	}
	_current_track->Temp.stszSampleSize = sample_size;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;

	if (count == 0 || sample_size > 0)
		return ISOM_ERR_OK;
	if (field_size != 8 && field_size != 16 && field_size != 32)
		return ISOM_ERR_OK;

	if (_current_track->Temp.stsz)
		free(_current_track->Temp.stsz);

	unsigned total_size = count * (field_size / 8);
	_current_track->Temp.stszCount = 0;
	_current_track->Temp.stsz = (unsigned*)malloc(count * 4 + 4);
	if (_current_track->Temp.stsz == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	auto tp = malloc(total_size);
	if (tp == nullptr)
		return ISOM_ERR_COMMON_MEMORY;
	if (_stream->Read(tp, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;

	auto ui8 = (uint8_t*)tp;
	auto ui16 = (uint16_t*)tp;
	auto ui32 = (uint32_t*)tp;
	for (unsigned i = 0; i < count; i++)
	{
		unsigned temp;
		switch (field_size)
		{
		case 8:
			temp = ui8[i];
			break;
		case 16:
			temp = ISOM_SWAP16(ui16[i]);
			break;
		default:
			temp = ui32[i];
			temp = ISOM_SWAP32(temp);
		}
		_current_track->Temp.stsz[i] = temp;
	}
	_current_track->Temp.stszCount = count;
	free(tp);

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_stco(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;
	if (count == 0)
		return ISOM_ERR_OK;

	unsigned total_size = count * (box->type == isom_box_content_stco ? 4 : 8);
	_current_track->Temp.chunkCount = 0;
	_current_track->Temp.chunkOffsets = (int64_t*)malloc(8 * count + 8);
	if (_current_track->Temp.chunkOffsets == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	auto tp = malloc(total_size);
	if (tp == nullptr)
		return ISOM_ERR_COMMON_MEMORY;
	if (_stream->Read(tp, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;

	auto ui32 = (uint32_t*)tp;
	auto ui64 = (uint64_t*)tp;
	for (unsigned i = 0; i < count; i++)
	{
		if (box->type == isom_box_content_stco)
			_current_track->Temp.chunkOffsets[i] = ISOM_SWAP32(ui32[i]);
		else //co64
			_current_track->Temp.chunkOffsets[i] = ISOM_SWAP64(ui64[i]);
	}
	_current_track->Temp.chunkCount = count;
	free(tp);

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_stsc(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;
	if (count == 0)
		return ISOM_ERR_OK;

	if (_current_track->Temp.stsc)
		free(_current_track->Temp.stsc);

	unsigned total_size = count * 12;
	_current_track->Temp.stscCount = 0;
	_current_track->Temp.stsc = (stscObject*)malloc(total_size + 12);
	if (_current_track->Temp.stsc == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	if (_stream->Read(_current_track->Temp.stsc, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;

	for (unsigned i = 0; i < count; i++)
	{
		auto p = &_current_track->Temp.stsc[i];
		p->first = ISOM_SWAP32(p->first);
		p->count = ISOM_SWAP32(p->count);
		p->id = ISOM_SWAP32(p->id);
	}
	_current_track->Temp.stscCount = count;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_ctts(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_stbl || _current_track == nullptr)
		return ISOM_ERR_OK;

	if (!GetBoxVersionAndFlags(nullptr, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;
	if (count == 0)
		return ISOM_ERR_OK;

	if (_current_track->Temp.ctts)
		free(_current_track->Temp.ctts);

	unsigned total_size = count * 8;
	_current_track->Temp.cttsCount = 0;
	_current_track->Temp.ctts = (sttsObject*)malloc(total_size + 8);
	if (_current_track->Temp.ctts == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	if (_stream->Read(_current_track->Temp.ctts, total_size) != total_size)
		return ISOM_ERR_COMMON_IO;

	for (unsigned i = 0; i < count; i++)
	{
		int sample_count = (&_current_track->Temp.ctts[i])->count;
		int sample_duration = (&_current_track->Temp.ctts[i])->duration;
		sample_count = ISOM_SWAP32(sample_count);
		sample_duration = ISOM_SWAP32(sample_duration);

		_current_track->Temp.ctts[i].count = sample_count;
		_current_track->Temp.ctts[i].duration = sample_duration;
	}
	_current_track->Temp.cttsCount = count;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_elst(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_edts || _current_track == nullptr)
		return ISOM_ERR_OK;

	int ver = 0;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;

	unsigned count = 0;
	if (!StmReadUI32(&count))
		return ISOM_ERR_COMMON_IO;
	if (count == 0)
		return ISOM_ERR_OK;

	if (_current_track->Temp.elst)
		free(_current_track->Temp.elst);

	_current_track->Temp.elstCount = 0;
	_current_track->Temp.elst = (elstObject*)malloc(sizeof(elstObject) * (count + 1));
	if (_current_track->Temp.elst == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	memset(_current_track->Temp.elst, 0, sizeof(elstObject) * count);
	for (unsigned i = 0; i < count; i++)
	{
		unsigned temp;
		auto e = &_current_track->Temp.elst[i];
		if (ver == 1)
		{
			if (!StmReadUI64((uint64_t*)&e->duration))
				return ISOM_ERR_COMMON_IO;
			if (!StmReadUI64((uint64_t*)&e->time))
				return ISOM_ERR_COMMON_IO;
		}else{
			if (!StmReadUI32(&temp))
				return ISOM_ERR_COMMON_IO;
			e->duration = temp;
			if (!StmReadUI32(&temp))
				return ISOM_ERR_COMMON_IO;
			e->time = temp;
		}
		if (!StmReadUI32(&temp))
			return ISOM_ERR_COMMON_IO;
		e->rate = float(temp) / 65536.0f;
	}
	_current_track->Temp.elstCount = count;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_chpl(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_udta)
		return ISOM_ERR_OK;

	int ver = 0;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;

	if (ver)
		StmReadSkip(4); //???

	unsigned count;
	if (!StmReadUI08(&count))
		return ISOM_ERR_COMMON_IO;

	for (unsigned i = 0; i < count; i++)
	{
		int64_t start_time;
		if (!StmReadUI64((uint64_t*)&start_time))
			return ISOM_ERR_COMMON_IO;

		unsigned str_len;
		if (!StmReadUI08(&str_len))
			return ISOM_ERR_COMMON_IO;

		Chapter chap = {};
		chap.Timestamp = double(start_time) / 10000000.0;
		chap.Title = (char*)malloc(str_len * 2);
		if (chap.Title == nullptr)
			return ISOM_ERR_COMMON_MEMORY;
		memset(chap.Title, 0, str_len * 2);
		if (_stream->Read(chap.Title, str_len) != str_len)
			break;

		if (!_chaps.Add(&chap))
			return ISOM_ERR_COMMON_MEMORY;
	}

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_url(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_dref || _current_track == nullptr)
		return ISOM_ERR_OK;

	int flags = 1;
	if (!GetBoxVersionAndFlags(nullptr, &flags))
		return ISOM_ERR_OK;

	if ((flags & 1) == 0)
	{
		if (box->size > 12)
		{
			_current_track->LocationUrl = (char*)malloc(((unsigned)box->size - 12) * 2);
			if (_current_track->LocationUrl == nullptr)
				return ISOM_ERR_COMMON_MEMORY;
			memset(_current_track->LocationUrl, 0, (unsigned)box->size);
			_stream->Read(_current_track->LocationUrl, (unsigned)box->size - 12);
		}
	}

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_smhd(BoxObject* box, BoxObject* parent)
{
	if (_current_track == nullptr)
		return ISOM_ERR_OK;

	int ver;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;
	if (ver != 0)
		return ISOM_ERR_OK;

	unsigned temp;
	if (!StmReadUI16(&temp))
		return ISOM_ERR_COMMON_IO;
	_current_track->AudioBalance = temp;

	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_vmhd(BoxObject* box, BoxObject* parent)
{
	if (_current_track == nullptr)
		return ISOM_ERR_OK;

	int ver;
	if (!GetBoxVersionAndFlags(&ver, nullptr))
		return ISOM_ERR_COMMON_DATA;
	if (ver != 0)
		return ISOM_ERR_OK;

	unsigned temp;
	if (!StmReadUI16(&temp))
		return ISOM_ERR_COMMON_IO;
	_current_track->VideoGraphicsMode = temp;

	if (_stream->Read(&_current_track->VideoOpColor, 6) != 6)
		return ISOM_ERR_COMMON_IO;

	_current_track->VideoOpColor[0] = ISOM_SWAP16(_current_track->VideoOpColor[0]);
	_current_track->VideoOpColor[1] = ISOM_SWAP16(_current_track->VideoOpColor[1]);
	_current_track->VideoOpColor[2] = ISOM_SWAP16(_current_track->VideoOpColor[2]);
	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_uuid(BoxObject* box, BoxObject* parent)
{
	unsigned sz = (unsigned)box->size - 8;
	if (sz > 20)
	{
		unsigned temp;
		if (!StmReadUI32(&temp))
			return ISOM_ERR_COMMON_IO;
		if (temp != 0x5CA708FB)
			return ISOM_ERR_OK;

		if (!StmReadSkip(12))
			return ISOM_ERR_COMMON_IO;
		if (!StmReadUI32(&temp))
			return ISOM_ERR_COMMON_IO;

		sz -= 20;
		if (sz == 0 || temp > sz)
			return ISOM_ERR_OK;

		Tag t = {};
		t.Length = temp + 1;
		t.Name = "winver";
		t.DataPointer = (unsigned char*)malloc(temp * 2);
		t.StringPointer = (char*)t.DataPointer;
		if (t.StringPointer)
		{
			memset(t.StringPointer, 0, temp * 2);
			_stream->Read(t.StringPointer, temp);
			if (!_tags.Add(&t))
				return ISOM_ERR_COMMON_MEMORY;
			if (strstr(t.StringPointer, "msft") != nullptr &&
				strstr(t.StringPointer, "ismv") != nullptr)
				return ISOM_ERR_COMMON_HEAD;
		}
	}
	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_wide(BoxObject* box, BoxObject* parent)
{
	if (box->size >= 16)
	{
		unsigned temp;
		StmReadUI32(&temp);
		if (temp == 0)
		{
			if (_stream->Read(&temp, 4) != 4)
				return ISOM_ERR_OK;
			if (temp != ISOM_FCC('mdat'))
				return ISOM_ERR_OK;
			_found_mdat = true;
		}
	}
	return ISOM_ERR_OK;
}

unsigned Parser::Internal_Process_trak(BoxObject* box, BoxObject* parent)
{
	if (parent->type != isom_box_container_moov)
		return ISOM_ERR_COMMON_DATA;

	_current_track = _tracks.New();
	if (_current_track == nullptr)
		return ISOM_ERR_COMMON_MEMORY;

	return ISOM_ERR_OK;
}