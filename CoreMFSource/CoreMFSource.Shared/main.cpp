#include "pch.h"
#include "GlobalSettings.h"
#include "HDCoreByteStreamHandler.h"
#include <wrl\module.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		GlobalOptionStartup();
		Module<InProc>::GetModule().Create();
	}else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		Module<InProc>::GetModule().Terminate();
		GlobalOptionClear();
	}
	return TRUE;
}

STDAPI DllGetActivationFactory(HSTRING activatibleClassId, IActivationFactory** factory)
{
	return Module<InProc>::GetModule().GetActivationFactory(activatibleClassId, factory);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow()
{
	return Module<InProc>::GetModule().Terminate() ? S_OK:S_FALSE;
}