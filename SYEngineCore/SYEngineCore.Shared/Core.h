#pragma once

#include "pch.h"
#include "MediaExtensionInstaller.h"

//MediaExtensionCallback.cpp
extern double MediaSourceNetworkIOBufTime;
extern unsigned MediaSourceNetworkIOBufSize;

namespace SYEngineCore
{
	static MediaExtensionInstaller* Installer;

	public ref class Core sealed
	{
	public:
		static bool Initialize();
		static void Uninitialize();
		
		static void ChangeNetworkPreloadTime(double bufTime)
		{ MediaSourceNetworkIOBufTime = bufTime; }
		static void ChangeNetworkIOBuffer(int sizeInBytes)
		{ MediaSourceNetworkIOBufSize = (unsigned)sizeInBytes; }

	private:
		Core() {}
	};
}