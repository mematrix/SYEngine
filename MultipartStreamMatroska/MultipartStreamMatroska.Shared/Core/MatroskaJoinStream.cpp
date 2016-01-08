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
	if (!tr.ParseFile(list_file)) //���������ļ�
		return false;
	if (tr.GetTextLine()->Count < 2) //����Ҫ��һ���ֶ�= =
		return false;

	PrepareConfigs(tr); //��������
	if (_cfgs.NetworkReconnect)
		_io_stream.SetAutoReconnect();
	if (!_cfgs.LocalFileTestMode) {
		if (!PrepareItems(tr)) //��������list
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

	if (!PrepareDownloader()) //��ʼ�����������list
		return false;

	_duration_temp = 0.0;
	if (_cfgs.LoadFullItems && _item_count > 1)
		if (!ProcessItemList()) //��������list��Ŀ
			return false;

	if (!OnPartInit())
		return false;

	if (!ProcessFirstItem() ||
		!ProcessFirstPacket()) //��ʼ������
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

	unsigned read_size = ReadInBuffered(buf, size); //ȡ��һ��ʣ�������
	unsigned pending_size = size - read_size; //��ȥ��һ�����ݵĴ�С
	if (pending_size == 0) //��һ��ʣ��������Ѿ����㱾�ε�����
		return read_size;

	//��һ��ʣ��������޷����㱾�ε�����
	if (_merger == NULL)
		return 0;
	if (!ProcessReadToSize(pending_size)) //������������
		return 0;
	read_size += ReadInBuffered((char*)buf + read_size, pending_size); //��ȫ

	_cur_part_read_size += read_size; //�ܴ�С
	return read_size;
}

bool MatroskaJoinStream::MatroskaTimeSeek(double seconds)
{
	printf("MatroskaJoinStream::MatroskaTimeSeek %.2f\n", float(seconds));
	//���Ȳ���һ��Ҫseek����time���ĸ��ֶ�
	//����Ҫ�Ѿ�֪��Ҫseek���ķֶ�·�������зֶε�duration��Ϣ
	int new_index = FindItemIndexByTime(seconds);
	if (seconds < 0.1)
		new_index = 0;
	if (new_index == -1) {
		//���û���ҵ�����ķֶΣ���ȫ��ȥloadһ�飬��ȡsize��duration��Ϣ
		if (!LoadBatchItemInfoToTime(seconds))
			return false;
		new_index = FindItemIndexByTime(seconds); //�ٴβ���
		if (new_index == -1)
			return false;
	}
	if (_merger == NULL)
		return false;

	//ȡ�ñ�seek�ķֶ�ǰ������зֶ�ʱ����ܺͣ���������ʱ���offset
	double prev_part_total_time = TimeCalcTotalItems(new_index);
	//��������Ҫ������ֶ�seek��ʱ��
	double seek_time = seconds - prev_part_total_time;
	if (_merger->GetDuration() - seconds < 1.0)
		seek_time -= 0.5;

	bool result = false;
	if (new_index == _index) {
		//���Ҫseek�ķֶξ��ǵ�ǰ���ڲ��ŵķֶΣ�ֱ��seek����
		result = _merger->GetDemuxObject()->TimeSeek(seek_time);
	}else{
		//��ȻҪȥ���������ķֶ�
		if (RunDownload(new_index)) {
			_tasks->GetDataSource(new_index)->SetPosition(0); //��ԭpos
			_io_stream.SwapDs(_tasks->GetDataSource(new_index));
			if (_merger->PutNewInput(&_io_stream)) //�����µķֶ����� (����IOһ��)
				if (_merger->GetDemuxObject() != NULL)
					result = _merger->GetDemuxObject()->TimeSeek(seek_time); //��seek (����IO����)
		}
	}
	if (!result)
		printf("MatroskaJoinStream::MatroskaTimeSeek FAILED!\n");

	//����ʱ���offset
	_merger->SetTimeOffset(prev_part_total_time, prev_part_total_time);
	_buffer.SetLength(0); //���seekǰ����
	_index = new_index; //�滻��ǰ����ķֶ�

	//���µ�ǰpart��size��Ϣ
	Item* item = GetItem(_index);
	double per = seek_time / (item->Duration > 0.1 ? item->Duration : 1.0);
	_cur_part_read_size = (unsigned)((double)item->Size * per);
	return result;
}

////////////////////// Private Functions //////////////////////

unsigned MatroskaJoinStream::ReadInBuffered(void* buf, unsigned size)
{
	unsigned plen = _buffer.GetPendingLength(); //ȡ�û�ʣ���������
	if (plen == 0 || _buffer.GetLength() == 0)
		return 0;
	unsigned allow_size = size > plen ? plen : size;
	_buffer.Read(buf, allow_size); //copy����
	if (_buffer.IsEof())
		_buffer.SetLength(0); //���EOF�����λ�����
	return allow_size;
}

