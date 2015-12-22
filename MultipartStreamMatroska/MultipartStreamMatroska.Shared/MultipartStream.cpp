#include "MultipartStream.h"

static const DWORD kPropertyStoreId[] = {
	MFNETSOURCE_BYTESRECEIVED_ID,
	MFNETSOURCE_PROTOCOL_ID,
	MFNETSOURCE_TRANSPORT_ID,
	MFNETSOURCE_CACHE_STATE_ID,
	MFNETSOURCE_DOWNLOADPROGRESS_ID};

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
static bool CheckWin10MatroskaSource(IMFAsyncCallback* cb)
{
	if (cb == NULL)
		return false;
	void* p = *(void**)cb;
	MEMORY_BASIC_INFORMATION mbi = {};
	VirtualQuery(p, &mbi, sizeof(mbi));
	HMODULE hmod = (HMODULE)mbi.AllocationBase;
	WCHAR buf[MAX_PATH] = {};
	GetModuleFileNameW(hmod, buf, MAX_PATH);
	wcslwr(buf);
	return wcsstr(buf, L"mfmkvsrcsnk.dll") != NULL;
}
#endif

HRESULT MultipartStream::QueryInterface(REFIID iid, void** ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	if (iid == IID_IUnknown ||
		iid == IID_IMFByteStream)
		*ppv = static_cast<IMFByteStream*>(this);
	else if (iid == IID_IMFByteStreamTimeSeek)
		*ppv = static_cast<IMFByteStreamTimeSeek*>(this);
	else if (iid == IID_IMFByteStreamBuffering)
		*ppv = static_cast<IMFByteStreamBuffering*>(this);
	else if (iid == IID_IMFByteStreamCacheControl)
		*ppv = static_cast<IMFByteStreamCacheControl*>(this);
	else if (iid == IID_IMFAttributes)
		*ppv = static_cast<IMFAttributes*>(this);
	else if (iid == IID_IMFGetService)
		*ppv = static_cast<IMFGetService*>(this);
	else if (iid == IID_IPropertyStore)
		*ppv = static_cast<IPropertyStore*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT MultipartStream::GetCount(DWORD* cProps)
{
	if (cProps == NULL)
		return E_POINTER;
	*cProps = _countof(kPropertyStoreId);
	return S_OK;
}

HRESULT MultipartStream::GetAt(DWORD iProp, PROPERTYKEY *pkey)
{
	if (pkey == NULL)
		return E_POINTER;
	if (iProp >= _countof(kPropertyStoreId))
		return E_INVALIDARG;

	pkey->fmtid = MFNETSOURCE_STATISTICS;
	pkey->pid = kPropertyStoreId[iProp];
	return S_OK;
}

HRESULT MultipartStream::GetValue(REFPROPERTYKEY key,PROPVARIANT *pv)
{
	if (pv == NULL)
		return E_POINTER;
	if (key.fmtid != MFNETSOURCE_STATISTICS)
		return MF_E_NOT_FOUND;

	if (key.pid != MFNETSOURCE_BYTESRECEIVED_ID &&
		key.pid != MFNETSOURCE_PROTOCOL_ID &&
		key.pid != MFNETSOURCE_TRANSPORT_ID &&
		key.pid != MFNETSOURCE_CACHE_STATE_ID &&
		key.pid != MFNETSOURCE_DOWNLOADPROGRESS_ID)
		return MF_E_NOT_FOUND;

	PropVariantInit(pv);
	pv->vt = VT_I4;
	switch (key.pid)
	{
	case MFNETSOURCE_BYTESRECEIVED_ID:
		pv->vt = VT_UI8;
		pv->uhVal.QuadPart = _stm_length;
		break;
	case MFNETSOURCE_PROTOCOL_ID:
		if (MFGetAttributeUINT32(_attrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 1234)
			pv->lVal = 1; //MFNETSOURCE_HTTP
		else
			pv->lVal = 3; //MFNETSOURCE_FILE
		break;
	case MFNETSOURCE_TRANSPORT_ID:
		pv->lVal = 1; //1 = MFNETSOURCE_TCP, 0 = MFNETSOURCE_UDP
		break;
	case MFNETSOURCE_CACHE_STATE_ID:
		pv->lVal = MFNETSOURCE_CACHE_STATE::MFNETSOURCE_CACHE_UNAVAILABLE;
		break;
	case MFNETSOURCE_DOWNLOADPROGRESS_ID:
		pv->lVal = _download_progress;
		break;
	}
	return S_OK;
}

HRESULT MultipartStream::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (guidService == MFNETSOURCE_STATISTICS_SERVICE)
		return QueryInterface(riid,ppvObject);
	return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT MultipartStream::GetCapabilities(DWORD *pdwCapabilities)
{
	if (pdwCapabilities == NULL)
		return E_POINTER;
	*pdwCapabilities = MFBYTESTREAM_IS_READABLE|MFBYTESTREAM_IS_SEEKABLE|
		MFBYTESTREAM_IS_REMOTE|(_download_progress > 99 ? 0:MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED);
	//可以读、Seek。流来自远程服务器。
	return S_OK;
}

HRESULT MultipartStream::GetLength(QWORD *pqwLength)
{
	if (pqwLength == NULL)
		return E_POINTER;
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	*pqwLength = _stm_length;
	return S_OK;
}

HRESULT MultipartStream::GetCurrentPosition(QWORD *pqwPosition)
{
	if (pqwPosition == NULL)
		return E_POINTER;
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	*pqwPosition = _stm_cur_pos;
	return S_OK;
}

HRESULT MultipartStream::IsEndOfStream(BOOL *pfEndOfStream)
{
	if (pfEndOfStream == NULL)
		return E_POINTER;
	*pfEndOfStream = _flag_eof ? TRUE:FALSE;
	return S_OK;
}

HRESULT MultipartStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
	//分2次处理：
	//1、如果Read开始时的当前位置落在Header+第一个Cluster(Frame)的大小内，就提供header的数据。
	//2、如果超过上面的大小，提供MatroskaReadFile的数据。
	if (pb == NULL || cb == 0)
		return S_OK;
	if (pcbRead != NULL)
		*pcbRead = 0;
	unsigned size = ReadFile(pb, cb); //同步读
	if (pcbRead)
		*pcbRead = size;
	return S_OK;
}

HRESULT MultipartStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
	if (pb == NULL ||
		cb == 0 ||
		pCallback == NULL)
		return E_INVALIDARG;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (!ThreadIsRun()) {
		if (!ThreadStart()) //启动异步读线程
			return E_ABORT;
	}

	_async_read_ps.pb = pb;
	_async_read_ps.size = cb;
	_async_read_ps.callback = pCallback; //保存读取完成后的响应cb
	_async_read_ps.punkState = punkState;
	_async_read_ps.AddRef();
	SetEvent(_event_async_read); //通知异步线程做读操作
	return S_OK;
}

