#pragma once

#include "pch.h"
#include "Playlist.h"
#include "MediaExtensionInstaller.h"

//MediaExtensionCallback.cpp
extern double MediaSourceNetworkIOBufTime;
extern unsigned MediaSourceNetworkIOBufSize;
extern bool MediaSourceForceNetworkMode;

namespace SYEngine
{
	static MediaExtensionInstaller* Installer;

	public delegate Platform::String^ PlaylistSegmentUrlUpdateEventHandler
	(Platform::String^ uniqueId, Platform::String^ opType, int curIndex, int totalCount, Platform::String^ curUrl);
	public delegate bool PlaylistSegmentDetailUpdateEventHandler
	(Platform::String^ uniqueId, Platform::String^ opType, int curIndex, int totalCount, IPlaylistNetworkUpdateInfo^ info);

	public ref class Core sealed
	{
	public:
		static bool Initialize();
		static bool Initialize(Windows::Foundation::Collections::IMapView<Platform::String^,Platform::String^>^ custom);
		static void Uninitialize();

		static property double NetworkBufferTimeInSeconds
		{
			double get() { return MediaSourceNetworkIOBufTime; }
			void set(double value) { MediaSourceNetworkIOBufTime = value; }
		}
		static property int NetworkBufferSizeInBytes
		{
			int get() { return MediaSourceNetworkIOBufSize; }
			void set(int value) { MediaSourceNetworkIOBufSize = value; }
		}

		static property bool ForceNetworkMode
		{
			bool get() { return MediaSourceForceNetworkMode; }
			void set(bool value) { MediaSourceForceNetworkMode = value; }
		}

		static event PlaylistSegmentUrlUpdateEventHandler^ PlaylistSegmentUrlUpdateEvent;
		static event PlaylistSegmentDetailUpdateEventHandler^ PlaylistSegmentDetailUpdateEvent;
	internal:
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
			void* values);

	private:
		Core() {}
	};
}