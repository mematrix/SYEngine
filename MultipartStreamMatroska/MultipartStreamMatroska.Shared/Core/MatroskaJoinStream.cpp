#include "MatroskaJoinStream.h"
#ifdef _MSC_VER
#include "WindowsLocalFile.h"
#include "WindowsTaskManager.h"
#include "WindowsHttpTaskManager.h"
#else
#error "Visual C++ for Win32 Only."
#endif

#define DEFAULT_DOWNLOAD_TIMEOUT 35

bool MatroskaJoinStream::Init(const wchar_t* list_file)
{
	printf("MatroskaJoinStream::Init\n");
	if (list_file == NULL)
		return false;
	if (wcslen(list_file) < 2)
		return false;
	if (_item_count > 0)
		return false;

	TextReader tr;
	if (!tr.ParseFile(list_file)) //分析设置文件
		return false;
	if (tr.GetTextLine()->Count < 2) //至少要有一个分段= =
		return false;

	PrepareConfigs(tr); //载入设置
	if (_cfgs.NetworkReconnect) {
		_io_stream.SetAutoReconnect();
		_io_stream.SetReconnectNotify(&_dsio_reconn_callback);
	}
	if (!_cfgs.LocalFileTestMode) {
		if (!PrepareItems(tr)) //载入所有list
			return false;
		if (!_cfgs.LoadFullItems && _cfgs.DurationMs == 0) {
			for (unsigned i = 0; i < _item_count; i++)
				_cfgs.DurationMs += unsigned(GetItem(i)->Duration * 1000.0);
		}
	}else{
		_cfgs.DurationMs = 0;
		_cfgs.LoadFullItems = true;
		if (!PrepareLocalItems(tr))
			return false;
	}

	if (!PrepareDownloader()) //初始化下载器添加list
		return false;

	_duration_temp = 0.0;
	if (_cfgs.LoadFullItems && _item_count > 1)
		if (!ProcessItemList()) //载入所有list项目
			return false;

	if (!OnPartInit())
		return false;

	if (!ProcessFirstItem() ||
		!ProcessFirstPacket()) //初始化容器
		return false;
	return true;
}

bool MatroskaJoinStream::ReInit(bool preload_first_pkt)
{
	printf("MatroskaJoinStream::ReInit\n");
	if (_merger) {
		_merger->ProcessComplete();
		delete _merger;
	}
	_header.Free();
	_buffer.SetOffset(0);

	for (unsigned i = 0; i < _item_count; i++) {
		if (_tasks->GetDataSource(i) != NULL)
			_tasks->StopTask(i);
	}

	if (!ProcessFirstItem())
		return false;
	if (preload_first_pkt)
		if (!ProcessFirstPacket())
			return false;
	return true;
}

unsigned MatroskaJoinStream::MatroskaReadFile(void* buf, unsigned size, unsigned skip_pkt_count)
{
	if (skip_pkt_count > 0)
	{
		for (unsigned i = 0; i < skip_pkt_count; i++)
			_merger->ProcessSkipPacket();
		return 0;
	}

	if (buf == NULL || size == 0)
		return 0;

	unsigned read_size = ReadInBuffered(buf, size); //取上一次剩余的数据
	unsigned pending_size = size - read_size; //减去上一次数据的大小
	if (pending_size == 0) //上一次剩余的数据已经满足本次的需求
		return read_size;

	//上一次剩余的数据无法满足本次的需求
	if (_merger == NULL)
		return 0;
	if (!ProcessReadToSize(pending_size)) //缓冲更多的数据
		return 0;
	read_size += ReadInBuffered((char*)buf + read_size, pending_size); //补全

	_cur_part_read_size += read_size; //总大小
	return read_size;
}

