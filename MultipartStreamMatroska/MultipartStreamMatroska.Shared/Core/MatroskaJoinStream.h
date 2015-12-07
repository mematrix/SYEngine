#pragma once

#include "TextReader.h"
#include "MemoryStream.h"
#include "MatroskaMerge.h"
#include "DsIOCallback.h"
#include "downloader/DownloadCore.h"
#include "downloader/DownloadTask.h"

class MatroskaJoinStream : private MergeManager::IOCallback
{
public:
	MatroskaJoinStream() throw() : _index(0), _merger(NULL), _tasks(NULL), _item_count(0)
	{ memset(&_cfgs, 0, sizeof(_cfgs)); }
	virtual ~MatroskaJoinStream() throw() { DeInit(); }

	bool Init(const wchar_t* list_file);
	bool ReInit(bool preload_first_pkt = false);
	void DeInit() { FreeItems(); FreeResources(); }

	inline MemoryBuffer* MatroskaHeadBytes() { return &_header; }
	inline MemoryStream* MatroskaInternalBuffer() { return &_buffer; }

	unsigned MatroskaReadFile(void* buf, unsigned size, unsigned skip_pkt_count = 0);
	bool MatroskaTimeSeek(double seconds);

private:
	//Init
	void PrepareConfigs(TextReader& tr);
	bool PrepareItems(TextReader& tr);
	bool PrepareLocalItems(TextReader& tr);
	bool PrepareDownloader();

	bool ProcessFirstItem();
	bool ProcessFirstPacket();
	bool ProcessItemList();

	//Free
	void FreeItems();
	void FreeResources();

	//ReadFile
	unsigned ReadInBuffered(void* buf, unsigned size);
	bool ProcessReadToSize(unsigned req_size);
	bool OpenNextItem();

	//SeekTime
	int FindItemIndexByTime(double seconds); //返回-1没找到
	bool LoadBatchItemInfoToTime(double seconds);
	double TimeCalcTotalItems(unsigned count);

	//Network
	bool RunDownload(int index);
	bool LoadItemInfo(int index);

	//MatroskaMerge
	virtual int Read(void* buf, unsigned size) { return 0; }
	virtual int Write(const void* buf, unsigned size)
	{ return (int)_buffer.Write(buf, size); }
	virtual bool Seek(int64_t offset, int whence)
	{ return _buffer.SetOffset((unsigned)offset); }
	virtual int64_t Tell()
	{ return _buffer.GetOffset(); }

protected:
	struct Config
	{
		unsigned DurationMs; //总时间
		bool LoadFullItems; //是否在Init时候就载入所有分段的信息
		bool LocalFileTestMode; //本地文件测试模式
		int PreloadNextPartRemainSeconds; //在上一个part播放到剩下多少秒的时候，载入下一个part
		bool NetworkReconnect; //是否在下载中断的时候重新连接
		int NetworkBufBlockSizeKB, NetworkBufBlockCount;
		char NetworkTempPath[512];
		struct HttpProfile
		{
			char Cookie[1024];
			char RefUrl[2048];
			char UserAgent[512];
		};
		HttpProfile Http;
	};
	struct Item //单个分段的信息
	{
		char* Url;
		unsigned Size;
		double Duration;
		unsigned Bitrate;
	};

private:
	unsigned _index; //当前处理哪个分段的索引
	unsigned _cur_part_read_size; //当前处理的这个分段读取了多少的数据
	MemoryStream _buffer; //ProcessPacket的缓冲区
	DsIOCallback _io_stream; //每个分段的IO流

	Downloader::Core::TaskManager* _tasks; //下载task管理器
	MatroskaMerge* _merger; //核心处理器
	MemoryBuffer _header; //保存MKV的头

	Config _cfgs; //设置信息
	MemoryBuffer _items; //item列表
	unsigned _item_count; //item总数

	double _cur_timestamp;
	double _duration_temp;

protected:
	inline unsigned GetIndex() const throw()
	{ return _index; }
	inline Config* GetConfigs() throw()
	{ return &_cfgs; }
	inline void* GetDownloadManager() throw()
	{ return _tasks; }

	inline Item* GetItem(unsigned index)
	{ return _items.Get<Item*>() + index; }
	inline Item* GetItems()
	{ return _items.Get<Item*>(); }
	inline unsigned GetItemCount() const throw()
	{ return _item_count; }

	inline unsigned GetCurrentPartTotalSize() throw()
	{ if (_index >= _item_count) return 0; return GetItem(_index)->Size; }
	inline unsigned GetCurrentPartReadSize() const throw()
	{ return _cur_part_read_size; }

	inline double GetCurrentTotalTime() throw()
	{ if (_index >= _item_count) return double(INT64_MAX); return GetItem(_index)->Duration; }
	inline double GetCurrentTimestamp() const throw()
	{ return _cur_timestamp; }

	void DownloadNextItem() throw()
	{ DownloadItemStart(_index + 1); }
	bool DownloadItemStart(int index) throw()
	{ return RunDownload(index); }
	bool DownloadItemStop(int index) throw()
	{ if (_tasks == NULL) return false;
	  return _tasks->StopTask(index) == Downloader::Core::CommonResult::kSuccess; }
	bool DownloadItemPause(int index) throw()
	{ if (_tasks == NULL) return false;
	  return _tasks->PauseTask(index) == Downloader::Core::CommonResult::kSuccess; }
	bool DownloadItemResume(int index) throw()
	{ if (_tasks == NULL) return false;
	  return _tasks->ResumeTask(index) == Downloader::Core::CommonResult::kSuccess; }

	virtual void OnPartStart()
	{
#ifdef _DEBUG
		printf("OnPartStart...\n");
#endif
		OnPartNext(0, _item_count);
	}
	virtual void OnPartNext(unsigned index, unsigned total)
	{
#ifdef _DEBUG
		printf("OnPartNext: %d / %d\n", index + 1, total);
#endif
	}
	virtual void OnPartEnded()
	{
#ifdef _DEBUG
		printf("OnPartEnded.\n");
#endif
	}
};