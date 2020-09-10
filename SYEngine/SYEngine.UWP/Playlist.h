#pragma once
#include "Playlist.g.h"


namespace winrt::SYEngine::implementation
{
    struct Playlist : PlaylistT<Playlist>
    {
        Playlist() = default;

        Playlist(SYEngine::PlaylistTypes const& type);

        virtual ~Playlist() noexcept;
        SYEngine::PlaylistNetworkConfigs NetworkConfigs() const;
        void NetworkConfigs(SYEngine::PlaylistNetworkConfigs const& value);
        bool Append(hstring const& url, int32_t sizeInBytes, float durationInSeconds);
        void Clear();
        void SetDebugFile(hstring const& fileName);
        Windows::Foundation::IAsyncOperation<Windows::Foundation::Uri> SaveAndGetFileUriAsync();

    private:
        class LineStream
        {
        public:
            virtual ~LineStream() = default;

            virtual bool HasNext() noexcept = 0;

            virtual std::string Next() = 0;
        };

        hstring SaveFile();

        std::unique_ptr<LineStream> SerializeForLocalFile() noexcept;

        std::unique_ptr<LineStream> SerializeForNetworkHttp() noexcept;

        SYEngine::PlaylistTypes type_ = PlaylistTypes::LocalFile;
        SYEngine::PlaylistNetworkConfigs cfgs_;
        hstring debug_file_;

        struct Item
        {
            hstring url;
            int size_in_bytes;
            float duration_in_seconds;
        };
        std::vector<Item> list_;
    };
}
namespace winrt::SYEngine::factory_implementation
{
    struct Playlist : PlaylistT<Playlist, implementation::Playlist>
    {
    };
}
