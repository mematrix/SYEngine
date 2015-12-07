#pragma once

#include <Windows.h>
#include <mfobjects.h>

typedef HRESULT (WINAPI* __MFCreateFileFromHandle)(MF_FILE_ACCESSMODE AccessMode,
	MF_FILE_OPENMODE OpenMode,
	MF_FILE_FLAGS fFlags,
	LPCWSTR pwszFileURL,
	HANDLE hFile,
	IMFByteStream **ppIByteStream); //win8
//same MFCreateFile.
//hFile can not is INVALID_HANDLE_VALUE.
//hFile must contain FILE_FLAG_OVERLAPPED.
//pwszFileURL not be NULL, can be L"";
//同MFCreateFile。
//hFile参数不能是INVALID_HANDLE_VALUE，并且在CreateFile时应指定可以异步IO的FILE_FLAG_OVERLAPPED旗帜。
//pwszFileURL不能给NULL，必需给一个字符串指针，随意即可。
//不要对hFile进行CloseHandle，在IMFByteStream接口Release后，会自动CloseHandle。

typedef HRESULT (WINAPI* __MFGetRandomNumber)(BYTE* pbBuffer,DWORD dwLen);
//same CryptGenRandom.
//去看CryptGenRandom后2个参数就是。

typedef HRESULT (WINAPI* __MFCreateMemoryStream)(BYTE* pBuffer,UINT nBufferSize,DWORD dwReserved,IStream** ppstm);
//DO NOT COPY NEW pBuffer DUPLICATE!
//ppstm->Release ago, You can not FREE pBuffer.
//dwReserved must be a 0.
//combining MFSerialize***ToStream and MFDeserialize***FromStream to use.
//注意，ppstm接口只是简单的包装一下pBuffer指针，比如ppstm->Read都是直接操作pBuffer的。
//所以在ppstm->Release前，不能对pBuffer进行free，不然如果有Read则会发生错误。
//参数dwReserved必需是0，这个其实可能是dwFlags。
//此函数一般配合MFSerialize***ToStream和MFDeserialize***FromStream使用。

typedef HRESULT (WINAPI* __MFAppendCollection)(IMFCollection* pAppendTo,IMFCollection* pAppend);
typedef HRESULT (WINAPI* __MFEnumLocalMFTRegistrations)(IUnknown** ppunk); //win8
typedef HRESULT (WINAPI* __MFClearLocalMFTs)(); //win8
typedef DWORD (WINAPI* __MFGetPlatformVersion)(); //win8
typedef DWORD (WINAPI* __MFGetPlatformFlags)(); //win8