bool MatroskaJoinStream::MatroskaTimeSeek(double seconds)
{
	printf("MatroskaJoinStream::MatroskaTimeSeek %.2f\n", float(seconds));
	//首先查找一下要seek到的time在哪个分段
	//首先要已经知道要seek到的分段路过的所有分段的duration信息
	int new_index = FindItemIndexByTime(seconds);
	if (seconds < 0.1)
		new_index = 0;
	if (new_index == -1) {
		//如果没有找到缓存的分段，就全部去load一遍，获取size和duration信息
		if (!LoadBatchItemInfoToTime(seconds))
			return false;
		new_index = FindItemIndexByTime(seconds); //再次查找
		if (new_index == -1)
			return false;
	}
	if (_merger == NULL)
		return false;

	//取得被seek的分段前面的所有分段时间的总和，用于设置时间戳offset
	double prev_part_total_time = TimeCalcTotalItems(new_index);
	//计算真正要对这个分段seek的时间
	double seek_time = seconds - prev_part_total_time;
	if (_merger->GetDuration() - seconds < 1.0)
		seek_time -= 0.5;

	bool result = false;
	if (new_index == _index) {
		//如果要seek的分段就是当前正在播放的分段，直接seek即可
		result = _merger->GetDemuxObject()->TimeSeek(seek_time);
	}else{
		//不然要去载入其他的分段
		if (RunDownload(new_index)) {
			_tasks->GetDataSource(new_index)->SetPosition(0); //复原pos
			_io_stream.SwapDs(_tasks->GetDataSource(new_index));
			if (_merger->PutNewInput(&_io_stream)) //载入新的分段输入 (网络IO一次)
				if (_merger->GetDemuxObject() != NULL)
					result = _merger->GetDemuxObject()->TimeSeek(seek_time); //做seek (网络IO两次)
		}
	}
	if (!result)
		printf("MatroskaJoinStream::MatroskaTimeSeek FAILED!\n");

	//设置时间戳offset
	_merger->SetTimeOffset(prev_part_total_time, prev_part_total_time);
	_buffer.SetLength(0); //清空seek前缓存
	_index = new_index; //替换当前处理的分段

	//更新当前part的size信息
	Item* item = GetItem(_index);
	double per = seek_time / (item->Duration > 0.1 ? item->Duration : 1.0);
	_cur_part_read_size = (unsigned)((double)item->Size * per);
	return result;
}

////////////////////// Private Functions //////////////////////

unsigned MatroskaJoinStream::ReadInBuffered(void* buf, unsigned size)
{
	unsigned plen = _buffer.GetPendingLength(); //取得还剩余多少数据
	if (plen == 0 || _buffer.GetLength() == 0)
		return 0;
	unsigned allow_size = size > plen ? plen : size;
	_buffer.Read(buf, allow_size); //copy数据
	if (_buffer.IsEof())
		_buffer.SetLength(0); //如果EOF，环形缓冲区
	return allow_size;
}

bool MatroskaJoinStream::ProcessReadToSize(unsigned req_size)
{
	bool close_cluster = false; //指示是否要关闭cluster（生成frame）
	while (1)
	{
		if (_buffer.GetLength() >= req_size) //判断是否满足需求
			break;
		int bytes = _merger->ProcessPacketOnce(&_cur_timestamp); //处理一个frame
		if (bytes == 0) { //上一个切片处理玩了
			bool result = OpenNextItem(); //尝试打开下一个
			if (!result && _index < _item_count) {
				if (OnPartError(_index)) {
					_index--;
					result = OpenNextItem(true);
				}
			}
			if (!result) {
				//如果打开下一个失败（没有下一个了），写文件尾巴
				close_cluster = false; //在ProcessComplete已经close了
				if (_merger->ProcessComplete()) //完成流性合并
					OnPartEnded(); //通知结束
				break;
			}else{
				//打开成功，更新item
				if (_merger->GetDemuxObject() == NULL)
					break; //WTF...
				Item* item = GetItem(_index);
				item->Duration = _merger->GetDemuxObject()->GetDuration();
				OnPartNext(_index, _item_count); //通知下一个
			}
		}
		close_cluster = true; //只要进行过一次ProcessPacketOnce
	}
	if (close_cluster) {
		AkaMatroska::Matroska* m = (AkaMatroska::Matroska*)_merger->GetWorker();
		m->ForceCloseCurrentCluster(); //关闭当前的Cluster
	}
	_buffer.SetOffset(0); //环形缓冲区
	return _buffer.GetLength() > 0;
}

bool MatroskaJoinStream::OpenNextItem(bool try_again)
{
	_cur_part_read_size = 0;
	++_index;
	if (_index == _item_count)
		return false;

	Item* next = GetItem(_index); //取得下一个item
	if (try_again)
		DownloadItemStop(_index);
	if (!RunDownload(_index)) //开始下载
		return false;

	//取得下载后的stream
	if (_tasks->GetDataSource(_index) == NULL)
		return false;
	_tasks->GetDataSource(_index)->SetPosition(0);
	_io_stream.SwapDs(_tasks->GetDataSource(_index));

	next->Size = (unsigned)_io_stream.Size(); //更新分段大小信息
	return _merger->PutNewInput(&_io_stream, try_again); //初始化这个分段
}

