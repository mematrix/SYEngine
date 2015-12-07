/* ---------------------------------------------------------------
 -
 - 实现基本的IMFByteStream对象，输出mkv文件live流。
 - 创建于：2015-09-21
 - 作者：ShanYe
 --------------------------------------------------------------- */

#pragma once

#include "pch.h"
#include "ThreadImpl.h"
#include "Core\MatroskaJoinStream.h"

/* Todo List...
 IMFByteStream -> 提供数据流。
 IMFByteStreamTimeSeek -> 接管Seek操作。
 IMFByteStreamBuffering -> 提供缓存控制，还必须实现IMFMediaEventGenerator。
 IMFByteStreamCacheControl -> 提供停止缓存的操作。
 IMFAttributes -> 提供属性集合。
 IMFGetService -> 提供查询取得IPropertyStore的方式。
 IPropertyStore -> 提供下载进度。
*/

class MultipartStream : 
	public IMFByteStream,
	public IMFByteStreamTimeSeek,
	public IMFByteStreamCacheControl,
	public IMFAttributes,
	public IMFGetService,
	protected ThreadImpl,
	protected MatroskaJoinStream {

public:
	explicit MultipartStream(const wchar_t* list) :
		_ref_count(1), _state(Invalid), _flag_eof(false), _flag_user_stop(false),
		_stm_length(0), _stm_cur_pos(0), _header_prue_size(0), _closed(true) {
		_event_async_read = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
		_event_exit_thr = CreateEventExW(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
		wcscpy_s(_list_file, list);
		MFCreateAttributes(&_attrs, 0);
		memset(&_async_read_ps, 0, sizeof(_async_read_ps));
		Module<InProc>::GetModule().IncrementObjectCount();
	}
	virtual ~MultipartStream()
	{
		Close();
		CloseHandle(_event_async_read);
		CloseHandle(_event_exit_thr);
		Module<InProc>::GetModule().DecrementObjectCount();
	}

	bool Open();

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef()
	{ return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ auto rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

public: //IMFByteStream
	STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities);
	STDMETHODIMP GetLength(QWORD *pqwLength);
	STDMETHODIMP SetLength(QWORD qwLength) { return E_NOTIMPL; }
	STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition);
	STDMETHODIMP SetCurrentPosition(QWORD qwPosition)
	{ return Seek(msoBegin, (LONG64)qwPosition, 0, NULL); }
	STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream);

	STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead);
	STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
	STDMETHODIMP EndRead(IMFAsyncResult *pResult, ULONG *pcbRead);

	STDMETHODIMP Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
	{ return E_NOTIMPL; }
	STDMETHODIMP BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
	{ return E_NOTIMPL; }
	STDMETHODIMP EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
	{ return E_NOTIMPL; }

	STDMETHODIMP Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition);
	STDMETHODIMP Flush() { return S_OK; }
	STDMETHODIMP Close();

public: //IMFByteStreamTimeSeek
	STDMETHODIMP IsTimeSeekSupported(BOOL *pfTimeSeekIsSupported);
	STDMETHODIMP TimeSeek(QWORD qwTimePosition);
	STDMETHODIMP GetTimeSeekResult(QWORD *pqwStartTime, QWORD *pqwStopTime, QWORD *pqwDuration)
	{ return E_NOTIMPL; }

public: //IMFByteStreamCacheControl
	STDMETHODIMP StopBackgroundTransfer();

