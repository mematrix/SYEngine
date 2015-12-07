#include <Windows.h>
#include "auto_lib.h"

#define DECL_WINHTTP_FN(name) static void* fn##name
#define INIT_WINHTTP_FN(name) fn##name = WinHttpModule->GetProcAddr<void*>(#name);

DECL_WINHTTP_FN(WinHttpAddRequestHeaders);
DECL_WINHTTP_FN(WinHttpCheckPlatform);
DECL_WINHTTP_FN(WinHttpCloseHandle);
DECL_WINHTTP_FN(WinHttpConnect);
DECL_WINHTTP_FN(WinHttpCrackUrl);
DECL_WINHTTP_FN(WinHttpCreateUrl);
DECL_WINHTTP_FN(WinHttpOpen);
DECL_WINHTTP_FN(WinHttpOpenRequest);
DECL_WINHTTP_FN(WinHttpQueryDataAvailable);
DECL_WINHTTP_FN(WinHttpQueryHeaders);
DECL_WINHTTP_FN(WinHttpQueryOption);
DECL_WINHTTP_FN(WinHttpReadData);
DECL_WINHTTP_FN(WinHttpReceiveResponse);
DECL_WINHTTP_FN(WinHttpSendRequest);
DECL_WINHTTP_FN(WinHttpSetCredentials);
DECL_WINHTTP_FN(WinHttpSetOption);
DECL_WINHTTP_FN(WinHttpSetStatusCallback);
DECL_WINHTTP_FN(WinHttpSetTimeouts);
DECL_WINHTTP_FN(WinHttpWriteData);

static AutoLibrary* WinHttpModule;

EXTERN_C BOOL WINAPI WinHttpAddRequestHeaders(void* hRequest,void* pwszHeaders,DWORD dwHeadersLength,DWORD dwModifiers)
{
	typedef BOOL (WINAPI* Callback)(void*,void*,DWORD,DWORD);
	return ((Callback)fnWinHttpAddRequestHeaders)(hRequest,pwszHeaders,dwHeadersLength,dwModifiers);
}

EXTERN_C BOOL WINAPI WinHttpCheckPlatform()
{
	typedef BOOL (WINAPI* Callback)();
	return ((Callback)fnWinHttpCheckPlatform)();
}

EXTERN_C BOOL WINAPI WinHttpCloseHandle(void* hInternet)
{
	typedef BOOL (WINAPI* Callback)(void*);
	return ((Callback)fnWinHttpCloseHandle)(hInternet);
}

EXTERN_C void* WINAPI WinHttpConnect(void* hSession,void* pswzServerName,WORD nServerPort,DWORD dwReserved)
{
	typedef void* (WINAPI* Callback)(void*,void*,WORD,DWORD);
	return ((Callback)fnWinHttpConnect)(hSession,pswzServerName,nServerPort,dwReserved);
}

EXTERN_C BOOL WINAPI WinHttpCrackUrl(void* pwszUrl,DWORD dwUrlLength,DWORD dwFlags,void* lpUrlComponents)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,DWORD,void*);
	return ((Callback)fnWinHttpCrackUrl)(pwszUrl,dwUrlLength,dwFlags,lpUrlComponents);
}

EXTERN_C BOOL WINAPI WinHttpCreateUrl(void* lpUrlComponents,DWORD dwFlags,void* pwszUrl,void* lpdwUrlLength)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,void*,void*);
	return ((Callback)fnWinHttpCreateUrl)(lpUrlComponents,dwFlags,pwszUrl,lpdwUrlLength);
}

EXTERN_C void* WINAPI WinHttpOpen(void* pwszUserAgent,DWORD dwAccessType,void* pwszProxyName,void* pwszProxyBypass,DWORD dwFlags)
{
	typedef void* (WINAPI* Callback)(void*,DWORD,void*,void*,DWORD);
	return ((Callback)fnWinHttpOpen)(pwszUserAgent,dwAccessType,pwszProxyName,pwszProxyBypass,dwFlags);
}

EXTERN_C void* WINAPI WinHttpOpenRequest(void* hConnect,void* pwszVerb,void* pwszObjectName,void* pwszVersion,void* pwszReferrer,void* ppwszAcceptTypes,DWORD dwFlags)
{
	typedef void* (WINAPI* Callback)(void*,void*,void*,void*,void*,void*,DWORD);
	return ((Callback)fnWinHttpOpenRequest)(hConnect,pwszVerb,pwszObjectName,pwszVersion,pwszReferrer,ppwszAcceptTypes,dwFlags);
}

EXTERN_C BOOL WINAPI WinHttpQueryDataAvailable(void* hRequest,void* lpdwNumberOfBytesAvailable)
{
	typedef BOOL (WINAPI* Callback)(void*,void*);
	return ((Callback)fnWinHttpQueryDataAvailable)(hRequest,lpdwNumberOfBytesAvailable);
}

EXTERN_C BOOL WINAPI WinHttpQueryHeaders(void* hRequest,DWORD dwInfoLevel,void* pwszName,void* lpBuffer,void* lpdwBufferLength,void* lpdwIndex)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,void*,void*,void*,void*);
	return ((Callback)fnWinHttpQueryHeaders)(hRequest,dwInfoLevel,pwszName,lpBuffer,lpdwBufferLength,lpdwIndex);
}

