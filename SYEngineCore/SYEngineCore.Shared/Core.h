#pragma once

#include "pch.h"
#include "MediaExtensionInstaller.h"

//MediaExtensionCallback.cpp
extern double MediaSourceNetworkIOBufTime;
extern unsigned MediaSourceNetworkIOBufSize;

namespace SYEngineCore
{
	static MediaExtensionInstaller* Installer;

	public delegate Platform::String^ UrlSegmentUpdateEventHandler
	(Platform::String^ uniqueId, int curIndex, int totalCount, Platform::String^ curUrl);
	public ref class Core sealed
	{
	public:
		static bool Initialize();
		static void Uninitialize();
		
		static void ChangeNetworkPreloadTime(double bufTime)
		{ MediaSourceNetworkIOBufTime = bufTime; }
		static void ChangeNetworkIOBuffer(int sizeInBytes)
		{ MediaSourceNetworkIOBufSize = (unsigned)sizeInBytes; }

		static event UrlSegmentUpdateEventHandler^ UrlSegmentUpdateEvent;
	internal:
		static LPSTR CALLBACK DefaultUrlSegmentUpdateCallback(LPCSTR uniqueId, int nCurrentIndex, int nTotalCount, LPCSTR strCurrentUrl);

	private:
		Core() {}
	};
}