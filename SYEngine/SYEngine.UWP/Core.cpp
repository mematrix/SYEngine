#include "pch.h"
#include "Core.h"
#include "Core.g.cpp"

#include <sstream>
#include <winsock2.h>

#include "MediaExtensionInstaller.h"


#define _STM_HANDLER_CLSID L"{1A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"
#define _URL_HANDLER_CLSID L"{2A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"
#define _RTMP_HANDLER_CLSID L"{F3B71F9B-0656-4562-8ED8-97C2C1A10F30}"

#define _STM_HANDLER_NAME L"CoreMFSource.HDCoreByteStreamHandler"
#define _URL_HANDLER_NAME L"MultipartStreamMatroska.UrlHandler"
#define _RTMP_HANDLER_NAME L"RtmpStream.RtmpUrlHandler"

#define _STM_HANDLER_FILE L"CoreMFSource.dll"
#define _URL_HANDLER_FILE L"MultipartStreamMatroska.dll"
#define _RTMP_HANDLER_FILE L"RtmpStream.dll"

//MediaExtensionCallback.cpp
extern double MediaSourceNetworkIOBufTime;
extern unsigned MediaSourceNetworkIOBufSize;
extern bool MediaSourceForceNetworkMode;
extern bool MediaSourceForceSoftwareDecode;

