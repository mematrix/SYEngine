#pragma once
#include "Core.g.h"


namespace winrt::SYEngine::implementation {

struct Core
{
    Core() = default;

    static bool Initialize();

    static bool Initialize(Windows::Foundation::Collections::IMapView<hstring, hstring> const &custom_handler);

    static void Uninitialize();

    static double NetworkBufferTimeInSeconds();

    static void NetworkBufferTimeInSeconds(double value);

    static int32_t NetworkBufferSizeInBytes();

    static void NetworkBufferSizeInBytes(int32_t value);

    static bool ForceNetworkMode();

    static void ForceNetworkMode(bool value);

    static bool ForceSoftwareDecode();

    static void ForceSoftwareDecode(bool value);

    static SYEngine::PlaylistSegmentUrlUpdateEventHandler PlaylistSegmentUrlUpdateDelegate();

    static void PlaylistSegmentUrlUpdateDelegate(SYEngine::PlaylistSegmentUrlUpdateEventHandler const &value);

    static SYEngine::PlaylistSegmentDetailUpdateEventHandler PlaylistSegmentDetailUpdateDelegate();

    static void PlaylistSegmentDetailUpdateDelegate(SYEngine::PlaylistSegmentDetailUpdateEventHandler const &value);

    static LPSTR CALLBACK DefaultPlaylistSegmentUrlUpdateCallback(LPCSTR uniqueId,
                                                                  LPCSTR opType,
                                                                  int nCurrentIndex,
                                                                  int nTotalCount,
                                                                  LPCSTR strCurrentUrl);

    static BOOL CALLBACK DefaultPlaylistSegmentDetailUpdateCallback(LPCSTR uniqueId,
                                                                    LPCSTR opType,
                                                                    int nCurrentIndex,
                                                                    int nTotalCount,
                                                                    LPCSTR strCurrentUrl,
                                                                    void *values);

private:
    static PlaylistSegmentUrlUpdateEventHandler url_update_handler_;
    static PlaylistSegmentDetailUpdateEventHandler detail_update_handler_;
};

}

namespace winrt::SYEngine::factory_implementation {
struct Core : CoreT<Core, implementation::Core> { };
}