HRESULT MultipartStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_async_read_ps.callback == NULL)
		return E_UNEXPECTED;
	if (pResult == NULL)
		return E_INVALIDARG;

	if (pcbRead)
		*pcbRead = _async_read_ps.cbRead; //返回读取到的字节数
	_async_read_ps.Release();
	memset(&_async_read_ps, 0, sizeof(_async_read_ps));
	return S_OK;
}

HRESULT MultipartStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
{
	/*
	Seek的时候有很多情况，首先我们知道，保存Header信息的MemoryStream保存了MKV的文件头+第一个Cluster(Frame)的字节流数据。
	为什么需要第一个Cluster的数据？因为Demuxer组件一般都是Parse到MKV头的第一个Cluster的地方停止的，如果没找到第一个Cluster，Demux可能会失败。
	1、刚刚打开（Open呼叫后，状态为AfterOpen），还没有真正进行到MatroskaReadFile的时候 -> 调整一下Header的MemoryStream指针到指定的位置就行，一般多用于Demux做检测文件头。
	2、如果已经进入MatroskaReadFile流程（stm_cur_pos大宇Header的Stream大小），Seek的指针又不大宇Header的Stream长度，则做Reset后真正去Seek到Header的Stream，需要跳过第一个Packet，然后变更状态为AfterOpen。如Seek的字节指针大宇Header的Stream，返回失败，用户应该做TimeSeek。
	3、如果在状态为AfterSeek的情况下做Seek，不管怎么样，都当作已经进入了MatroskaReadFile流程，见第二个处理。
	4、如果TimeSeek后，state变成AfterSeek，不需要跳过第一个Packet，也不需要从Header的Stream开始，直接进入MatroskaReadFile流程。
	5、如果TimeSeek的时间是0.0，则同第二个方法，转发给Reset。
	*/

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (SeekOrigin == 0x10000)
	{
		//FFmpeg avio_size...
		*pqwCurrentPosition = _stm_length;
		return S_OK;
	}
	if ((dwSeekFlags & MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO) != 0) {
		if (ThreadIsRun()) {
			//如果要求Seek时取消等待中的IO
			//则做一个卡死等待，等待异步读线程返回
			SetEvent(_event_exit_thr);
			ThreadWait(); //等待异步读取线程退出
			ResetEvent(_event_exit_thr);
		}
	}

	//当前是否已经进入ReadFile流程
	bool enter_matroska_read_file = _stm_cur_pos > _header.GetLength();
	if (_state == AfterSeek) //如果是TimeSeek后，肯定是已经进入ReadFile
		enter_matroska_read_file = true;

	if (llSeekOffset > INT32_MAX)
		return E_FAIL;

	unsigned offset = (unsigned)llSeekOffset;
	if (enter_matroska_read_file) {
		Reset(false); //重设
		_stm_cur_pos = 0;
		_state = AfterOpen; //变成Open时的状态
		if (SeekOrigin == msoCurrent)
			return S_OK;
		if (offset > _header.GetLength())
			return S_OK;
		_header.SetOffset(offset);
	}else{
		if (SeekOrigin == msoCurrent)
			offset += _header.GetOffset();
		if (offset > _header.GetLength())
			return E_FAIL;
		_header.SetOffset(offset); //直接seek头即可
	}
	_stm_cur_pos = _header.GetOffset();
	if (pqwCurrentPosition)
		*pqwCurrentPosition = _stm_cur_pos;
	_flag_eof = false;
	return S_OK;
}

