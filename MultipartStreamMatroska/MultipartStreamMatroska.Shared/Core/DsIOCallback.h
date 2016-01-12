#ifndef __DS_IOCALLBACK_H
#define __DS_IOCALLBACK_H

#include "MergeManager.h"
#include "downloader/DownloadCore.h"
#include <assert.h>
#include <functional>

class DsIOCallback : public MergeManager::IOCallback
{
	Downloader::Core::DataSource* _ds;
	int64_t _cur_pos;

	bool _reconnect; //http连接中断，尝试从新连接
	std::function<void (int64_t,int64_t)>* _reconn_notify;

public:
	DsIOCallback() throw() : _ds(NULL), _cur_pos(0), _reconnect(false), _reconn_notify(NULL) {}
	explicit DsIOCallback(Downloader::Core::DataSource* ds) throw() : _ds(ds), _cur_pos(0) {}
	virtual ~DsIOCallback() throw() {}

	inline Downloader::Core::DataSource* GetCurrentDs() throw()
	{ return _ds; }
	inline void SwapDs(Downloader::Core::DataSource* ds) throw()
	{ _ds = ds; _cur_pos = 0; }
	inline void ResetReadIo() throw()
	{ Seek(0, SEEK_SET); }

	inline void SetAutoReconnect(bool state = true) throw()
	{ _reconnect = state; }
	inline void SetReconnectNotify(std::function<void (int64_t,int64_t)>* callback) throw()
	{ _reconn_notify = callback; }

	virtual void* GetPrivate()
	{ return _ds; }

	virtual int Read(void* buf, unsigned size)
	{
		unsigned rs = 0;
		auto r = _ds->ReadBytes(buf, size, &rs);
		if (r != Downloader::Core::CommonResult::kSuccess)
			return 0;

		if (rs == 0 && _reconnect) {
			//可能需要重新连接
			int64_t total_size = 0;
			_ds->GetLength(&total_size);
			if (total_size > 1 && total_size != INT64_MAX &&
				(_ds->GetFlags() & Downloader::Core::DataSource::Flags::kFromRemoteLink) != 0) {
				if (_cur_pos < (total_size - 1)) {
					//reconnect
					if (_reconn_notify != NULL)
						_reconn_notify->operator()(_cur_pos, total_size);
					if (_ds->ReconnectAtOffset(_cur_pos) == Downloader::Core::CommonResult::kSuccess) {
						_ds->ReadBytes(buf, size, &rs);
						if (r != Downloader::Core::CommonResult::kSuccess)
							return 0;
					}
				}
			}
		}
		_cur_pos += rs;
		return rs;
	}
	virtual int Write(const void* buf, unsigned size) { return 0; }

	virtual bool Seek(int64_t offset, int whence)
	{
		assert(whence != SEEK_END);
		if (whence == SEEK_END)
			return false;
		int64_t o = offset;
		if (whence == SEEK_CUR)
			o += _cur_pos;
		if (_ds->SetPosition(o) != Downloader::Core::CommonResult::kSuccess)
			return false;
		_cur_pos = o;
		return true;
	}
	virtual int64_t Tell() { return _cur_pos; }

	virtual int64_t Size()
	{
		int64_t s = 0;
		_ds->GetLength(&s);
		return s;
	}
};

#endif //__DS_IOCALLBACK_H