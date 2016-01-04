#include "pch.h"
#include "UrlHandler.h"
#include <wrl\module.h>

HMODULE khInstance;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		khInstance = hModule;
		DisableThreadLibraryCalls(hModule);
		Module<InProc>::GetModule().Create();
	}else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		Module<InProc>::GetModule().Terminate();
	}
	return TRUE;
}

STDAPI DllGetActivationFactory(HSTRING activatibleClassId, IActivationFactory** factory)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	return E_NOTIMPL;
#else
	return Module<InProc>::GetModule().GetActivationFactory(activatibleClassId, factory);
#endif
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow()
{
	return Module<InProc>::GetModule().Terminate() ? S_OK:S_FALSE;
}