int MatroskaJoinStream::FindItemIndexByTime(double seconds)
{
	double time = 0.0;
	int ret = -1;
	for (unsigned i = 0; i < _item_count; i++)
	{
		Item* item = GetItem(i);
		if (item == NULL)
			break;
		time += item->Duration;
		if (time > seconds) {
			ret = (int)i;
			break; //找到
		}
		if (item->Duration == 0.0 &&
			item->Size == 0)
			break; //没有找到，需要加载到这个分片
	}
	return ret;
}

bool MatroskaJoinStream::LoadBatchItemInfoToTime(double seconds)
{
	double time = 0.0;
	for (unsigned i = 0; i < _item_count; i++)
	{
		Item* item = GetItem(i);
		if (item == NULL)
			return false;

		if (item->Duration == 0.0) {
			if (!LoadItemInfo(i))
				return false;
		}
		time += item->Duration;
		if (time > seconds)
			return true;
	}
	return true;
}

double MatroskaJoinStream::TimeCalcTotalItems(unsigned count)
{
	double time = 0.0;
	for (unsigned i = 0; i < count; i++)
		time += GetItem(i)->Duration;
	return time;
}

void MatroskaJoinStream::PrepareConfigs(TextReader& tr)
{
	_cfgs.DurationMs = atoi(tr.GetTextLine()->Line[0]);
	_cfgs.LoadFullItems = (stricmp(tr.GetTextLine()->Line[1], "FULL") == 0);
	_cfgs.LocalFileTestMode = (stricmp(tr.GetTextLine()->Line[0], "LOCAL") == 0);
	if (!_cfgs.LocalFileTestMode) {
		_cfgs.DefaultUsePartDuration = (stricmp(tr.GetTextLine()->Line[2], "NextUseDuration") == 0);
		_cfgs.PreloadNextPartRemainSeconds = atoi(tr.GetTextLine()->Line[3]);
		_cfgs.NetworkReconnect = (stricmp(tr.GetTextLine()->Line[4], "Reconnect") == 0);
		if (strstr(tr.GetTextLine()->Line[5], "|") != NULL) {
			_cfgs.NetworkBufBlockSizeKB = atoi(tr.GetTextLine()->Line[5]);
			_cfgs.NetworkBufBlockCount = atoi(strstr(tr.GetTextLine()->Line[5], "|") + 1);
		}
		if (!_cfgs.LocalFileTestMode) {
			strcpy(_cfgs.NetworkTempPath, tr.GetTextLine()->Line[6]);
			strcpy(_cfgs.Http.Cookie, tr.GetTextLine()->Line[7]);
			strcpy(_cfgs.Http.RefUrl, tr.GetTextLine()->Line[8]);
			strcpy(_cfgs.Http.UserAgent, tr.GetTextLine()->Line[9]);
			if (strlen(tr.GetTextLine()->Line[10]) > 0)
				_cfgs.UniqueId = strdup(tr.GetTextLine()->Line[10]);
			if (strlen(tr.GetTextLine()->Line[11]) > 0)
				_cfgs.DebugInfo = strdup(tr.GetTextLine()->Line[11]);
		}
	}
}

bool MatroskaJoinStream::PrepareItems(TextReader& tr)
{
	static unsigned start_index = 12;
	unsigned count = (tr.GetTextLine()->Count - start_index) / 2;
	if (count == 0)
		return false;
	if (!_items.Alloc(count * sizeof(Item), true)) //申请所有item缓冲
		return false;

	Item* item = _items.Get<Item*>();
	unsigned index = start_index;
	for (unsigned i = 0; i < count; i++)
	{
		char* path = tr.GetTextLine()->Line[index + 1];
		if (path == NULL || strlen(path) < 2)
			return false;
		path = strdup(path);
		if (*path == '"') //去掉双引号
			*(path + strlen(path) - 1) = 0;
		item->Url = strdup(*path == '"' ? path + 1 : path);

		//format: size|time
		char* size_time = tr.GetTextLine()->Line[index];
		item->Size = atoi(size_time);
		if (strstr(size_time, "|") != NULL) {
			unsigned ms = atoi(strstr(size_time, "|") + 1);
			item->Duration = (double)ms / 1000.0;
		}

		++item;
		index += 2;
		free(path);
	}
	_item_count = count;
	return true;
}