namespace winrt::SYEngine::implementation {

PlaylistSegmentUrlUpdateEventHandler Core::url_update_handler_;
PlaylistSegmentDetailUpdateEventHandler Core::detail_update_handler_;

static MediaExtensionInstaller *Installer = nullptr;

bool Core::Initialize()
{
    if (Installer) {
        return false;
    }

    Installer = new(std::nothrow) MediaExtensionInstaller();
    if (!Installer) {
        return false;
    }

    if (!Installer->InstallSchemeHandler(_URL_HANDLER_CLSID, _URL_HANDLER_NAME,
                                         _URL_HANDLER_FILE, L"plist:")) {
        return false;
    }

    if (!Installer->InstallSchemeHandler(_RTMP_HANDLER_CLSID, _RTMP_HANDLER_NAME,
                                         _RTMP_HANDLER_FILE, L"rtmp:")) {
        return false;
    }

    struct ByteStreamHandlerPair
    {
        LPCWSTR FileExtension;
        LPCWSTR MimeType;
    };
    static ByteStreamHandlerPair Handlers[] = {
        {nullptr, L"video/force-network-stream"},
        {L".mkv", L"video/x-matroska"},
        {L".flv", L"video/x-flv"},
        {L".f4v", nullptr}
    };
    for (auto h : Handlers) {
        Installer->InstallByteStreamHandler(_STM_HANDLER_CLSID,
                                            _STM_HANDLER_NAME,
                                            _STM_HANDLER_FILE,
                                            h.FileExtension,
                                            h.MimeType);
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    return true;
}

bool Core::Initialize(Windows::Foundation::Collections::IMapView<hstring, hstring> const &custom_handler)
{
    if (!Initialize()) {
        return false;
    }
    if (custom_handler.Size() == 0) {
        return true;
    }

    for (auto &&pair : custom_handler) {
        auto key = pair.Key();
        auto value = pair.Value();
        Installer->InstallByteStreamHandler(_STM_HANDLER_CLSID,
                                            _STM_HANDLER_NAME, _STM_HANDLER_FILE,
                                            key.empty() ? nullptr : key.c_str(),
                                            value.empty() ? nullptr : value.c_str());
    }
    return true;
}

void Core::Uninitialize()
{
    if (Installer) {
        delete Installer;
        Installer = nullptr;
    }
}

double Core::NetworkBufferTimeInSeconds()
{
    return MediaSourceNetworkIOBufTime;
}

void Core::NetworkBufferTimeInSeconds(double value)
{
    MediaSourceNetworkIOBufTime = value;
}

int32_t Core::NetworkBufferSizeInBytes()
{
    return MediaSourceNetworkIOBufSize;
}

void Core::NetworkBufferSizeInBytes(int32_t value)
{
    MediaSourceNetworkIOBufSize = value;
}

bool Core::ForceNetworkMode()
{
    return MediaSourceForceNetworkMode;
}

void Core::ForceNetworkMode(bool value)
{
    MediaSourceForceNetworkMode = value;
}

bool Core::ForceSoftwareDecode()
{
    return MediaSourceForceSoftwareDecode;
}

void Core::ForceSoftwareDecode(bool value)
{
    MediaSourceForceSoftwareDecode = value;
}

SYEngine::PlaylistSegmentUrlUpdateEventHandler Core::PlaylistSegmentUrlUpdateDelegate()
{
    return url_update_handler_;
}

void Core::PlaylistSegmentUrlUpdateDelegate(SYEngine::PlaylistSegmentUrlUpdateEventHandler const &value)
{
    url_update_handler_ = value;
}

SYEngine::PlaylistSegmentDetailUpdateEventHandler Core::PlaylistSegmentDetailUpdateDelegate()
{
    return detail_update_handler_;
}

void Core::PlaylistSegmentDetailUpdateDelegate(SYEngine::PlaylistSegmentDetailUpdateEventHandler const &value)
{
    detail_update_handler_ = value;
}

LPSTR CALLBACK Core::DefaultPlaylistSegmentUrlUpdateCallback(LPCSTR uniqueId, LPCSTR opType, int nCurrentIndex, int nTotalCount,
                                                             LPCSTR strCurrentUrl)
{
    auto uid = winrt::to_hstring(uniqueId);
    auto typ = winrt::to_hstring(opType);
    auto url = winrt::to_hstring(strCurrentUrl);

    hstring result;
    try {
        result = url_update_handler_(uid, typ, nCurrentIndex, nTotalCount, url);
    } catch (...) {
        return nullptr;
    }
    if (result.empty() || result == url) {
        return nullptr;
    }

    int len = WideCharToMultiByte(CP_ACP, 0, result.c_str(), -1, nullptr, 0, nullptr, nullptr);
    auto *str = static_cast<char *>(CoTaskMemAlloc(len * 2));
    WideCharToMultiByte(CP_ACP, 0, result.c_str(), -1, str, len + 1, nullptr, nullptr);
    return str;
}

class PlaylistNetworkUpdateInfo : public implements<PlaylistNetworkUpdateInfo, IPlaylistNetworkUpdateInfo>
{
public:
    explicit PlaylistNetworkUpdateInfo(hstring &url) : url_(url), timeout_(0) {}

    virtual ~PlaylistNetworkUpdateInfo() noexcept { headers_.clear(); }

    hstring Url() const
    {
        return url_;
    }

    void Url(const hstring &value)
    {
        url_ = value;
    }

    int32_t Timeout() const
    {
        return timeout_;
    }

    void Timeout(int32_t value)
    {
        timeout_ = value;
    }

    bool SetRequestHeader(const hstring &name, const hstring &value)
    {
        if (name.empty()) {
            return false;
        }

        const auto it = headers_.find(name);
        if (it != headers_.end()) {
            headers_.erase(it);
            if (value.empty()) {
                return true;
            }
        }

        headers_.emplace(name, value);
        return true;
    }

    hstring GetRequestHeader(const hstring &name) const
    {
        if (name.empty()) {
            return hstring();
        }

        const auto it = headers_.find(name);
        if (it == headers_.end()) {
            return hstring();
        }

        return it->second;
    }

    hstring GetAllRequestHeaders()
    {
        if (headers_.empty()) {
            return hstring();
        }

        std::wostringstream oss;
        for (auto &header : headers_) {
            oss << static_cast<std::wstring_view>(header.first) << L": "
                << static_cast<std::wstring_view>(header.second) << L"\r";
        }

        return hstring(oss.str());
    }

private:
    hstring url_;
    int32_t timeout_;
    std::map<hstring, hstring> headers_;
};

BOOL CALLBACK Core::DefaultPlaylistSegmentDetailUpdateCallback(LPCSTR uniqueId, LPCSTR opType, int nCurrentIndex, int nTotalCount,
                                                               LPCSTR strCurrentUrl, void *values)
{
    auto uid = winrt::to_hstring(uniqueId);
    auto typ = winrt::to_hstring(opType);
    auto url = winrt::to_hstring(strCurrentUrl);

    struct UpdateItemDetailValues
    {
        LPSTR pszUrl;
        LPSTR pszRequestHeaders;
        int timeout;
    };
    auto *v = static_cast<UpdateItemDetailValues *>(values);
    auto info = make<PlaylistNetworkUpdateInfo>(url);

    try {
        if (!detail_update_handler_(uid, typ, nCurrentIndex, nTotalCount, info))
            return FALSE;
    } catch (...) {
        return FALSE;
    }

    auto updateUrl = info.Url();
    if (updateUrl.empty() || wcsstr(updateUrl.c_str(), L":") == nullptr || updateUrl == url) {
        return FALSE;
    }

    auto result = get_self<PlaylistNetworkUpdateInfo>(info)->GetAllRequestHeaders();
    if (!result.empty()) {
        int len = WideCharToMultiByte(CP_ACP, 0, result.c_str(), -1, nullptr, 0, nullptr, nullptr);
        v->pszRequestHeaders = static_cast<LPSTR>(CoTaskMemAlloc(len * 2));
        WideCharToMultiByte(CP_ACP, 0, result.c_str(), -1, v->pszRequestHeaders, len + 1, nullptr, nullptr);
    }

    v->timeout = info.Timeout();
    int len = WideCharToMultiByte(CP_ACP, 0, updateUrl.c_str(), -1, nullptr, 0, nullptr, nullptr);
    v->pszUrl = static_cast<LPSTR>(CoTaskMemAlloc(len * 2));
    WideCharToMultiByte(CP_ACP, 0, updateUrl.c_str(), -1, v->pszUrl, len + 1, nullptr, nullptr);
    return TRUE;
}

}