HRESULT MultipartStream::Close()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_closed)
		return S_OK;

	SetEvent(_event_exit_thr); //通知异步读线程可以退出了
	ThreadWait(); //同步等待异步读线程退出（如果存在）

	if (_async_time_seek.seekThread) {
		delete _async_time_seek.seekThread;
		_async_time_seek.seekThread = NULL;
	}

	DeInit(); //销毁父类资源
	_header.SetLength(0);
	_header.Buffer()->Free();
	_closed = true; //关闭标记
	return S_OK;
}

HRESULT MultipartStream::IsTimeSeekSupported(BOOL *pfTimeSeekIsSupported)
{
	if (pfTimeSeekIsSupported == NULL)
		return E_POINTER;
	*pfTimeSeekIsSupported = TRUE; //mediasource通过这个判断stream是不是支持timeseek
	return S_OK;
}

HRESULT MultipartStream::TimeSeek(QWORD qwTimePosition)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (MFGetAttributeUINT32(_attrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 0)
		return SeekTo((double)qwTimePosition / 10000000.0) ? S_OK : E_FAIL;

	if (_async_time_seek.seekThread)
		WaitTimeSeekResult();

	_async_time_seek.result = false;
	double time = (double)qwTimePosition / 10000000.0;
	auto thr = new(std::nothrow) std::thread(&MultipartStream::DoTimeSeekAsync,this,time);
	if (thr == NULL)
		return E_OUTOFMEMORY;

	_async_time_seek.seekThread = thr;
	WaitForSingleObjectEx(_async_time_seek.eventStarted, INFINITE, FALSE);
	return S_OK;
}

HRESULT MultipartStream::EnableBuffering(BOOL fEnable)
{
	if (fEnable == FALSE)
		return StopBackgroundTransfer();

#ifdef _DEBUG
	OutputDebugStringA("MultipartStream::EnableBuffering.");
#endif
	return S_OK;
}

HRESULT MultipartStream::StopBackgroundTransfer()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (GetDownloadManager()) {
		for (unsigned i = 0; i < GetItemCount(); i++)
			DownloadItemPause(i); //暂停所有后台下载任务
	}
	_flag_user_stop = true;
	return S_OK;
}

bool MultipartStream::Open()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (!Init(_list_file)) //初始化父类资源，核心代码
		return false;
	if (MatroskaHeadBytes()->Length() == 0)
		return false;

	_header_prue_size = MatroskaHeadBytes()->Length(); //MKV头（第一个Cluster前）
	_header.SetOffset(0);
	_header.Write(MatroskaHeadBytes()->Get<void*>(), MatroskaHeadBytes()->Length());

	//取得第一个Packet（Cluster）的数据
	unsigned first_size = MatroskaInternalBuffer()->GetLength();
	MemoryBuffer buf;
	if (!buf.Alloc(first_size))
		return false;

	//在MKV头后面Append第一个Cluster的数据
	if (!MatroskaReadFile(buf.Get<void*>(), first_size))
		return false;
	_header.Write(buf.Get<void*>(), first_size);
	_header.SetOffset(0);

	_stm_length = InternalGetStreamSize(); //所有切片的大小（可能是预估的）
	if (_stm_length == 0)
		return false;
	if (_stm_length == GetItems()->Size)
		_stm_length *= GetItemCount();

	_attrs->SetUINT64(MF_BYTESTREAM_DURATION, GetConfigs()->DurationMs * 10000);
	_attrs->SetString(MF_BYTESTREAM_ORIGIN_NAME, _list_file);
	_attrs->SetString(MF_BYTESTREAM_CONTENT_TYPE, L"video/x-matroska");
	if (GetItems()->Url[1] != L':') { //给自己的MFSource的一个flag
		_attrs->SetString(MF_BYTESTREAM_CONTENT_TYPE, L"video/force-network-stream");
		_attrs->SetUINT32(MF_BYTESTREAM_TRANSCODED, 1234);
	}

	_state = AfterOpen; //状态是Open后
	return true;
}

