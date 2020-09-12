#include "pch.h"
#include "Playlist.h"

#include <winrt/Windows.Storage.h>

#include "Playlist.g.cpp"


namespace winrt::SYEngine::implementation {

Playlist::Playlist(SYEngine::PlaylistTypes const &type) : type_(type), debug_file_() {}

Playlist::~Playlist() noexcept
{
    Clear();
}

SYEngine::PlaylistNetworkConfigs Playlist::NetworkConfigs() const
{
    return cfgs_;
}

void Playlist::NetworkConfigs(SYEngine::PlaylistNetworkConfigs const &value)
{
    cfgs_ = value;
}

bool Playlist::Append(hstring const &url, int32_t sizeInBytes, float durationInSeconds)
{
    if (type_ == PlaylistTypes::LocalFile) {
        auto hf = CreateFile2(url.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
        if (hf == INVALID_HANDLE_VALUE) {
            return false;
        }
        CloseHandle(hf);
    }

    list_.push_back({url, sizeInBytes, durationInSeconds});
    return true;
}

void Playlist::Clear()
{
    list_.clear();
}

void Playlist::SetDebugFile(hstring const &fileName)
{
    debug_file_ = fileName;
    if (!fileName.empty()) {
        DeleteFileW(fileName.c_str());
    }
}

Windows::Foundation::IAsyncOperation<Windows::Foundation::Uri> Playlist::SaveAndGetFileUriAsync()
{
    //winrt::get_cancellation_token
    co_await winrt::resume_background();

    auto uri = SaveFile();
    if (uri.empty()) {
        co_return Windows::Foundation::Uri(nullptr);
    }

    hstring protocol(L"plist://WinRT-TemporaryFolder_");
    co_return Windows::Foundation::Uri(protocol + uri);
}

hstring Playlist::SaveFile()
{
    std::unique_ptr<LineStream> line_stream;
    if (type_ == PlaylistTypes::LocalFile) {
        line_stream = SerializeForLocalFile();
    } else if (type_ == PlaylistTypes::NetworkHttp) {
        line_stream = SerializeForNetworkHttp();
    }
    if (!line_stream) {
        return hstring();
    }

    hstring file_name = L"MultipartStreamMatroska_" + to_hstring(GetTickCount64()) + L".txt";
    auto temp_folder = Windows::Storage::ApplicationData::Current().TemporaryFolder().Path();
    auto temp_file = temp_folder + L"\\" + file_name;

    DeleteFileW(temp_file.c_str());
    auto file = CreateFile2(temp_file.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return hstring();
    }

    if (line_stream->HasNext()) {
        do {
            auto line = line_stream->Next();
            WriteFile(file, line.data(), line.size(), nullptr, nullptr);

            if (line_stream->HasNext()) {
                WriteFile(file, "\r\n", 2, nullptr, nullptr);
            } else {
                break;
            }
        } while (true);
    }
    FlushFileBuffers(file);
    CloseHandle(file);

    return file_name;
}

std::unique_ptr<Playlist::LineStream> Playlist::SerializeForLocalFile() noexcept
{
    class LocalFileLineStream : public LineStream
    {
    public:
        explicit LocalFileLineStream(const std::vector<Item> &list) : list_(list), it_(list_.begin()) {}

        bool HasNext() noexcept override
        {
            return !handle_list_ || it_ != list_.end();
        }

        std::string Next() override
        {
            if (!handle_list_) {
                handle_list_ = true;
                return "LOCAL";
            }

            return to_string((it_++)->url);
        }

    private:
        bool handle_list_ = false;
        const std::vector<Item> &list_;
        decltype(list_.begin()) it_;
    };

    return std::make_unique<LocalFileLineStream>(list_);
}

std::unique_ptr<Playlist::LineStream> Playlist::SerializeForNetworkHttp() noexcept
{
    class HttpLineStream : public LineStream
    {
    public:
        HttpLineStream(const PlaylistNetworkConfigs &cfg, const hstring &dbg_file, const std::vector<Item> &list)
            : status_(Status::serial_header_1),
              cfg_(cfg),
              dbg_file_(dbg_file),
              list_(list),
              it_(list_.begin()) { }

        bool HasNext() noexcept override
        {
            return !(status_ == Status::serial_list_size && it_ == list_.end());
        }

        std::string Next() override
        {
            switch (status_) {
                case Status::serial_header_1: {
                    auto duration = cfg_.ExplicitTotalDurationSeconds;
                    if (duration < 0.1) {
                        for (const auto &i : list_) {
                            duration += i.duration_in_seconds;
                        }
                    }

                    status_ = Status::serial_header_2;
                    return std::to_string(static_cast<int>(duration * 1000.0));
                }
                case Status::serial_header_2: {
                    status_ = Status::serial_header_3;
                    return cfg_.DetectDurationForParts ? "FULL" : "NO";
                }
                case Status::serial_header_3: {
                    status_ = Status::serial_header_4;
                    return cfg_.NotUseCorrectTimestamp ? "NextUseDuration" : "";
                }
                case Status::serial_header_4: {
                    status_ = Status::serial_header_5;
                    return std::to_string(cfg_.FetchNextPartThresholdSeconds > 1 ? cfg_.FetchNextPartThresholdSeconds : 30);
                }
                case Status::serial_header_5: {
                    status_ = Status::serial_header_6;
                    return cfg_.DownloadRetryOnFail ? "Reconnect" : "NO";
                }
                case Status::serial_header_6: {
                    status_ = Status::serial_header_7;
                    return std::to_string(cfg_.BufferBlockSizeKB > 8 ? cfg_.BufferBlockSizeKB : 64) + "|" +
                        std::to_string(cfg_.BufferBlockCount > 2 ? cfg_.BufferBlockCount : 80);
                }
                case Status::serial_header_7: {
                    status_ = Status::serial_header_8;
                    return "NULL";
                }
                case Status::serial_header_8: {
                    status_ = Status::serial_header_9;
                    return to_string(cfg_.HttpCookie);
                }
                case Status::serial_header_9: {
                    status_ = Status::serial_header_10;
                    return to_string(cfg_.HttpReferer);
                }
                case Status::serial_header_10: {
                    status_ = Status::serial_header_11;
                    return to_string(cfg_.HttpUserAgent);
                }
                case Status::serial_header_11: {
                    status_ = Status::serial_header_12;
                    return to_string(cfg_.UniqueId);
                }
                case Status::serial_header_12: {
                    status_ = Status::serial_list_size;
                    return to_string(dbg_file_);
                }
                case Status::serial_list_size: {
                    status_ = Status::serial_list_url;
                    if (it_->size_in_bytes > 0 || it_->duration_in_seconds > 0.1) {
                        return std::to_string(it_->size_in_bytes) + "|" +
                            std::to_string(static_cast<int>(it_->duration_in_seconds * 1000.0));
                    }
                    return "0";
                }
                case Status::serial_list_url: {
                    status_ = Status::serial_list_size;
                    return to_string((it_++)->url);
                }
                default: {
                    assert(false);
                    return "";
                }
            }
        }

    private:
        enum class Status : int
        {
            serial_header_1,
            serial_header_2,
            serial_header_3,
            serial_header_4,
            serial_header_5,
            serial_header_6,
            serial_header_7,
            serial_header_8,
            serial_header_9,
            serial_header_10,
            serial_header_11,
            serial_header_12,
            serial_list_size,
            serial_list_url
        };

        Status status_;
        const PlaylistNetworkConfigs &cfg_;
        const hstring &dbg_file_;
        const std::vector<Item> &list_;
        decltype(list_.begin()) it_;
    };

    return std::make_unique<HttpLineStream>(cfgs_, debug_file_, list_);
}

}
