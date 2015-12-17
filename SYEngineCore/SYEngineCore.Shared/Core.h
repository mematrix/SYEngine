#pragma once

#include "pch.h"
#include "MediaExtensionInstaller.h"

namespace SYEngineCore
{
	static MediaExtensionInstaller* Installer;

	public ref class Core sealed
	{
	public:
		static bool Initialize();
		static void Uninitialize();
		
	private:
		Core() {}
	};
}