bool MatroskaJoinStream::PrepareLocalItems(TextReader& tr)
{
	if (tr.GetTextLine()->Count < 2)
		return false;

	unsigned count = tr.GetTextLine()->Count - 1;
	if (!_items.Alloc(count * sizeof(Item), true))
		return false;

	Item* item = _items.Get<Item*>();
	for (unsigned i = 0; i < count; i++)
	{
		char* path = tr.GetTextLine()->Line[i + 1];
		if (path == NULL || strlen(path) < 2)
			return false;
		path = strdup(path);
		if (*path == '"')
			*(path + strlen(path) - 1) = 0;
		item->Url = strdup(*path == '"' ? path + 1 : path);
		++item;
		free(path);
	}
	_item_count = count;
	return true;
}

bool MatroskaJoinStream::PrepareDownloader()
{
	if (_cfgs.LocalFileTestMode) {
		_tasks = new(std::nothrow) Downloader::Windows::WindowsTaskManager();
	}else{
		if (_cfgs.NetworkBufBlockSizeKB == 0 || _cfgs.NetworkBufBlockCount == 0)
			_tasks = new(std::nothrow) Downloader::Windows::WindowsHttpTaskManager();
		else
			_tasks = new(std::nothrow) Downloader::Windows::WindowsHttpTaskManager(_cfgs.NetworkBufBlockSizeKB, _cfgs.NetworkBufBlockCount);
	}
	if (_tasks == NULL)
		return false;

	Downloader::Windows::WindowsLocalFile* local_file = NULL;
	if (_cfgs.NetworkTempPath[0] != 0 && strcmpi(_cfgs.NetworkTempPath, "NULL") != 0)
		local_file = new(std::nothrow) Downloader::Windows::WindowsLocalFile(_cfgs.NetworkTempPath);
	if (_tasks->Initialize(local_file, NULL) != Downloader::Core::CommonResult::kSuccess)
		return false;

	for (unsigned i = 0; i < _item_count; i++)
	{
		Item* item = GetItem(i); //把每个item都添加到list（仅添加不下载）
		if (item->Url == NULL)
			goto task_err;

		Downloader::Core::Task task;
		task.Timeout = DEFAULT_DOWNLOAD_TIMEOUT;
		task.SetUrl(item->Url);
		task.SetOthers(_cfgs.Http.RefUrl[0] != 0 ? _cfgs.Http.RefUrl : NULL,
			_cfgs.Http.Cookie[0] != 0 ? _cfgs.Http.Cookie : NULL,
			_cfgs.Http.UserAgent[0] != 0 ? _cfgs.Http.UserAgent : NULL);

		Downloader::Core::TaskId tid = 0;
		if (_tasks->AddTask(&task, &tid) != Downloader::Core::CommonResult::kSuccess)
			goto task_err;
		if (tid != i)
			goto task_err;
	}
	return true;
task_err:
	delete _tasks;
	_tasks = NULL;
	return false;
}

bool MatroskaJoinStream::ProcessFirstItem()
{
	bool result = false;
	MatroskaMerge* merger = new(std::nothrow) MatroskaMerge(true); //新的流性合并器
	if (merger == NULL)
		return false;
	if (!merger->PutNewOutput(this)) { //输出的IO
		delete merger;
		return false;
	}
	if (_cfgs.LocalFileTestMode || _cfgs.DefaultUsePartDuration)
		merger->SetForceUseDuration();

	_index = 0; //设置为第一个分段索引
	while (1)
	{
		if (!RunDownload(0)) //开始下载第一个分段
			break;
		
		double duration = double(_cfgs.DurationMs) / 1000.0;
		if (_duration_temp > 0.5)
			duration = _duration_temp - 0.1;

		_tasks->GetDataSource(0)->SetPosition(0);
		_io_stream.SwapDs(_tasks->GetDataSource(0));
		if (!merger->PutNewInput(&_io_stream)) //第一个分段的IO
			break;
		if (!merger->ProcessHeader(duration, _item_count == 1 ? true:false)) //生成MKV头
			break;

		//更新第一个分段的item信息
		Item* t = GetItem(0);
		t->Duration = merger->GetDemuxObject()->GetDuration();
		t->Size = (unsigned)_io_stream.Size();
		result = true;
		break;
	}

	if (!result) {
		delete merger;
		merger = NULL;
	}
	_merger = merger;

	//copy Header.
	if (result) {
		_buffer.SetOffset(0);
		if (_header.Alloc(_buffer.GetLength(), true))
			_buffer.Read(_header.Get<void*>(), _buffer.GetLength(), true);
	}
	_buffer.SetLength(0); //头的数据已经copy，全部忽略
	_cur_part_read_size = 0; //重叠当前分段的总大小计数

	if (result)
		OnPartStart(); //通知开始
	return result;
}