void MultipartStream::ThreadInvoke(void*)
{
	AddRef();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
	HANDLE handles[2] = {_event_async_read, _event_exit_thr};
	while (1)
	{
		DWORD state = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, FALSE);
		if (state != 0) //如果不是_event_async_read请求，那就是exit或者错误，退出线程
			break;

		ComPtr<IMFAsyncResult> result;
		{
			std::lock_guard<decltype(_mutex)> lock(_mutex);
			MFCreateAsyncResult(NULL,
				_async_read_ps.callback,
				_async_read_ps.punkState,
				&result);
			result->SetStatus(S_OK);
			//做读取
			_async_read_ps.cbRead = ReadFile(_async_read_ps.pb, _async_read_ps.size);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			if (CheckWin10MatroskaSource(_async_read_ps.callback)) {
				if (_async_read_ps.cbRead > 0)
					_async_read_ps.cbRead += ReadFile(_async_read_ps.pb + _async_read_ps.cbRead,
						_async_read_ps.size - _async_read_ps.cbRead);
			}
#endif
		}
		MFInvokeCallback(result.Get()); //通知异步callback，做EndRead
	}
}

unsigned MultipartStream::InternalGetStreamSize()
{
	if (GetItemCount() == 0)
		return false;
	unsigned size = 0;
	for (unsigned i = 0; i < GetItemCount(); i++)
		size += (GetItems() + i)->Size;
	return size;
}

double MultipartStream::InternalGetStreamDuration()
{
	if (GetItemCount() == 0)
		return false;
	double time = 0;
	for (unsigned i = 0; i < GetItemCount(); i++)
		time += (GetItems() + i)->Duration;
	return time;
}

unsigned MultipartStream::ReadFile(PBYTE pb, unsigned size)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_async_time_seek.seekThread) {
		if (!WaitTimeSeekResult())
			return 0; //TimeSeek async failed.
	}

	unsigned allow_size = size;
	if (allow_size == 0)
		return 0;

	if (_stm_cur_pos < _header.GetLength() && _state == AfterOpen)
	{
		//当前指针在MKV头+第一个Cluster的范围内，并且不是TimeSeek后
		if (allow_size > _header.GetPendingLength())
			allow_size = _header.GetPendingLength();
		_header.Read(pb, allow_size); //则直接读这个范围内的数据
		_stm_cur_pos = _header.GetOffset();
	}else{
		//当前指针已经超过以上范围，或者是AfterSeek...
		if (_flag_user_stop) {
			_flag_user_stop = false;
			DownloadItemStart(GetIndex()); //如果用户暂停了下载，恢复
		}
		//做读取，核心调用
		allow_size = MatroskaReadFile(pb, size);
		_stm_cur_pos += allow_size;

		//如果当前处理的时间已经达到需要缓存下一个的时候
		if (int(GetCurrentTotalTime() - GetCurrentTimestamp()) <= 
			GetConfigs()->PreloadNextPartRemainSeconds)
			DownloadNextItem(); //启动下一个的item的下载

		/*
		//如果当前处理的这个part已经过半，缓存下一个
		if (GetCurrentPartReadSize() > 
			(GetCurrentPartTotalSize() / 2))
			DownloadNextItem(); //启动下一个的item的下载
		*/
	}
	if (allow_size == 0)
		_flag_eof = true;
	return allow_size;
}

bool MultipartStream::SeekTo(double time)
{
	if (time < 0.1) //过小，直接Reset
		return Reset(true);

	if (!MatroskaTimeSeek(time)) //做TimeSeek
		return false;
	_flag_eof = false;
	_state = AfterSeek; //状态从AfterOpen变成AfterSeek，即TimeSeek后
	return true;
}

bool MultipartStream::Reset(bool timeseek)
{
	if (timeseek) {
		//AfterSeek
		//不需要跳过第一个packet
		if (!MatroskaTimeSeek(0.0))
			return Reset(false);
	}else{
		//AfterOpen
		//需要跳过第一个packet
		if (!ReInit(false))
			return false;
		MatroskaReadFile(NULL, 0, 1); //skip first pkt.
		return true;
	}
	for (unsigned i = 1; i < GetItemCount(); i++)
		DownloadItemStop(i);
	return true;
}

bool MultipartStream::WaitTimeSeekResult()
{
	_async_time_seek.seekThread->join();
	delete _async_time_seek.seekThread;
	_async_time_seek.seekThread = NULL;
	return _async_time_seek.result;
}

void MultipartStream::DoTimeSeekAsync(double time)
{
	AddRef();
	SetEvent(_async_time_seek.eventStarted);
	_async_time_seek.result = SeekTo(time);
	Release();
}