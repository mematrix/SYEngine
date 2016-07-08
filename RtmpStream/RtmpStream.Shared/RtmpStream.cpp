#include <algorithm>
#include <Windows.h>
#include <winsock.h>
#include "RtmpStream.h"
#include "librtmp/rtmp.h"

RtmpStream::RtmpStream(std::wstring& rtmp_url)
    : _ref_count(1), _rtmp(nullptr), _rtmp_connected(false), _received_length(0), _first_read(true), _is_eof(false) {
    // ↑ fuck ref count

    MFCreateAttributes(&_attrs, 0);

    _event_async_read = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    _event_async_exit = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);

    memset(_flv_header, 0, sizeof(_flv_header));
    memset(&_async_read_ps, 0, sizeof(_async_read_ps));

    int len = WideCharToMultiByte(CP_ACP, 0, rtmp_url.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 0) {
        _url.reserve(len + 1);
        WideCharToMultiByte(CP_ACP, 0, rtmp_url.c_str(), -1, &_url[0], len + 1, nullptr, nullptr);
    }
    Module<InProc>::GetModule().IncrementObjectCount();
}

RtmpStream::~RtmpStream() {
    if (_rtmp) {
        Close();
    }

    CloseHandle(_event_async_read);
    CloseHandle(_event_async_exit);

    Module<InProc>::GetModule().DecrementObjectCount();
}

HRESULT RtmpStream::QueryInterface(REFIID iid, void** ppv) {
    if (ppv == nullptr)
        return E_POINTER;

    *ppv = nullptr;

    if (iid == IID_IMFByteStream) {
        *ppv = static_cast<IMFByteStream*>(this);
    } else if (iid == IID_IMFAttributes) {
        *ppv = static_cast<IMFAttributes*>(this);
    } else if (iid == IID_IMFByteStreamBuffering) {
        *ppv = static_cast<IMFByteStreamBuffering*>(this);
    } else {
        return E_NOINTERFACE;
    }

    this->AddRef();
    return S_OK;
}

bool RtmpStream::Open(IMFAttributes* config) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    _rtmp = RTMP_Alloc();
    if (_rtmp == nullptr) {
        return false;
    }

    RTMP_Init(_rtmp);

    if (!RTMP_SetupURL(_rtmp, const_cast<char*>(_url.c_str()))) {
        RTMP_Free(_rtmp);
        _rtmp = nullptr;
        return false;
    }

    _rtmp->Link.lFlags |= RTMP_LF_LIVE;
    RTMP_SetBufferMS(_rtmp, 500);

    if (!RTMP_Connect(_rtmp, nullptr)) {
        RTMP_Close(_rtmp);
        RTMP_Free(_rtmp);
        _rtmp = nullptr;
        return false;
    }

    if (!RTMP_ConnectStream(_rtmp, 0)) {
        RTMP_Close(_rtmp);
        RTMP_Free(_rtmp);
        _rtmp = nullptr;
        return false;
    }

    _thread = std::thread(&RtmpStream::RtmpLoop, this, nullptr);

    _rtmp_connected = true;
    return true;
}

STDMETHODIMP RtmpStream::Close() {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    if (_rtmp_connected || _rtmp) {
        RTMP_Close(_rtmp);
        RTMP_Free(_rtmp);
        _rtmp_connected = false;
        _rtmp = nullptr;
    }

    if (_thread.joinable()) {
        SetEvent(_event_async_exit);
        _thread.join();
    }

    return S_OK;
}

STDMETHODIMP RtmpStream::GetCapabilities(DWORD *pdwCapabilities) {
    if (pdwCapabilities == NULL)
        return E_POINTER;
    *pdwCapabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE |
        MFBYTESTREAM_IS_REMOTE | MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED;
    //可以读、Seek。流来自远程服务器。
    return S_OK;
}

STDMETHODIMP RtmpStream::GetCurrentPosition(QWORD *pqwPosition) {
    if (pqwPosition == nullptr)
        return E_POINTER;
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    *pqwPosition = (QWORD)_received_length;
    return S_OK;
}

