#pragma once

#include "pch.h"
#include "Playlist.h"
#include "MediaExtensionInstaller.h"

//MediaExtensionCallback.cpp
extern double MediaSourceNetworkIOBufTime;
extern unsigned MediaSourceNetworkIOBufSize;

namespace SYEngineCore
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
		static void Uninitialize();
		
		static void ChangeNetworkPreloadTime(double bufTime)
		{ MediaSourceNetworkIOBufTime = bufTime; }
		static void ChangeNetworkIOBuffer(int sizeInBytes)
		{ MediaSourceNetworkIOBufSize = (unsigned)sizeInBytes; }

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