public: //IMFAttributes
	STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItem(guidKey, pValue); }
	STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType)
	{ return _attrs->GetItemType(guidKey, pType); }
	STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult)
	{ return _attrs->CompareItem(guidKey, Value, pbResult); }
	STDMETHODIMP Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult)
	{ return _attrs->Compare(pTheirs, MatchType, pbResult); }
	STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32 *punValue)
	{ return _attrs->GetUINT32(guidKey, punValue); }
	STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64 *punValue)
	{ return _attrs->GetUINT64(guidKey, punValue); }
	STDMETHODIMP GetDouble(REFGUID guidKey, double *pfValue)
	{ return _attrs->GetDouble(guidKey, pfValue); }
	STDMETHODIMP GetGUID(REFGUID guidKey, GUID *pguidValue)
	{ return _attrs->GetGUID(guidKey, pguidValue); }
	STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32 *pcchLength)
	{ return _attrs->GetStringLength(guidKey, pcchLength); }
	STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength)
	{ return _attrs->GetString(guidKey, pwszValue, cchBufSize, pcchLength); }
	STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength)
	{ return _attrs->GetAllocatedString(guidKey, ppwszValue, pcchLength); }
	STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlobSize(guidKey, pcbBlobSize); }
	STDMETHODIMP GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize); }
	STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize)
	{ return _attrs->GetAllocatedBlob(guidKey, ppBuf, pcbSize); }
	STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv)
	{ return _attrs->GetUnknown(guidKey, riid, ppv); }
	STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value)
	{ return _attrs->SetItem(guidKey, Value); }
	STDMETHODIMP DeleteItem(REFGUID guidKey)
	{ return _attrs->DeleteItem(guidKey); }
	STDMETHODIMP DeleteAllItems()
	{ return _attrs->DeleteAllItems(); }
	STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue)
	{ return _attrs->SetUINT32(guidKey, unValue); }
	STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue)
	{ return _attrs->SetUINT64(guidKey, unValue); }
	STDMETHODIMP SetDouble(REFGUID guidKey, double fValue)
	{ return _attrs->SetDouble(guidKey, fValue); }
	STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue)
	{ return _attrs->SetGUID(guidKey, guidValue); }
	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue)
	{ return _attrs->SetString(guidKey, wszValue); }
	STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize)
	{ return _attrs->SetBlob(guidKey, pBuf, cbBufSize); }
	STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown *pUnknown)
	{ return _attrs->SetUnknown(guidKey, pUnknown); }
	STDMETHODIMP LockStore()
	{ return _attrs->LockStore(); }
	STDMETHODIMP UnlockStore()
	{ return _attrs->UnlockStore(); }
	STDMETHODIMP GetCount(UINT32 *pcItems)
	{ return _attrs->GetCount(pcItems); }
	STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItemByIndex(unIndex, pguidKey, pValue); }
	STDMETHODIMP CopyAllItems(IMFAttributes *pDest)
	{ return _attrs->CopyAllItems(pDest); }

public: //IMFGetService
	STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
	{ return E_NOTIMPL; }

protected:
	virtual void ThreadInvoke(void*);
	virtual void ThreadEnded() { Release(); }
	enum CurrentState
	{
		Invalid,
		AfterOpen,
		AfterSeek
	};
	CurrentState _state;

private:
	unsigned InternalGetStreamSize();
	double InternalGetStreamDuration();

	unsigned ReadFile(PBYTE pb, unsigned size);
	bool Reset(bool timeseek);

private:
	ULONG _ref_count; //COM引用计数
	std::recursive_mutex _mutex; //安全同步lock
	ComPtr<IMFAttributes> _attrs; //属性的thunk

	WCHAR _list_file[MAX_PATH]; //分p的文件列表（可以是本地或者网络）
	QWORD _stm_length, _stm_cur_pos; //不准（fake）的流长度和curpos
	bool _flag_eof, _flag_user_stop;

	MemoryStream _header; //存储MKV头+第一个Cluster
	unsigned _header_prue_size; //仅MKV头的大小

	struct AsyncReadParameters
	{
		PBYTE pb;
		unsigned size;
		IMFAsyncCallback* callback;
		IUnknown* punkState;
		unsigned cbRead;

		void AddRef()
		{
			callback->AddRef();
			if (punkState)
				punkState->AddRef();
		}
		void Release()
		{
			callback->Release();
			if (punkState)
				punkState->Release();
		}
	};
	AsyncReadParameters _async_read_ps; //异步读参数
	HANDLE _event_async_read, _event_exit_thr; //请求异步读和请求退出事件

	bool _closed;
};