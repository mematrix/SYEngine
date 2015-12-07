#include "pch.h"
#include <CritSec.h>

CRITICAL_SECTION GlobalCS;
IMFAttributes* GlobalSettings;

void GlobalOptionStartup()
{
	if (GlobalSettings != nullptr)
		return;
	InitializeCriticalSectionEx(&GlobalCS,0,CRITICAL_SECTION_NO_DEBUG_INFO);
	MFCreateAttributes(&GlobalSettings,0);
}

void GlobalOptionClear()
{
	if (GlobalSettings == nullptr)
		return;
	GlobalSettings->Release();
	DeleteCriticalSection(&GlobalCS);
	GlobalSettings = nullptr;
}

BOOL GlobalOptionGetBOOL(const GUID& guid)
{
	if (GlobalSettings == nullptr)
		return FALSE;
	
	EnterCriticalSection(&GlobalCS);
	BOOL Result = MFGetAttributeUINT32(GlobalSettings,guid,FALSE);
	LeaveCriticalSection(&GlobalCS);
	return Result;
}

UINT32 GlobalOptionGetUINT32(const GUID& guid)
{
	if (GlobalSettings == nullptr)
		return 0;

	EnterCriticalSection(&GlobalCS);
	UINT32 Result = MFGetAttributeUINT32(GlobalSettings,guid,0);
	LeaveCriticalSection(&GlobalCS);
	return Result;
}

UINT64 GlobalOptionGetUINT64(const GUID& guid)
{
	if (GlobalSettings == nullptr)
		return 0;

	EnterCriticalSection(&GlobalCS);
	UINT64 Result = MFGetAttributeUINT64(GlobalSettings,guid,0);
	LeaveCriticalSection(&GlobalCS);
	return Result;
}

DOUBLE GlobalOptionGetDouble(const GUID& guid)
{
	if (GlobalSettings == nullptr)
		return 0;

	EnterCriticalSection(&GlobalCS);
	DOUBLE Result = MFGetAttributeDouble(GlobalSettings,guid,0.0);
	LeaveCriticalSection(&GlobalCS);
	return Result;
}

IUnknown* GlobalOptionGetInterface(const GUID& guid)
{
	if (GlobalSettings == nullptr)
		return nullptr;

	EnterCriticalSection(&GlobalCS);
	IUnknown* pResult = nullptr;
	GlobalSettings->GetUnknown(guid,IID_PPV_ARGS(&pResult));
	LeaveCriticalSection(&GlobalCS);
	return pResult;
}

__declspec(dllexport) CRITICAL_SECTION* WINAPI GetCoreMFsGlobalCS()
{ GlobalOptionStartup(); return &GlobalCS; }
__declspec(dllexport) IMFAttributes* WINAPI GetCoreMFsGlobalSettings()
{ GlobalOptionStartup(); return GlobalSettings; }