bool MatroskaJoinStream::ProcessReadToSize(unsigned req_size)
{
	bool close_cluster = false; //ָʾ�Ƿ�Ҫ�ر�cluster������frame��
	while (1)
	{
		if (_buffer.GetLength() >= req_size) //�ж��Ƿ���������
			break;
		int bytes = _merger->ProcessPacketOnce(&_cur_timestamp); //����һ��frame
		if (bytes == 0) { //��һ����Ƭ��������
			bool result = OpenNextItem(); //���Դ���һ��
			if (!result && _index < _item_count) {
				if (OnPartError(_index)) {
					_index--;
					result = OpenNextItem(true);
				}
			}
			if (!result) {
				//�������һ��ʧ�ܣ�û����һ���ˣ���д�ļ�β��
				close_cluster = false; //��ProcessComplete�Ѿ�close��
				if (_merger->ProcessComplete()) //������Ժϲ�
					OnPartEnded(); //֪ͨ����
				break;
			}else{
				//�򿪳ɹ�������item
				if (_merger->GetDemuxObject() == NULL)
					break; //WTF...
				Item* item = GetItem(_index);
				item->Duration = _merger->GetDemuxObject()->GetDuration();
				OnPartNext(_index, _item_count); //֪ͨ��һ��
			}
		}
		close_cluster = true; //ֻҪ���й�һ��ProcessPacketOnce
	}
	if (close_cluster) {
		AkaMatroska::Matroska* m = (AkaMatroska::Matroska*)_merger->GetWorker();
		m->ForceCloseCurrentCluster(); //�رյ�ǰ��Cluster
	}
	_buffer.SetOffset(0); //���λ�����
	return _buffer.GetLength() > 0;
}

bool MatroskaJoinStream::OpenNextItem(bool try_again)
{
	_cur_part_read_size = 0;
	++_index;
	if (_index == _item_count)
		return false;

	Item* next = GetItem(_index); //ȡ����һ��item
	if (try_again)
		DownloadItemStop(_index);
	if (!RunDownload(_index)) //��ʼ����
		return false;

	//ȡ�����غ��stream
	if (_tasks->GetDataSource(_index) == NULL)
		return false;
	_tasks->GetDataSource(_index)->SetPosition(0);
	_io_stream.SwapDs(_tasks->GetDataSource(_index));

	next->Size = (unsigned)_io_stream.Size(); //���·ֶδ�С��Ϣ
	return _merger->PutNewInput(&_io_stream, try_again); //��ʼ������ֶ�
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
			break; //�ҵ�
		}
		if (item->Duration == 0.0 &&
			item->Size == 0)
			break; //û���ҵ�����Ҫ���ص������Ƭ
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
	if (!_items.Alloc(count * sizeof(Item), true)) //��������item����
		return false;

	Item* item = _items.Get<Item*>();
	unsigned index = start_index;
	for (unsigned i = 0; i < count; i++)
	{
		char* path = tr.GetTextLine()->Line[index + 1];
		if (path == NULL || strlen(path) < 2)
			return false;
		path = strdup(path);
		if (*path == '"') //ȥ��˫����
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
		Item* item = GetItem(i); //��ÿ��item����ӵ�list������Ӳ����أ�
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
	MatroskaMerge* merger = new(std::nothrow) MatroskaMerge(true); //�µ����Ժϲ���
	if (merger == NULL)
		return false;
	if (!merger->PutNewOutput(this)) { //�����IO
		delete merger;
		return false;
	}
	if (_cfgs.LocalFileTestMode || _cfgs.DefaultUsePartDuration)
		merger->SetForceUseDuration();

	_index = 0; //����Ϊ��һ���ֶ�����
	while (1)
	{
		if (!RunDownload(0)) //��ʼ���ص�һ���ֶ�
			break;
		
		double duration = double(_cfgs.DurationMs) / 1000.0;
		if (_duration_temp > 0.5)
			duration = _duration_temp - 0.1;

		_tasks->GetDataSource(0)->SetPosition(0);
		_io_stream.SwapDs(_tasks->GetDataSource(0));
		if (!merger->PutNewInput(&_io_stream)) //��һ���ֶε�IO
			break;
		if (!merger->ProcessHeader(duration, _item_count == 1 ? true:false)) //����MKVͷ
			break;

		//���µ�һ���ֶε�item��Ϣ
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
	_buffer.SetLength(0); //ͷ�������Ѿ�copy��ȫ������
	_cur_part_read_size = 0; //�ص���ǰ�ֶε��ܴ�С����

	if (result)
		OnPartStart(); //֪ͨ��ʼ
	return result;
}

bool MatroskaJoinStream::ProcessFirstPacket()
{
	if (_merger->ProcessPacketOnce() == 0) //����һ��packet
		return false;
	((AkaMatroska::Matroska*)_merger->GetWorker())->ForceCloseCurrentCluster();
	_buffer.SetOffset(0); //���λ�����
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
	//����������
	if (!RunDownload(index))
		return false;

	if (_tasks->GetDataSource(index) == NULL)
		return false;
	//ȡ���ֽ���
	DsIOCallback stream;
	stream.SwapDs(_tasks->GetDataSource(index));

	//����demux���
	MergeManager::DemuxCore* demux = new(std::nothrow) MergeManager::DemuxCore(&stream);
	if (demux == NULL)
		return false;

	bool result = false;
	while (1)
	{
		if (!demux->OpenByteStream()) //open����demux
			break;

		Item* item = GetItem(index);
		item->Size = (unsigned)stream.Size(); //���ô�С
		item->Duration = demux->GetDuration(); //����ʱ��
		result = true;
		break;
	}
	delete demux; //ɾ��demux
	//��ͣ����ֹͣ������
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