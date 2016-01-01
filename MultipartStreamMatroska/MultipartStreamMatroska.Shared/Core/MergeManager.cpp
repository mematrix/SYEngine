#include "MergeManager.h"
#include <stdio.h>
#include <new.h>

bool MergeManager::PutNewInput(IOCallback* cb)
{
	if (cb == NULL)
		return false;
	IOCallback* old = _input;
	_input = cb;
	if (_demux == NULL)
		return true;
	if (!InternalNewDemux())
		return false;
	if (old != NULL)
		if (!OnNewInput(old, cb))
			return false;
	return true;
}

bool MergeManager::PutNewOutput(IOCallback* cb)
{
	if (cb == NULL)
		return false;
	IOCallback* old = _output;
	_output = cb;
	if (old != NULL)
		if (!OnNewOutput(old, cb))
			return false;
	return true;
}

bool MergeManager::ProcessHeader(double total_duration, bool auto_duration)
{
	_prev_audio_time = _prev_video_time = 0;
	_prev_audio_duration = _prev_video_duration = 0;
	_audio_time_offset = _video_time_offset = 0.0;

	if (_demux != NULL ||
		_input == NULL)
		return false;

	double demux_duation;
	if (!InternalNewDemux(&demux_duation))
		return false;

	_header.SetLength(0);
	if (!DoProcessHeader(auto_duration ? demux_duation : total_duration))
		return false;

	if (_header.GetLength() > 0 && _output->Size() == 0)
		return _output->Write(_header.Buffer()->Get<void*>(), _header.GetLength()) > 0;
	return true;
}

bool MergeManager::ProcessComplete(double total_duration)
{
	if (_demux == NULL)
		return false;
	if (!DoProcessComplete(total_duration))
		return false;

	delete _demux;
	_demux = NULL;

	_prev_audio_time = _prev_video_time = 0;
	_prev_audio_duration = _prev_video_duration = 0;
	_audio_time_offset = _video_time_offset = 0.0;
	return true;
}

void MergeManager::ProcessSkipPacket()
{
	if (_demux == NULL)
		return;
	Packet pkt;
	if (!_demux->ReadPacket(&pkt)) {
		_audio_time_offset = _audio_time_offset + 
			(double(_prev_audio_time + _prev_audio_duration) / double(_audio_ts)) - _start_time_offset;
		_video_time_offset = _video_time_offset + 
			(double(_prev_video_time + _prev_video_duration) / double(_video_ts)) - _start_time_offset;
	}
}

int MergeManager::ProcessPacketOnce(double* cur_timestamp)
{
	//处理一个帧并重新复用为MKV
	if (_demux == NULL)
		return 0;

loop:
	int64_t offset = _output->Tell();
	Packet pkt;
	if (!_demux->ReadPacket(&pkt)) {
		//如果一个分段已经结束，记录这个分段最后的时间戳和帧时长
		//生成下一个分段的时间偏移
		_audio_time_offset = _audio_time_offset + 
			(double(_prev_audio_time + _prev_audio_duration) / double(_audio_ts)) - _start_time_offset;
		_video_time_offset = _video_time_offset + 
			(double(_prev_video_time + _prev_video_duration) / double(_video_ts)) - _start_time_offset;

		_prev_video_time = _prev_video_time = 0;
		_prev_audio_duration = _prev_video_duration = 0;
		return 0;
	}
	pkt.ScalePTS = pkt.ScaleDTS = pkt.ScaleDuration = 0.0;
	if (pkt.Id > 1)
		goto loop;

	int timescale = (pkt.Id == 0 ? _video_ts : _audio_ts);
	int64_t start_offset_no_scale = (pkt.Id == 0 ? _start_time_offset_video_no_scale : _start_time_offset_audio_no_scale);
	if (pkt.PTS != InvalidTimestamp())
		pkt.ScalePTS = double(pkt.PTS) / double(timescale) -
		(pkt.PTS >= start_offset_no_scale ? _start_time_offset : 0.0);
	if (pkt.DTS != InvalidTimestamp())
		pkt.ScaleDTS = double(pkt.DTS) / double(timescale) -
		(pkt.DTS >= start_offset_no_scale ? _start_time_offset : 0.0);
	if (pkt.Duration > 0 && pkt.Duration != InvalidTimestamp())
		pkt.ScaleDuration = double(pkt.Duration) / double(timescale);

	double time_off = (pkt.Id == 0 ? _video_time_offset : _audio_time_offset);
	pkt.ScalePTS += time_off; //加上上N个分段的时间偏移
	pkt.ScaleDTS += time_off;
	int64_t timestamp;
	if (pkt.Id == 0) {
		timestamp = (pkt.DTS == InvalidTimestamp() ? pkt.PTS : pkt.DTS); //DTS优先
		if (pkt.Duration != InvalidTimestamp() && pkt.Duration > 0) {
			_prev_video_duration = (int)pkt.Duration;
		}else{
			auto temp = timestamp - _prev_video_time;
			if (temp > 1)
				_prev_video_duration = (int)temp;
		}
		_prev_video_time = timestamp;
	}else{
		timestamp = pkt.DTS;
		if (pkt.Duration != InvalidTimestamp() && pkt.Duration > 0) {
			_prev_audio_duration = (int)pkt.Duration;
		}else{
			auto temp = timestamp - _prev_audio_time;
			if (temp > 1)
				_prev_audio_duration = (int)temp;
		}
		_prev_audio_time = timestamp;;
	}

	if (pkt.Id == 1)
		pkt.KeyFrame = false;
	if (!DoProcessPacketOnce(&pkt)) //MKV复用器
		return false;

	if (cur_timestamp)
		*cur_timestamp = double(pkt.DTS) / double(timescale);
	return (int)(_output->Tell() - offset);
}

bool MergeManager::InternalNewDemux(double* duration)
{
	if (_demux)
		delete _demux;
	_demux = NULL;

	DemuxCore* core = new(std::nothrow) DemuxCore(_input);
	if (core == NULL)
		return false;
	
	if (!InternalInitDemux(core)) {
		delete core;
		return false;
	}
	if (duration)
		*duration = core->GetDuration();

	_demux = core;
	return true;
}

bool MergeManager::InternalInitDemux(void* demux)
{
	DemuxCore* core = (DemuxCore*)demux;
	if (!core->OpenByteStream())
		return false;
	if (core->GetTrackCount() < 2)
		return false;

	DemuxProxy::Track t1 = {}, t2 = {};
	core->GetTrack(0, &t1);
	core->GetTrack(1, &t2);
	if (t1.Type != DemuxProxy::Track::TrackType::Video ||
		t2.Type != DemuxProxy::Track::TrackType::Audio)
		return false;

	_audio_ts = t2.Timescale;
	_video_ts = t1.Timescale;

	_start_time_offset = core->GetStartTime();
	_start_time_offset_audio_no_scale = (int64_t)(_start_time_offset * _audio_ts);
	_start_time_offset_video_no_scale = (int64_t)(_start_time_offset * _video_ts);
	return true;
}