bool MatroskaJoinStream::ProcessFirstPacket()
{
	if (_merger->ProcessPacketOnce() == 0) //处理一个packet
		return false;
	((AkaMatroska::Matroska*)_merger->GetWorker())->ForceCloseCurrentCluster();
	_buffer.SetOffset(0); //环形缓冲区
	return true;
}

bool MatroskaJoinStream::ProcessItemList()
{
	_duration_temp = 0.0;
	for (unsigned i = 0; i < _item_count; i++) {
		if (!LoadItemInfo(i))
			return false;
		_duration_temp += GetItem(i)->Duration;

#if defined(_WIN32) && defined(_DEBUG)
		unsigned duration_ms = (unsigned)(GetItem(i)->Duration * 1000.0);
		printf("Part Item %d - %dms\n", i, duration_ms);
#endif
	}
	return true;
}

bool MatroskaJoinStream::RunDownload(int index)
{
#if defined(_WIN32) && defined(_DEBUG)
	printf("RunDownload - Item:%d, Started:%s, Paused:%s\n",
		index, _tasks->IsStarted(index) ? "Yes" : "No",
		_tasks->IsPaused(index) ? "Yes" : "No");
#endif
	if (index >= (int)_item_count)
		return false;

	if (!_tasks->IsStarted(index))
		if (_tasks->StartTask(index) != Downloader::Core::CommonResult::kSuccess)
			return false;
	if (_tasks->IsPaused(index))
		if (_tasks->ResumeTask(index) != Downloader::Core::CommonResult::kSuccess)
			return false;
	return true;
}

bool MatroskaJoinStream::LoadItemInfo(int index)
{
	//先启动下载
	if (!RunDownload(index))
		return false;

	if (_tasks->GetDataSource(index) == NULL)
		return false;
	//取得字节流
	DsIOCallback stream;
	stream.SwapDs(_tasks->GetDataSource(index));

	//创建demux组件
	MergeManager::DemuxCore* demux = new(std::nothrow) MergeManager::DemuxCore(&stream);
	if (demux == NULL)
		return false;

	bool result = false;
	while (1)
	{
		if (!demux->OpenByteStream()) //open流来demux
			break;

		Item* item = GetItem(index);
		item->Size = (unsigned)stream.Size(); //设置大小
		item->Duration = demux->GetDuration(); //设置时长
		result = true;
		break;
	}
	delete demux; //删除demux
	//暂停或者停止下载流
	_tasks->StopTask(index);
	return result;
}

void MatroskaJoinStream::FreeItems()
{
	if (_item_count > 0) {
		for (unsigned i = 0; i < _item_count; i++) {
			Item* item = GetItem(i);
			if (item)
				if (item->Url)
					free(item->Url);
		}
	}
	_items.Free();
	_item_count = 0;
	_index = 0;
}

void MatroskaJoinStream::FreeResources()
{
	if (_cfgs.UniqueId)
		free(_cfgs.UniqueId);
	_cfgs.UniqueId = NULL;
	if (_cfgs.DebugInfo)
		free(_cfgs.DebugInfo);
	_cfgs.DebugInfo = NULL;

	_header.Free();
	_buffer.SetLength(0);

	if (_merger) {
		delete _merger;
		_merger = NULL;
	}
	if (_tasks) {
		_tasks->Uninitialize();
		delete _tasks;
		_tasks = NULL;
	}
}

void MatroskaJoinStream::UpdateItemInfo(int index, const char* url, const char* req_headers, int timeout, unsigned size, double duration)
{
	if (index >= (int)_item_count)
		return;

	auto item = GetItem(index);
	if (url) {
		free(item->Url);
		item->Url = strdup(url);
	}

	if (size != item->Size && size > 0)
		item->Size = size;
	if (duration != item->Duration && duration > 0.1)
		item->Duration = duration;

	Downloader::Core::Task task;
	task.Timeout = timeout > 1 ? timeout : DEFAULT_DOWNLOAD_TIMEOUT;
	task.SetUrl(item->Url);
	task.SetRequestHeaders(req_headers);
	task.SetOthers(_cfgs.Http.RefUrl[0] != 0 ? _cfgs.Http.RefUrl : NULL,
		_cfgs.Http.Cookie[0] != 0 ? _cfgs.Http.Cookie : NULL,
		_cfgs.Http.UserAgent[0] != 0 ? _cfgs.Http.UserAgent : NULL);

	_tasks->UpdateTask(index, &task);
}