STDMETHODIMP RtmpStream::IsEndOfStream(BOOL *pfEndOfStream) {
    if (pfEndOfStream == nullptr) {
        return E_POINTER;
    }
    *pfEndOfStream = _is_eof ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP RtmpStream::EnableBuffering(BOOL fEnable) {
    if (fEnable == FALSE && _rtmp) {
        RTMP_Close(_rtmp);
        RTMP_Free(_rtmp);
        _rtmp = nullptr;

        if (_thread.joinable()) {
            SetEvent(_event_async_exit);
            _thread.join();
        }
    }
    return S_OK;
}

STDMETHODIMP RtmpStream::StopBuffering() {
    if (_rtmp) {
        RTMP_Close(_rtmp);
        RTMP_Free(_rtmp);
        _rtmp = nullptr;

        if (_thread.joinable()) {
            SetEvent(_event_async_exit);
            _thread.join();
        }
    }
    return S_OK;
}

STDMETHODIMP RtmpStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    bool is_header = false;

    int read = this->ReadRtmp((uint8_t*)pb, cb, &is_header);
    if (read < 0) {
        return E_FAIL;
    } else if (read == 0) {
        _is_eof = true;
        // also set 0 to *pcbRead
    } else {
        if (!is_header)
            _received_length += read;
    }


    *pcbRead = (ULONG)read;

    return S_OK;
}

STDMETHODIMP RtmpStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    if (_rtmp == nullptr)
        return E_FAIL;

    _async_read_ps.pb = pb;
    _async_read_ps.size = cb;
    _async_read_ps.callback = pCallback;
    _async_read_ps.punkState = punkState;
    _async_read_ps.AddRef();

    SetEvent(_event_async_read);

    return S_OK;
}

STDMETHODIMP RtmpStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    if (_async_read_ps.callback == nullptr)
        return E_UNEXPECTED;
    if (pResult == nullptr)
        return E_UNEXPECTED;

    if (pcbRead)
        *pcbRead = _async_read_ps.cbRead;

    _async_read_ps.Release();
    memset(&_async_read_ps, 0, sizeof(_async_read_ps));

    return S_OK;
}

STDMETHODIMP RtmpStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition) {
    if (llSeekOffset < sizeof(_flv_header)) {
        _request_seek = true;
        return S_OK;
    }
    return E_FAIL;
}

int RtmpStream::ReadRtmp(uint8_t* buffer, size_t size, bool* is_header) {
    if (_first_read) {
        _first_read = false;
        RTMP_Read(_rtmp, (char*)_flv_header, sizeof(_flv_header));
        _received_length += sizeof(_flv_header);
        return this->ReadRtmp(buffer, size, is_header);
    }
    if (_request_seek == true && _first_read == false && _received_length >= sizeof(_flv_header)) {
        _request_seek = false;
        int length = (std::min)(size, sizeof(_flv_header));
        memcpy(buffer, _flv_header, length);
        if (is_header) *is_header = true;
        return length;
    }
    return RTMP_Read(_rtmp, (char*)buffer, (int)size);
}

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void RtmpStream::RtmpLoop(void*) {
    DWORD thread_id = GetCurrentThreadId();
    SetThreadName(thread_id, "Rtmp Receive Thread");

    while (true) {
        HANDLE events[] = {_event_async_read, _event_async_exit};
        DWORD state = WaitForMultipleObjectsEx(2, events, FALSE, INFINITE, FALSE);
        if (state != WAIT_OBJECT_0) {  // exit if event_async_exit
            break;
        }
        if (_rtmp == nullptr) {
            break;
        }

        ComPtr<IMFAsyncResult> result;
        {
            std::lock_guard<decltype(_mutex)> lock(_mutex);
            MFCreateAsyncResult(nullptr,
                                _async_read_ps.callback,
                                _async_read_ps.punkState,
                                &result
            );

            bool is_header = false;

            int read = this->ReadRtmp(_async_read_ps.pb, _async_read_ps.size, &is_header);
            if (read < 0) {
                result->SetStatus(E_FAIL);
                break;
            } else if (read == 0) {
                _is_eof = true;
                break;
            } else {
                if (!is_header)
                    _received_length += read;
            }

            result->SetStatus(S_OK);
            _async_read_ps.cbRead = read;

        }
        MFInvokeCallback(result.Get());
    }
}

void RtmpStream::SetThreadName(DWORD thread_id, const char* thread_name) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = thread_name;
    info.dwThreadID = thread_id;
    info.dwFlags = 0;

#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
}