EXTERN_C BOOL WINAPI WinHttpQueryOption(void* hInternet,DWORD dwOption,void* lpBuffer,void* lpdwBufferLength)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,void*,void*);
	return ((Callback)fnWinHttpQueryOption)(hInternet,dwOption,lpBuffer,lpdwBufferLength);
}

EXTERN_C BOOL WINAPI WinHttpReadData(void* hRequest,void* lpBuffer,DWORD dwNumberOfBytesToRead,void* lpdwNumberOfBytesRead)
{
	typedef BOOL (WINAPI* Callback)(void*,void*,DWORD,void*);
	return ((Callback)fnWinHttpReadData)(hRequest,lpBuffer,dwNumberOfBytesToRead,lpdwNumberOfBytesRead);
}

EXTERN_C BOOL WINAPI WinHttpReceiveResponse(void* hRequest,void* lpReserved)
{
	typedef BOOL (WINAPI* Callback)(void*,void*);
	return ((Callback)fnWinHttpReceiveResponse)(hRequest,lpReserved);
}

EXTERN_C BOOL WINAPI WinHttpSendRequest(void* hRequest,void* pwszHeaders,DWORD dwHeadersLength,void* lpOptional,DWORD dwOptionalLength,DWORD dwTotalLength,DWORD_PTR dwContext)
{
	typedef BOOL (WINAPI* Callback)(void*,void*,DWORD,void*,DWORD,DWORD,DWORD_PTR);
	return ((Callback)fnWinHttpSendRequest)(hRequest,pwszHeaders,dwHeadersLength,lpOptional,dwOptionalLength,dwTotalLength,dwContext);
}

EXTERN_C BOOL WINAPI WinHttpSetCredentials(void* hRequest,DWORD AuthTargets,DWORD AuthScheme,void* pwszUserName,void* pwszPassword,void* pAuthParams)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,DWORD,void*,void*,void*);
	return ((Callback)fnWinHttpSetCredentials)(hRequest,AuthTargets,AuthScheme,pwszUserName,pwszPassword,pAuthParams);
}

EXTERN_C BOOL WINAPI WinHttpSetOption(void* hInternet,DWORD dwOption,void* lpBuffer,DWORD dwBufferLength)
{
	typedef BOOL (WINAPI* Callback)(void*,DWORD,void*,DWORD);
	return ((Callback)fnWinHttpSetOption)(hInternet,dwOption,lpBuffer,dwBufferLength);
}

EXTERN_C void* WINAPI WinHttpSetStatusCallback(void* hInternet,void* lpfnInternetCallback,DWORD dwNotificationFlags,DWORD_PTR dwReserved)
{
	typedef void* (WINAPI* Callback)(void*,void*,DWORD,DWORD_PTR);
	return ((Callback)fnWinHttpSetStatusCallback)(hInternet,lpfnInternetCallback,dwNotificationFlags,dwReserved);
}

EXTERN_C BOOL WINAPI WinHttpSetTimeouts(void* hInternet,int dwResolveTimeout,int dwConnectTimeout,int dwSendTimeout,int dwReceiveTimeout)
{
	typedef BOOL (WINAPI* Callback)(void*,int,int,int,int);
	return ((Callback)fnWinHttpSetTimeouts)(hInternet,dwResolveTimeout,dwConnectTimeout,dwSendTimeout,dwReceiveTimeout);
}

EXTERN_C BOOL WINAPI WinHttpWriteData(void* hRequest,void* lpBuffer,DWORD dwNumberOfBytesToWrite,void* lpdwNumberOfBytesWritten)
{
	typedef BOOL (WINAPI* Callback)(void*,void*,DWORD,void*);
	return ((Callback)fnWinHttpWriteData)(hRequest,lpBuffer,dwNumberOfBytesToWrite,lpdwNumberOfBytesWritten);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		WinHttpModule = new AutoLibrary("winhttp.dll");
		INIT_WINHTTP_FN(WinHttpAddRequestHeaders);
		INIT_WINHTTP_FN(WinHttpCheckPlatform);
		INIT_WINHTTP_FN(WinHttpCloseHandle);
		INIT_WINHTTP_FN(WinHttpConnect);
		INIT_WINHTTP_FN(WinHttpCrackUrl);
		INIT_WINHTTP_FN(WinHttpCreateUrl);
		INIT_WINHTTP_FN(WinHttpOpen);
		INIT_WINHTTP_FN(WinHttpOpenRequest);
		INIT_WINHTTP_FN(WinHttpQueryDataAvailable);
		INIT_WINHTTP_FN(WinHttpQueryHeaders);
		INIT_WINHTTP_FN(WinHttpQueryOption);
		INIT_WINHTTP_FN(WinHttpReadData);
		INIT_WINHTTP_FN(WinHttpReceiveResponse);
		INIT_WINHTTP_FN(WinHttpSendRequest);
		INIT_WINHTTP_FN(WinHttpSetCredentials);
		INIT_WINHTTP_FN(WinHttpSetOption);
		INIT_WINHTTP_FN(WinHttpSetStatusCallback);
		INIT_WINHTTP_FN(WinHttpSetTimeouts);
		INIT_WINHTTP_FN(WinHttpWriteData);
	}else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		delete WinHttpModule;
	}
	return TRUE;
}